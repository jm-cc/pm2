/*
                      PM2 HIGH-PERF/ISOMALLOC
           High Performance Parallel Multithreaded Machine
                           version 3.0

     Gabriel Antoniu, Olivier Aumage, Luc Bouge, Vincent Danjean,
       Christian Perez, Jean-Francois Mehaut, Raymond Namyst

            Laboratoire de l'Informatique du Parallelisme
                        UMR 5668 CNRS-INRIA
                 Ecole Normale Superieure de Lyon

                      External Contributors:
                 Yves Denneulin (LMC - Grenoble),
                 Benoit Planquelle (LIFL - Lille)

                    1998 All Rights Reserved


                             NOTICE

 Permission to use, copy, modify, and distribute this software and
 its documentation for any purpose and without fee is hereby granted
 provided that the above copyright notice appear in all copies and
 that both the copyright notice and this permission notice appear in
 supporting documentation.

 Neither the institutions (Ecole Normale Superieure de Lyon,
 Laboratoire de L'informatique du Parallelisme, Universite des
 Sciences et Technologies de Lille, Laboratoire d'Informatique
 Fondamentale de Lille), nor the Authors make any representations
 about the suitability of this software for any purpose. This
 software is provided ``as is'' without express or implied warranty.
*/

#include <pm2.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>


int nul_cmd();
int conf_cmd();
int help_cmd();
int quit_cmd();
int ps_cmd();
int ping_cmd();
int th_cmd();
int user_cmd();
int migrate_cmd();
int version_cmd();
int read_cmd();
int alias_cmd();
int unalias_cmd();
int freeze_cmd();

extern char *pvm_errlist[];
extern int pvm_nerr;

struct cmd_possible {
   char *cmd;
   int (*fonction) ();
};

struct cmd_possible commandes[]={
  { " ",	nul_cmd},
  { "#",	nul_cmd},
  { "?",	help_cmd},
  { "alias",	alias_cmd},
  { "conf",	conf_cmd},
  { "freeze",	freeze_cmd},
  { "help",	help_cmd},
  { "mig",	migrate_cmd},
  { "ping",	ping_cmd},
  { "ps",	ps_cmd},
  { "quit",	quit_cmd},
  { "read",	read_cmd},
  { "th",	th_cmd},
  { "unalias",	unalias_cmd},
  { "user",	user_cmd},
  { "version",	version_cmd},
  { 0,0 }
};

char *pmp_bandeau={"Pm2 High Perf v1.0 Console - Copyright Cymbalista G. /Hubert Y. Sept 1995"};
char *pmp_underline={"---------------------------------------------------------------"};

typedef struct {
  char *nom;
  char *arch;
  int num;
  int speed;
} machine_t;

static machine_t machine[MAX_MODULES];
static int nb_machines;

static int nb_modules, *les_modules;

typedef struct {
 char cmd_principale[32];
 char *parametres[128];
 int nb_parametre;
} pm2_cmd;

static int xtoi(p)
	char *p;
{
	int i = 0;
	char c;

	while (isxdigit(c = *p++)) {
		i = (i << 4) + c - (isdigit(c) ? '0' : (isupper(c) ? 'A' : 'a') - 10);
	}
	return i;
}


static int tidtoi(p)
	char *p;
{
	if (*p == 't')
		p++;
	return xtoi(p);
}

static void init_machines()
{
  FILE *f;
  char path[128], tempo[256];

  sprintf(path, "%s/.madconf", getenv("MADELEINE_ROOT"));
  f = fopen(path, "r");
  nb_machines = 0;
  while(fgets(tempo, 255, f) != NULL) {
    tempo[strlen(tempo)-1] = '\0';
    machine[nb_machines].nom = malloc(strlen(tempo)+1);
    strcpy(machine[nb_machines].nom, tempo);
    machine[nb_machines].arch = "UNKNOWN";
    machine[nb_machines].num = nb_machines;
    machine[nb_machines].speed = 1000;
    nb_machines++;
  }
  fclose(f);
}

/*----------------------------------------------*/
/*   NUL_CMD :					*/
/*----------------------------------------------*/
int nul_cmd()
{
  return 0;
}


