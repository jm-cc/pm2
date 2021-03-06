/*
 * NewMadeleine
 * Copyright (C) 2006 (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

#include <Padico/Puk.h>
#include <nm_public.h>
#include <nm_private.h>
#include <nm_launcher.h>
#include <nm_sendrecv_interface.h>
#include <nm_session_interface.h>

#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <ifaddrs.h>
#include <sys/uio.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <netpacket/packet.h>

#include <Padico/Module.h>


static int nm_cmdline_launcher_declare(void);

PADICO_MODULE_BUILTIN(NewMad_Launcher_cmdline, &nm_cmdline_launcher_declare, NULL, NULL);

/* ** Cmd line launcher ************************************ */

static void*     nm_cmdline_launcher_instantiate(puk_instance_t i, puk_context_t c);
static void      nm_cmdline_launcher_destroy(void*_status);

const static struct puk_component_driver_s nm_cmdline_launcher_component_driver =
  {
    .instantiate = &nm_cmdline_launcher_instantiate,
    .destroy     = &nm_cmdline_launcher_destroy
  };

static void         nm_cmdline_launcher_init(void*_status, int*argc, char**argv, const char*group_name);
static int          nm_cmdline_launcher_get_size(void*_status);
static int          nm_cmdline_launcher_get_rank(void*_status);
static void         nm_cmdline_launcher_get_gates(void*_status, nm_gate_t*gates);

const static struct newmad_launcher_driver_s nm_cmdline_launcher_driver =
  {
    .init         = &nm_cmdline_launcher_init,
    .get_size     = &nm_cmdline_launcher_get_size,
    .get_rank     = &nm_cmdline_launcher_get_rank,
    .get_gates    = &nm_cmdline_launcher_get_gates
  };

static int nm_cmdline_launcher_declare(void)
{
  puk_component_declare("NewMad_Launcher_cmdline",
                      puk_component_provides("PadicoComponent", "component", &nm_cmdline_launcher_component_driver),
                      puk_component_provides("NewMad_Launcher", "launcher", &nm_cmdline_launcher_driver ));
  return 0;
}


struct nm_cmdline_launcher_status_s
{
  nm_session_t p_session;
  nm_gate_t p_gate;
  nm_gate_t p_self_gate;
  int is_server;
};

static void*nm_cmdline_launcher_instantiate(puk_instance_t i, puk_context_t c)
{
  struct nm_cmdline_launcher_status_s*status = TBX_MALLOC(sizeof(struct nm_cmdline_launcher_status_s));
  status->p_session = NULL;
  status->p_gate = NM_GATE_NONE;
  status->is_server = 0;
  return status;
}

static void nm_cmdline_launcher_destroy(void*_status)
{
  struct nm_cmdline_launcher_status_s*status = _status;
  int err = nm_session_destroy(status->p_session);
  if(err != NM_ESUCCESS) 
    {
      fprintf(stderr, "# launcher: nm_session_destroy return err = %d\n", err);
    }
  TBX_FREE(status);
}

static int nm_cmdline_launcher_get_rank(void*_status)
{
  struct nm_cmdline_launcher_status_s*status = _status;
  return status->is_server ? 0 : 1;
}

static int nm_cmdline_launcher_get_size(void*_status)
{
  return 2;
}

static void nm_cmdline_launcher_get_gates(void*_status, nm_gate_t *_gates)
{
  struct nm_cmdline_launcher_status_s*status = _status;
  int self = nm_cmdline_launcher_get_rank(status);
  int peer = 1 - self;
  _gates[self] = status->p_self_gate;
  _gates[peer] = status->p_gate;
}

static void nm_launcher_addr_send(int sock, const char*url)
{
  int len = strlen(url) + 1;
  int rc = send(sock, &len, sizeof(len), 0);
  if(rc != sizeof(len))
    {
      fprintf(stderr, "# launcher: cannot send address to peer.\n");
      abort();
    }
  rc = send(sock, url, len, 0);
  if(rc != len)
    {
      fprintf(stderr, "# launcher: cannot send address to peer.\n");
      abort();
    }
}

static void nm_launcher_addr_recv(int sock, char**p_url)
{
  int len = -1;
  int rc = -1;
 retry_recv:
  rc = recv(sock, &len, sizeof(len), MSG_WAITALL);
  int err = errno;
  if(rc == -1 && err == EINTR)
    goto retry_recv;
  if(rc != sizeof(len))
    {
      fprintf(stderr, "# launcher: cannot get address from peer (%s).\n", strerror(err));
      abort();
    }
  char*url = TBX_MALLOC(len);
 retry_recv2:
  rc = recv(sock, url, len, MSG_WAITALL);
  err = errno;
  if(rc == -1 && err == EINTR)
    goto retry_recv2;
  if(rc != len)
    {
      fprintf(stderr, "# launcher: cannot get address from peer (%s).\n", strerror(err));
      abort();
    }
  *p_url = url;
}

