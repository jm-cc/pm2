
#include <stdio.h>

#include "pm2.h"

#include "dsmlib_erc_sw_inv_protocol.h"

#define COUNTER 

BEGIN_DSM_DATA
dsm_lock_t L;
END_DSM_DATA

int DSM_SERVICE;
pm2_completion_t c;
int *ptr, *start;
int private_data_size;

#define ITERATIONS 20

void f()
{
  int i, n = ITERATIONS;
//  char *p = (char*)pm2_isomalloc(private_data_size);

  dsm_lock(L);
  tfprintf(stderr,"user thread ! ptr = %p, *ptr = %d (I am %p)\n", ptr, *ptr, marcel_self());
  for (i = 0; i < n; i++) {
    (*ptr)++;
    //    fprintf(stderr,"ptr = %p, *ptr = %d\n",ptr, *ptr);
  }
  tfprintf(stderr,"user thread finished! ptr = %p, *ptr = %d\n", ptr, *ptr);
  dsm_unlock(L);
  pm2_completion_signal(&c); 
}


void threaded_f()
{
  int i, n = ITERATIONS;
  pm2_completion_t my_c;
  int *my_ptr;
//  char *p = (char*)pm2_isomalloc(private_data_size);

  pm2_unpack_completion(SEND_CHEAPER, RECV_CHEAPER, &my_c);
  pm2_unpack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&my_ptr, sizeof(int *));
  pm2_rawrpc_waitdata(); 

  dsm_lock(L);
  tfprintf(stderr,"user thread ! ptr = %p, *ptr = %d (I am %p)\n", my_ptr, *my_ptr, marcel_self());
  for (i = 0; i < n; i++) {
    (*my_ptr)++;
    //    fprintf(stderr,"ptr = %p, *ptr = %d\n",my_ptr, *my_ptr);
  }  
  tfprintf(stderr,"user thread finished! ptr = %p, *ptr = %d\n", my_ptr, *my_ptr);

  dsm_unlock(L);
  pm2_completion_signal(&my_c); 
}


static void DSM_func(void)
{
  pm2_thread_create(threaded_f, NULL);
}


int pm2_main(int argc, char **argv)
{
  int i, j, prot;
  dsm_lock_attr_t attr;

  pm2_rawrpc_register(&DSM_SERVICE, DSM_func);

  //dsm_set_default_protocol(MIGRATE_THREAD);
  dsm_set_default_protocol(LI_HUDAK);
//  isoaddr_page_set_distribution(DSM_CYCLIC, 16);
   isoaddr_page_set_distribution(DSM_BLOCK);

  prot = dsm_create_protocol(dsmlib_erc_sw_inv_rfh, // read_fault_handler 
			     dsmlib_erc_sw_inv_wfh, // write_fault_handler 
			     dsmlib_erc_sw_inv_rs, // read_server
			     dsmlib_erc_sw_inv_ws, // write_server 
			     dsmlib_erc_sw_inv_is, // invalidate_server 
			     dsmlib_erc_sw_inv_rps, // receive_page_server 
			     NULL, // expert_receive_page_server
			     NULL, // acquire_func 
			     dsmlib_erc_release, // release_func 
			     dsmlib_erc_sw_inv_init, // prot_init_func 
			     dsmlib_erc_add_page  // page_add_func 
			     );

  dsm_lock_attr_register_protocol(&attr, prot);

  dsm_lock_init(&L, &attr);
  fprintf(stderr,"L=%p, nb_prot = %d, prot[0] = %d\n", L, L->nb_prot, L->prot[0]);
  pm2_set_dsm_page_protection(DSM_OWNER_WRITE_ACCESS_OTHER_NO_ACCESS);
  pm2_init(&argc, argv);

  if (argc != 6)
    {
      fprintf(stderr, "Usage: test_prot <number of threads per node> <shared pages> <protocol> <offset> <private per thread data>\n");
      exit(1);
    }

//  dsm_display_page_ownership();

  private_data_size =  atoi(argv[5]);

  if(pm2_self() == 0) { /* master process */
    isoaddr_attr_t attr;

    isoaddr_attr_set_atomic(&attr);
    isoaddr_attr_set_status(&attr, ISO_SHARED);
    //isoaddr_attr_set_protocol(&attr, prot);
    //isoaddr_attr_set_protocol(&attr, MIGRATE_THREAD);
    isoaddr_attr_set_protocol(&attr, atoi(argv[3]));
    start = (int *)pm2_malloc((atoi(argv[2]))*4096, &attr);
    ptr = (int *)((char*)start + atoi(argv[4]) * 4096);
    fprintf(stderr,"ptr = %p\n",ptr);

    pm2_completion_init(&c, NULL, NULL);
       *ptr = 0;
    
    /* create local threads */
    for (i=0; i< atoi(argv[1]) ; i++)
      pm2_thread_create(f, NULL); 

    /* create remote threads */
    for (j=1; j < pm2_config_size(); j++)
      for (i=0; i< atoi(argv[1]) ; i++) {
	pm2_rawrpc_begin(j, DSM_SERVICE, NULL);
	pm2_pack_completion(SEND_CHEAPER, RECV_CHEAPER,&c);
	pm2_pack_byte(SEND_CHEAPER, RECV_CHEAPER, (char*)&ptr, sizeof(int*));
	pm2_rawrpc_end();
      }

    for (i = 0 ; i < atoi(argv[1]) * pm2_config_size(); i++)
      pm2_completion_wait(&c);

    tfprintf(stderr, "*ptr=%d\n", *ptr );
    pm2_halt();
  }

  pm2_exit();

  tfprintf(stderr, "Main is ending\n");
  return 0;
}