/*----------------------------------------------*/
/*   VERSION_CMD :					*/
/*----------------------------------------------*/
int version_cmd()
{
   printf("Pm2 High Perf v1.0\n");
   return 0;
}


/*----------------------------------------------*/
/*   ALIAS_CMD :				*/
/*----------------------------------------------*/

typedef struct alias_struct {
	char alias[64], command[256];
	struct alias_struct *next, *prev;
} *alias_list;

static alias_list alias_head = NULL, alias_queue = NULL;

static void list_aliases()
{ alias_list l = alias_head;

	while(l != NULL) {
		tprintf("%s\t%s\n", l->alias, l->command);
		l = l->next;
	}
}

static alias_list find_alias(char *name)
{ alias_list l = alias_head;
  int comp;

	while(l != NULL) {
		comp = strcmp(name, l->alias);
		if(comp == 0)
			return l;
		if(comp < 0)
			return NULL;
		l = l->next;
	}
	return NULL;
}

static void insert_alias(alias_list l)
{
	if(alias_head == NULL) {
		alias_head = alias_queue = l;
		l->next = l->prev = NULL;
	} else {
		alias_list ptr;

		ptr = alias_head;
		while(ptr != NULL) {
			if(strcmp(ptr->alias, l->alias) > 0)
				break;
			else
				ptr = ptr->next;
		}
		if(ptr == NULL) {
			l->next = NULL;
			l->prev = alias_queue;
			alias_queue->next = l;
			alias_queue = l;
		} else {
			l->next = ptr;
			l->prev = ptr->prev;
			ptr->prev = l;
			if(l->prev == NULL)
				alias_head = l;
			else
				l->prev->next = l;
		}
	}
}

static void remove_alias(alias_list l)
{
	if(alias_head == l)
		alias_head = l->next;
	if(alias_queue == l)
		alias_queue = l->prev;
	if(l->prev != NULL)
		l->prev->next = l->next;
	if(l->next != NULL)
		l->next->prev = l->prev;
}

int alias_cmd(pm2_cmd instr)
{
	if(instr.nb_parametre == 0) {
		list_aliases();
	} else if(instr.nb_parametre == 1) {
		tfprintf(stderr, "Wrong Parameters .....\n");
		tfprintf(stderr, "Syntax:  alias [ name args ]\n");
		return -1;
	} else {
		alias_list l;
		int i;

		l = find_alias(instr.parametres[0]);
		if(l != NULL) {
			remove_alias(l);
			tfree(l);
		}
		l = (alias_list)tmalloc(sizeof(struct alias_struct));
		strcpy(l->alias, instr.parametres[0]);
		strcpy(l->command, instr.parametres[1]);
		for(i=2; i<instr.nb_parametre; i++) {
			strcat(l->command, " ");
			strcat(l->command, instr.parametres[i]);
		}
		insert_alias(l);
	}
	return 0;
}


/*----------------------------------------------*/
/*   UNALIAS_CMD :				*/
/*----------------------------------------------*/
int unalias_cmd(pm2_cmd instr)
{ alias_list l;

	if(instr.nb_parametre != 1) {
		tfprintf(stderr, "Wrong Parameters .....\n");
		tfprintf(stderr, "Syntax:  unalias name\n");
		return -1;
	}
	l = find_alias(instr.parametres[0]);
	if(l == NULL) {
		tfprintf(stderr, "alias %s did not exist\n", instr.parametres[0]);
		return -1;
	}
	remove_alias(l);
	tfree(l);
	return 0;
}


/*----------------------------------------------*/
/*   USER_CMD :					*/
/*----------------------------------------------*/