void nm_cmdline_launcher_init(void*_status, int *argc, char **argv, const char*_label)
{
  struct nm_cmdline_launcher_status_s*status = _status;
  const char*local_session_url = NULL;
  char*remote_session_url = NULL;
  const char*remote_launcher_url = NULL;

  int i;
  for(i = 1; i < *argc; i++)
    {
      if(strcmp(argv[i], "-R") == 0)
	{
	  /* parse -R for rails */
	  if(*argc <= i+1)
	    {
	      fprintf(stderr, "# launcher: missing railstring.\n");
	      abort();
	    }
	  const char*railstring = argv[i+1];
	  padico_setenv("NMAD_DRIVER", railstring); /* TODO- convert this into an nm_session_set_param() */
	  fprintf(stderr, "# launcher: railstring = %s\n", railstring);
	  *argc = *argc - 2;
	  int j;
	  for(j = i; j < *argc; j++)
	    {
	      argv[j] = argv[j + 2];
	    }
	  i--;
	  continue;
	}
      else if(argv[i][0] != '-')
	{
	  remote_launcher_url = argv[i];
	  fprintf(stderr, "# launcher: remote url = %s\n", remote_launcher_url);
	  /* update command line  */
	  *argc = *argc - 1;
	  int j;
	  for(j = i; j < *argc; j++)
	    {
	      argv[j] = argv[j+1];
	    }
	  break;
	}
      else if(strcmp(argv[i], "--") == 0)
	{
	  /* app args follow; update command line  */
	  *argc = *argc - 1;
	  int j;
	  for(j = i; j < *argc; j++)
	    {
	      argv[j] = argv[j+1];
	    }
	  break;
	}
    }
  argv[*argc] = NULL;
  status->is_server = (!remote_launcher_url);

  int err = nm_session_create(&status->p_session, _label);
  if (err != NM_ESUCCESS)
    {
      fprintf(stderr, "# launcher: nm_session_create returned err = %d\n", err);
      abort();
    }
  err = nm_session_init(status->p_session, argc, argv, &local_session_url);
  if (err != NM_ESUCCESS)
    {
      fprintf(stderr, "# launcher: nm_session_init returned err = %d\n", err);
      abort();
    }


  /* address exchange */
	
  if(!remote_launcher_url)
    {
      /* server */
      char local_launcher_url[16] = { 0 };
      int server_sock = socket(AF_INET, SOCK_STREAM, 0);
      assert(server_sock > -1);
      int val = 1;
      int rc = setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val));
      struct sockaddr_in addr;
      unsigned addr_len = sizeof addr;
      addr.sin_family = AF_INET;
      addr.sin_port = htons(8657 + getuid());
      addr.sin_addr.s_addr = INADDR_ANY;
      rc = bind(server_sock, (struct sockaddr*)&addr, addr_len);
      int err = errno;
      if(rc == -1 && err == EADDRINUSE)
	{
	  /* try again without default port */
	  addr.sin_family = AF_INET;
	  addr.sin_port = htons(0);
	  addr.sin_addr.s_addr = INADDR_ANY;
	  rc = bind(server_sock, (struct sockaddr*)&addr, addr_len);
	}
      if(rc) 
	{
	  fprintf(stderr, "# launcher: bind error (%s)\n", strerror(err));
	  abort();
	}
      rc = getsockname(server_sock, (struct sockaddr*)&addr, &addr_len);
      listen(server_sock, 255);
      struct in_addr inaddr = puk_inet_getaddr();
      snprintf(local_launcher_url, 16, "%08x%04x", htonl(inaddr.s_addr), addr.sin_port);
      if(local_launcher_url[0] == '\0')
	{
	  fprintf(stderr, "# launcher: cannot get local address\n");
	  abort();
	} 
      fprintf(stderr, "# launcher: local url = '%s'\n", local_launcher_url);
      int sock = -1;
    retry_accept:
      sock = accept(server_sock, (struct sockaddr*)&addr, &addr_len);
      err = errno;
      if(sock < 0)
	{
	  if(err == EINTR)
	    goto retry_accept;
	  fprintf(stderr, "# launcher: accept() failed- err = %d (%s)\n", err, strerror(err));
	  abort();
	}
      close(server_sock);
      nm_launcher_addr_send(sock, local_session_url);
      nm_launcher_addr_recv(sock, &remote_session_url);
      close(sock);
    }
  else
    { 
      /* client */
      int sock = socket(AF_INET, SOCK_STREAM, 0);
      assert(sock > -1);
      assert(strlen(remote_launcher_url) == 12);
      in_addr_t peer_addr;
      int peer_port;
      sscanf(remote_launcher_url, "%08x%04x", &peer_addr, &peer_port);
      struct sockaddr_in inaddr = {
	.sin_family = AF_INET,
	.sin_port   = peer_port,
	.sin_addr   = (struct in_addr){ .s_addr = ntohl(peer_addr) }
      };
      int rc = -1;
    retry_connect:
      rc = connect(sock, (struct sockaddr*)&inaddr, sizeof(struct sockaddr_in));
      int err = errno;
      if(rc)
	{
	  if(err == EINTR)
	    goto retry_connect;
	  fprintf(stderr, "# launcher: cannot connect to %s:%d\n", inet_ntoa(inaddr.sin_addr), peer_port);
	  abort();
	}
      nm_launcher_addr_recv(sock, &remote_session_url);
      nm_launcher_addr_send(sock, local_session_url);
      close(sock);
    }

  err = nm_session_connect(status->p_session, &status->p_gate, remote_session_url);
  if (err != NM_ESUCCESS)
    {
      fprintf(stderr, "launcher: nm_session_connect returned err = %d\n", err);
      abort();
    }

  /* connect to self */
  err = nm_session_connect(status->p_session, &status->p_self_gate, local_session_url);
  if (err != NM_ESUCCESS)
    {
      fprintf(stderr, "launcher: self nm_session_connect returned err = %d\n", err);
      abort();
    }

  if(remote_session_url != NULL)
    TBX_FREE(remote_session_url);
}


