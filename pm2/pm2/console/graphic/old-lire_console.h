

/* included by xpm2.c */

/* Definition de la liste des Hosts  */
typedef struct _HOST{
	char nom[128];
	char dtid[50];
	char archi[32];
	char speed[50];
}Host;

/* Definition de la liste des Taches PVM */
typedef struct {
	char host[128];
	char tid[32];
	char command[128];
	char threads[50];
	char active[50];
	char blocked[50];
}Pvm_Task;

/* Definition de la liste de Threads */
typedef struct {
	char thread_id[32];
	char num[50];
	char prio[10];
	char running[10];
	char stack[50];
	char migratable[6];
	char service[50];
}Thread;


int nb_host = 0;
int nb_pvm = 0;
int nb_thread = 0;

/* Les selections en cours ..... */
int selection_courante = -1;
int selection_pvm = -1;
int selection_thread = -1;

Host table_host[MAX_HOST+1];
Pvm_Task table_pvm[MAX_TASK+1];
Thread table_thread[MAX_THREAD+1];

void extract_word(char *buf,char nom[128],int *i)
{	int j=*i,k=0;

	
	while((buf[j]==' ') || (buf[j]=='\t'))
		j++;
	while((buf[j] != ' ') && (buf[j] != '\n') && (buf[j] != 0))
	{
		nom[k]=buf[j];
		j++;k++;
	}
	if (buf[j] == '\n') j++;
	nom[k]=0; 
	*i=j;
}

void extract_conf(char *buf)
{
	int i;
	int nb_info = 0;
	char nom[128];

	
	i=0; 
	nb_host = 0;
	while(strcmp(nom,"SPEED"))
		extract_word(buf,nom,&i); 
	
	while((strcmp(nom,"pm2>") != 0))
	{
		if (strcmp(nom,"pm2>") != 0)
		{
		extract_word(buf,nom,&i);
		if (nb_info == 4)
		{
			if(nb_host < MAX_HOST)
				nb_host ++;
			nb_info = 0;
		}
		switch(nb_info)
			{
			case 0 :
				strcpy(table_host[nb_host].nom,nom);
				break;
			case 1 :
				strcpy(table_host[nb_host].dtid,nom);
				break;
			case 2 :
				strcpy(table_host[nb_host].archi,nom);
				break;
			case 3 :
				strcpy(table_host[nb_host].speed,nom);
				break;
			}
		nb_info++;
		}
	}
}

void extract_ps(char *buf)
{
	int i,k,nb_retour=0;
	int nb_info = 0,ne_pas_prendre = 0;
	char nom[128];

	i=0;
	nb_pvm = 0; 
	
	while(strcmp(nom,"COMMAND"))
		extract_word(buf,nom,&i);
	
	while(strcmp(nom,"pm2>") != 0)
	{
		if (strcmp(nom,"pm2>") != 0)
		{
		extract_word(buf,nom,&i);			
		if (nb_info == 3)
		{
			if(nb_pvm < MAX_TASK)
				nb_pvm ++;
			nb_info = 0;
		}
		switch(nb_info)
			{
			case 0 :
				if (!strcmp(table_host[selection_courante].nom,nom))
				{
					strcpy(table_pvm[nb_pvm].host,nom);
					ne_pas_prendre = 0;
				}
				else
					ne_pas_prendre = 1;
				break;
			case 1 :
				strcpy(table_pvm[nb_pvm].tid,nom);
				break;
			case 2 :
				strcpy(table_pvm[nb_pvm].command,nom);
				break;
/*
			case 3 :
				strcpy(table_pvm[nb_pvm].threads,nom);
				break;
			case 4 :
				strcpy(table_pvm[nb_pvm].active,nom);
				break;
			case 5 :
				strcpy(table_pvm[nb_pvm].blocked,nom);
				break;
*/
			}
		if (!ne_pas_prendre) nb_info++;
		}
	}
}


void extract_th(char *buf)
{
	int i,k,nb_retour=0;
	int nb_info = 0;
	char nom[128];

	
	i=0;
	nb_thread = 0; 

	while(strcmp(nom,"SERVICE"))
		extract_word(buf,nom,&i);
	extract_word(buf,nom,&i);

	if(!strcmp(nom,"pm2>"))
           return;

	while(strcmp(nom,table_pvm[selection_pvm].tid))
		extract_word(buf,nom,&i);
	extract_word(buf,nom,&i);

	while(strcmp(nom,"pm2>") != 0)
	{
		extract_word(buf,nom,&i);
		if (strcmp(nom,"pm2>") != 0)
		{
		
		if (nb_info == 7)
		{
			if(nb_thread < MAX_THREAD)
				nb_thread ++;
			nb_info = 0;
		}
		switch(nb_info)
			{
			case 0 :
				strcpy(table_thread[nb_thread].thread_id,nom);
				break;
			case 1 :
				strcpy(table_thread[nb_thread].num,nom);
				break;
			case 2 :
				strcpy(table_thread[nb_thread].prio,nom);
				break;
			case 3 :
				strcpy(table_thread[nb_thread].running,nom);
				break;
			case 4 :
				strcpy(table_thread[nb_thread].stack,nom);
				break;
			case 5 :
				strcpy(table_thread[nb_thread].migratable,nom);
				break;
			case 6 :
				strcpy(table_thread[nb_thread].service,nom);
				break;
			}
		nb_info++;
		}
	}
	if(nb_thread < MAX_THREAD)
		nb_thread ++;
}