int user_cmd(pm2_cmd instr)
{
   int i;
   int this_tid = -1;
   LRPC_REQ(LRPC_CONSOLE_USER) req;
   int skip = 0;


   if (instr.nb_parametre > 0) {
	if(!strcmp(instr.parametres[0], "-t")) {
	   if(instr.nb_parametre < 2) {
              tfprintf(stderr, "Wrong parameters\n");
	      tfprintf(stderr, "Syntax:  user [ -t task_id ] { arg }\n");
              return 0;
	   }
	   this_tid = tidtoi(instr.parametres[1]);
	   skip = 2;
	}
   }

   { char *p;

      p = req.tab;
      for(i=skip; i<instr.nb_parametre; i++) {
         strcpy(p, instr.parametres[i]);
         p += strlen(instr.parametres[i])+1;
      }
      *p = 0;
   }

   for(i = 0; i < nb_modules; i++) {
     if(this_tid == -1 || this_tid == les_modules[i])
       QUICK_LRPC(les_modules[i], LRPC_CONSOLE_USER, &req, NULL);
   }

   return 0;
}

/*----------------------------------------------*/
/* FREEZE_CMD 				        */
/*----------------------------------------------*/

int freeze_cmd(pm2_cmd instr)
{
    int i;
    LRPC_REQ(LRPC_CONSOLE_FREEZE) req;

    if (instr.nb_parametre < 2) {
	tfprintf(stderr, "Wrong Parameters .....\n");
	tfprintf(stderr, "Syntax:  freeze task_id thread_id { thread_id }\n");
	return 0; }
    for (i=1;i<instr.nb_parametre;i++)
    {
	req.thread_id = instr.parametres[i];
	QUICK_ASYNC_LRPC(xtoi(instr.parametres[0]), LRPC_CONSOLE_FREEZE, &req);
    }
    return 0;
}


/*----------------------------------------------*/
/* MIGRATE_CMD 				        */
/*----------------------------------------------*/

int migrate_cmd(pm2_cmd instr)
{
    int i;
    LRPC_REQ(LRPC_CONSOLE_MIGRATE) req;

    if (instr.nb_parametre < 3) {
	tfprintf(stderr, "Wrong Parameters .....\n");
	tfprintf(stderr, "Syntax:  mig src_tid  dest_tid  thread_id { thread_id }\n");
	return 0; }
    for (i=2;i<instr.nb_parametre;i++)
    {
	req.destination = xtoi(instr.parametres[1]);
	req.thread_id = instr.parametres[i];
	QUICK_ASYNC_LRPC(xtoi(instr.parametres[0]), LRPC_CONSOLE_MIGRATE, &req) ;
    }
    return 0;
}


/*----------------------------------------------*/
/*   PING_CMD : commande qui teste si une ou    */
/*   des taches ne sont pas bloquees            */
/*----------------------------------------------*/

pm2_rpc_wait_t _ping_rpc_wait;
extern LRPC_REQ(LRPC_CONSOLE_PING) _ping_req;
extern marcel_sem_t _ping_sem;

int ping_cmd(pm2_cmd instr)
{
   int cpt_tent=1;
   int i;
   char s[13];
   int nb_try,this_tid=-1;
   LRPC_RES(LRPC_CONSOLE_PING) res;

   if (instr.nb_parametre == 0)
      nb_try = 3;
   else {
      int skip;

      if(!strcmp(instr.parametres[0], "-n")) {
         if(instr.nb_parametre < 2) {
            tfprintf(stderr, "Wrong parameters\n");
            tfprintf(stderr, "Syntax:  ping [ -n num ] [ tid ]\n");
            return 0;
         } else {
	    nb_try = atoi(instr.parametres[1]);
            skip = 2;
         }
      } else {
         nb_try = 3;
         skip = 0;
      }
      if(instr.nb_parametre > skip+1) {
         tfprintf(stderr, "Wrong parameters\n");
         tfprintf(stderr, "Syntax:  ping [ -n num ] [ tid ]\n");
         return 0;
      }
      if(instr.nb_parametre > skip)
	 this_tid = tidtoi(instr.parametres[skip]);
   }

   for (i = 0; i < nb_modules; i++) { 

     if(this_tid == -1 || les_modules[i] == this_tid) {

       tprintf("Trying task :[%x]....\n",les_modules[i]);
       marcel_sem_init(&_ping_sem,0);
       QUICK_LRP_CALL(les_modules[i], LRPC_CONSOLE_PING,
		      &_ping_req, &res, &_ping_rpc_wait);
       for(cpt_tent=1;cpt_tent<=nb_try;cpt_tent++) { 
	 BEGIN
	   marcel_sem_timed_P(&_ping_sem, 500);
	 
	 tprintf("\tTry number (%d/%d) : task alive\n", cpt_tent, nb_try);
	 cpt_tent = nb_try+1; /* Aie, que c'est moche ! */
	 
	 EXCEPTION
	   WHEN(TIME_OUT)  
	   if(cpt_tent < nb_try)
	     strcpy(s,"still trying...");
	   else
	     strcpy(s,"aborting.");
	 tprintf("\tTry number (%d/%d) : failed ...%s\n",
		 cpt_tent, nb_try, s);
	 END 
	   }
       _ping_req.version++;
     }
   }
   return 0;
}

/*----------------------------------------------*/
/* TH_CMD : 					*/
/*----------------------------------------------*/

int th_cmd(pm2_cmd instr)
{
   int i, m, ma = -1;
   int this_tid=-1;
   LRPC_RES(LRPC_CONSOLE_THREADS) les_threads;

   if (instr.nb_parametre > 0)
	this_tid=tidtoi(instr.parametres[0]);

   tprintf("\t       THREAD_ID     NUM  PRIO    STATE   STACK  MIGRATABLE  SERVICE\n");

   for(m = 0; m < nb_machines; m++) {
     for(i = m; i < nb_modules; i += nb_machines) {
       if(this_tid == -1 || les_modules[i] == this_tid) {
	 if(ma < m) {
	   tprintf("\nHost : %s \n", machine[m].nom);
	   ma = m;
	 }
	 tprintf("Task : %x  \n",les_modules[i]);
	 QUICK_LRPC(les_modules[i], LRPC_CONSOLE_THREADS, NULL, &les_threads);
       }
     }
   }

   return 0;
}

/*----------------------------------------------*/
/*   CONF_CMD : Donne la liste des hosts        */
/*----------------------------------------------*/

int conf_cmd()
{
 	int i;

	tprintf("\n");
	tprintf("%d host%s, %d data format%s\n",
		nb_machines, (nb_machines > 1 ? "s" : ""), 1, "");
	tprintf("                    HOST     DTID     ARCH   SPEED\n");

	for (i = 0; i < nb_machines; i++)
			tprintf("%24s %8x %8s%8d\n",
					machine[i].nom,
					machine[i].num,
					machine[i].arch,
					machine[i].speed);
	return 0;
}

static char *helptx[] = {

	"? ?       - Same as help",
	"? Syntax:  ? [ command ]",

	"alias alias   - Define/list command aliases",
	"alias Syntax:  alias [ name args ]",

	"conf conf    - List virtual machine configuration",
	"conf Syntax:  conf",
	"conf Output fields:",
	"conf   HOST    host name",
	"conf   DTID    tid base of pvmd",
	"conf   ARCH    xhost architecture",
	"conf   SPEED   host relative speed",

	"freeze freeze  - Freeze threads",
	"freeze Syntax:  freeze task_tid thread_id { thread_id }",

	"help help    - Print helpful information about a command",
	"help Syntax:  help [ command ]",

	"kill kill    - Terminate tasks",
	"kill Syntax:  kill [ options ] tid ...",
	"kill Options:  -c   kill children of tid",

	"mig mig     - Migrate threads between PM2 processes",
	"mig Syntax:  mig src_tid  dest_tid  thread_id { thread_id }",

	"ping ping    - Test if a task is alive",
	"ping Syntax:  ping [ -n num ] [ task_id ]",
	"ping Options : num : Number of tries (default is 3)",


	"ps ps      - List tasks",
	"ps Syntax:  ps [ -a ]",
	"ps Options:  -a       all hosts (default is local)",
	"ps Output fields:",
	"ps   HOST    host name",
	"ps   TID     task id",
	"ps   PTID    parent task id",
	"ps   PID     task process id",
	"ps   COMMAND executable name",
        
	"quit quit    - Exit console",
	"quit Syntax:  quit",

	"read read    - Read input in a specified file",
	"read Syntax:  read filename",

	"th th	  - Show informations on threads",
	"th Syntax: th [ task_id ]",
	"th Output fields:",
	"th   THREAD_ID  identifier",
	"th   NUM        number",
	"th   PRIO       priority",
	"th   RUNNING    running state (yes/no)",
	"th   STACK      stack size",
	"th   MIGRATABLE migration state (yes=enabled/no=disabled)",
	"th   SERVICE    LRPC service executed",

	"unalias unalias - Undefine command alias",
	"unalias Syntax:  unalias name",

        "user user    - Display user-customized information",
        "user Syntax:  user [ -t task_id ] { arg }",

        "version version - Display console version",
        "version Syntax:  version",

	NULL
};


/*-----------------------------------------------*/
/* HELP_CMD : Obtenir de l'aide sur une commande */
/*  de la console Pm2                            */
/*-----------------------------------------------*/

int help_cmd(pm2_cmd instr)
{
 	char **p;
	char *topic;
	int l;
	struct cmd_possible *csp;

	/* if not specified, topic = help */
	if (instr.nb_parametre > 0) {
		topic = instr.parametres[0];
		l = strlen(topic);

		/* search through messages for ones matching topic */
		for (p = helptx; *p; p++) {
			if (!strncmp(topic, *p, l) && ((*p)[l] == ' ' || (*p)[l] == '-'))
				tprintf("%s\n", (*p) + l + 1);
	}

	} else {
		tprintf("Commands are:\n");
		for (csp = commandes; csp->cmd; csp++) {
			l = strlen(csp->cmd);
			for (p = helptx; *p; p++)
				if (!strncmp(csp->cmd, *p, l) && (*p)[l] == ' ') {
					tprintf("  %s\n", (*p) + l + 1);
					break;
				}
		}
	}

	return 0;
}

/*-----------------------------------------------*/
/* QUIT_CMD : Quitte la console.                 */
/*-----------------------------------------------*/

int quit_cmd(pm2_cmd instr)
{
  int self = pm2_self();

  tprintf("\n");
  pm2_kill_modules(&self, 1);
  pm2_exit();
  exit(0);
}

static char *tflgs[] = {
	0, "f", "c", "a", "o", "s", 0, 0,
	"R", "H", "T"
};


char * task_flags(f)
	int f;
{
	static char buf[64];
	int bit, i;

	sprintf(buf, "%x/", f);
	i = sizeof(tflgs)/sizeof(tflgs[0]) - 1;
	bit = 1 << i;
	while (i >= 0) {
		if ((f & bit) && tflgs[i]) {
			strcat(buf, tflgs[i]);
			strcat(buf, ",");
		}
		bit /= 2;
		i--;
	}
	buf[strlen(buf) - 1] = 0;
	return buf;
}

/*-------------------------------------------------*/
/* PS_CMD : donne les processus actifs             */
/*-------------------------------------------------*/


int ps_cmd(pm2_cmd instr)
{
	int i, m;
	char *p;

	if (instr.nb_parametre > 0) {

	  for (p = instr.parametres[0]; *p; p++)

	    switch (*p) {

	    case 'a':
	      break;

	    default:
	      tfprintf(stderr, "Option unknown -%c\n", *p);
	      break;
	    }
	}

	tprintf("                    HOST      TID   COMMAND\n");
	for(i = 0; i < nb_modules; i++) {
	  m = i % nb_machines;
	  tprintf("%24s", machine[m].nom);
	  tprintf(" %8x", les_modules[i]);
	  tprintf("   %-12s", "-");
	  tprintf("\n");
	}

	return 0;
}

/*---------------------------------------------------------*/
/*  GENERER_COMMANDE : separe la ligne de commande  dans   */
/*   la structure pm2_cmd                                  */
/*   EXEMPLE : pm2>add gauloise                            */
/*    instr->cmd_principale contient "add"                 */
/*    instr->parametres[0]="gauloise"                      */
/*    instr->nb_parametre = 1                              */
/*---------------------------------------------------------*/

void generer_commande(char *s, pm2_cmd *instr)
{
    char tampon[1024];
    char *resul;

    instr->nb_parametre=0;
    strcpy(tampon," ");
    strcat(tampon,s);
    resul=strtok(tampon," \t");
    if (resul != NULL)
    	strcpy(instr->cmd_principale,resul);
    else
        strcpy(instr->cmd_principale," ");
    while(resul != NULL)
    {
	resul=strtok(NULL," \t");
	if (resul != NULL)
	{              
					 instr->parametres[instr->nb_parametre]=(char *)tmalloc(sizeof(char)*(strlen(resul)+1));
		strcpy(instr->parametres[instr->nb_parametre],resul);
		instr->nb_parametre++;
	}
    }
}

/*------------------------------------------*/
/*  EXECUTER_COMMANDE : execute la commande */
/*    qui se trouve dans instr              */
/*------------------------------------------*/

void executer_commande(pm2_cmd instr)
{
  struct cmd_possible *rech;

  for(rech = commandes;rech->cmd;rech++)
  {
    if(!strcmp(rech->cmd,instr.cmd_principale))
     {
      (rech->fonction)(instr);
      break;
     }
  }
  if(!rech->cmd)
     tfprintf(stderr, "%s : command not found ..\n",instr.cmd_principale);
}


void liberer_objet(pm2_cmd *instr)
{
   int i;

   for(i=0; i<instr->nb_parametre; i++)
        tfree(instr->parametres[i]);
}

/*----------------------------------------------*/
/* EXECUTE_FILE				        */
/*----------------------------------------------*/

void memoriser_commande(char *cmd)
{ char tmp[1024];
  pm2_cmd instr;

   sprintf(tmp, "alias !! %s", cmd);
   generer_commande(tmp, &instr);
   executer_commande(instr);
   liberer_objet(&instr);
}

/* f doit etre ouvert. Et il ne sera pas ferme... */
void execute_file(FILE *f)
{ char comd[1024];
  pm2_cmd instructions;
  int len;
  boolean expanded;

   LOOP(bcl)
      if(f == stdin) {
         tprintf("%s", "pm2>"); fflush(stdout);
      }
      if(fgets(comd, 1024, f) == NULL)
         EXIT_LOOP(bcl);
      len = strlen(comd);
      if(len > 0) {
         if(comd[len-1] == '\n')
            comd[len-1] = '\0';
         expanded = FALSE;
         while(1) {
            alias_list l;
            char tempo[1024];
            int i;

            generer_commande(comd, &instructions);

            l = find_alias(instructions.cmd_principale);
            if(l == NULL) {
               if(expanded && f == stdin)
                  printf("%s\n", comd);
               break;
            }
            expanded = TRUE;
            strcpy(tempo, l->command);
            for(i=0; i<instructions.nb_parametre; i++) {
               strcat(tempo, " ");
               strcat(tempo, instructions.parametres[i]);
            }
            strcpy(comd, tempo);
            liberer_objet(&instructions);
         }
         if(f == stdin && strlen(comd) > 0)
            memoriser_commande(comd);
         executer_commande(instructions);
         liberer_objet(&instructions);
      }
   END_LOOP(bcl);
}

/*----------------------------------------------*/
/* READ_CMD 				        */
/*----------------------------------------------*/

int read_cmd(pm2_cmd instr)
{ FILE *f;

   if (instr.nb_parametre != 1) {
      tfprintf(stderr, "Wrong Parameters .....\n");
      tfprintf(stderr, "Syntax:  read filename\n");
      return 0;
   }

   f = fopen(instr.parametres[0], "r");
   if(f == NULL) {
      tfprintf(stderr, "Bad filename\n");
      return 0;
   }

   execute_file(f);

   fclose(f);
   return 0;
}


/*---------------*/
/* C'est le main */
/*---------------*/

int pm2_main(argc,argv)
	int argc;
	char **argv;

{
   tprintf("\n%s\n%s\n", pmp_bandeau, pmp_underline);
   
   pm2_init_rpc();

   pm2_init(&argc, argv, 1, &les_modules, &nb_modules);
   nb_modules--; /* On ne se compte pas soi-meme ! */

   init_machines();

   /* Execution du fichier .pm2rc */
   { char path[128];
     FILE *f;

      strcpy(path, getenv("HOME"));
      strcat(path, "/.pm2rc");

      f = fopen(path, "r");
      if(f != NULL) {
         execute_file(f);
         fclose(f);

      }
   }

   execute_file(stdin);

   return 0;
}
