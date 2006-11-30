/**********************************************************************
 * File  : animateur.c
 * Author: Lightness1024
 *         mailto:oddouv@enseirb.fr
 * Date  : 21/04/2006
 *********************************************************************/

#include "animateur.h"
#include "rightwindow.h"

#include <fxt/fut.h>
#include <fxt/fxt.h>
#include <fxt/fxt-tools.h>

#include "read_trace.h"
#include "load.h"

// pour les constantes
struct fxt_code_name fut_code_table2 [] =
{
#include <fut_print.h>
   {0, NULL }
};

static long ARGB(byte a, byte r, byte g, byte b)
{
   return a << 24 | r << 16 | g << 8 | b;
}

static byte Alpha(long c)
{
   return c >> 24;
}

static byte Red(long c)
{
   return (c >> 16) & 0x0000ff;
}

static byte Green(long c)
{
   return (c >> 8) & 0x0000ff;
}

static byte Blue(long c)
{
   return c & 0x0000ff;
}

static int dbg_printf(const char* format, ...)
{
   int ret = 0;
   
   va_list ap;
   va_start(ap, format);
   
#ifdef VERBOSE_DEBUG
   ret = vprintf(format, ap);
#endif

   va_end(ap);
   return ret;
}

// todo: jarter cte craderie
static int GetNumInStr(char* str)
{
   int len = strlen(str);
   int i;
   for (i = 0; i < len; ++i)
   {
      if (str[i] >= 48 && str[i] < 58)
         break;
   }
   int ret = -1;
   if (i < len)
   {
      ret = atoi(str + i);
   }
   return ret;
}

// initialisation
AnimElements* AnimationNew(GtkWidget* drawzone)
{
   AnimElements* newobj = malloc(sizeof(AnimElements));
   
   Chrono* seconds = malloc(sizeof(Chrono));  // création et démarrage d'un chronometre
   newobj->time = seconds;
   newobj->drawzone = drawzone;
   // en attendant mieux à l'evennement realize (voir Realize_dz)
   // (l'initialisation des fonts doit se faire en zone de dessin opengl)
   newobj->pIfont = NULL;
   AnimationReset(newobj, ConfigGetTraceFileName(CONFIG_FILE_NAME));

   return newobj;
}

// TODO: memleak
void AnimationReset(AnimElements *newobj, const char *file)
{
   chrono_init(newobj->time);
   chrono_start(newobj->time);

   // initialisation des autres structures:
   newobj->links = NULL;
   newobj->runQueues.levels = NULL;
   newobj->runQueues.level_num = 0;
   // création obligatoire d'une runqueue level -1
   CreateRunQueue(&newobj->runQueues, 0, -1);

   LoadScene(newobj, file);
}

// charger les elements graphiques de la scène avec leur propriétés
int LoadScene(AnimElements* anim, const char *tracefile)
{
   ev_t* myEvents = malloc_tracetab(tracefile);

   int i = 0;
   printf("\n");
   int bubbles_num = 0;
   int threads_num = 0;
   int rqnum = 0;
   int frames = 0;
   
   // parcours de comptage
   while (!GetFin_Ev(GetEv(myEvents, i)))
   {
      switch (GetCode_Ev(GetEv(myEvents, i)))
      {
         case BUBBLE_SCHED_NEW:   // nouvelle bulle
            bubbles_num++;
            break;
         case FUT_THREAD_BIRTH_CODE:   // nouveau thread
            threads_num++;
            break;
         case FUT_RQS_NEWRQ:
         case FUT_RQS_NEWLWPRQ:
            rqnum++;
            break;
         case FUT_SWITCH_TO_CODE:
         case BUBBLE_SCHED_SWITCHRQ:
	 case SCHED_THREAD_BLOCKED:
            frames++;
      }
      ++i;
   }

   printf("%d bulles, %d threads, %d rqs, %d frames\n", bubbles_num, threads_num, rqnum, frames);
   anim->scene.objects = malloc((bubbles_num + threads_num) * sizeof(ScnObj));
   anim->scene.num = bubbles_num + threads_num;
   // l'argument est a titre indicatif pour optimiser le rapport perte d'espace / vitesse:
   anim->ht_threads = NewHashTable(threads_num+1);
   anim->ht_bubbles = NewHashTable(bubbles_num+1);
   anim->ht_rqslvl = NewHashTable(rqnum+1);
   anim->ht_rqspos = NewHashTable(rqnum+1);
   
   anim->animation.frames = malloc(sizeof(Frame) * frames);
   anim->animation.num = frames;

   gtk_range_set_adjustment(GTK_RANGE(GTK_SCROLLBAR(right_scroll_bar)), GTK_ADJUSTMENT(gtk_adjustment_new(0, 0, frames, 1, 10, 1)));
   
   int ind = 0, ind2;
   long adr, adr2;
   ev_t* evt_i;
   char* thename;
   int id, rpos, lvl;
   // boucle de création des objets
   i = 0;
   while (!GetFin_Ev(GetEv(myEvents, i)))
   {
      evt_i = GetEv(myEvents, i);
      i++;
      switch (GetCode_Ev(evt_i))
      {
         case BUBBLE_SCHED_NEW:
            adr = GetAdr_Bulle_Ev(evt_i);
            AddPair(anim->ht_bubbles, adr, ind);

            dbg_printf("ajout de bulle %lx\n", adr);

            anim->scene.objects[ind].color = ARGB(200, 200, 100, 255);
            anim->scene.objects[ind].pos.x = (ind % 5) * 100;
            anim->scene.objects[ind].pos.y = (ind / 5) * 100;
            anim->scene.objects[ind].size.x = 64;
            anim->scene.objects[ind].size.y = 64;
            anim->scene.objects[ind].type = BUBBLE_OBJ;
            anim->scene.objects[ind].prop.id = ind;
            anim->scene.objects[ind].prop.prior = -1;
            anim->scene.objects[ind].description = NULL;

            AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[ind], 0, -1);
            
            ind++;
            break;
         case SCHED_SETPRIO:
            adr = GetAdr_Bulle_Ev(evt_i);  // indifférent pour les threads ou les bulles

            // déterminons si il s'agit d'un thread ou d'une bulle:
            ind2 = GetData(anim->ht_bubbles, adr);
            if (ind2 == -1)
            {  // ah ben c'etait un thread :)
               ind2 = GetData(anim->ht_threads, adr);
               if (ind2 == -1)
                  wprintf(L"erreur : sched_setprio : objet non trouvé\n");
            }
            anim->scene.objects[ind2].prop.prior = GetPrio_Ev(evt_i);
            break;
         case BUBBLE_SCHED_INSERT_BUBBLE:

            adr = GetAdr_Bulle_Ev(evt_i);
            adr2 = GetMere_Bulle_Ev(evt_i);

            dbg_printf("lier adr: %lx et %lx\n", adr, adr2);
            
            anim->links = CreateLink(anim->links, GetData(anim->ht_bubbles, adr2), GetData(anim->ht_bubbles, adr));            
            break;
         case FUT_THREAD_BIRTH_CODE:

            adr = GetAdr_Thread_Ev(evt_i);
            AddPair(anim->ht_threads, adr, ind);

            anim->scene.objects[ind].color = ARGB(200, 0, 255, 0);
            anim->scene.objects[ind].pos.x = (ind % 5) * 100;
            anim->scene.objects[ind].pos.y = (ind / 5) * 100;
            anim->scene.objects[ind].size.x = 32;
            anim->scene.objects[ind].size.y = 64;
            anim->scene.objects[ind].type = THREAD_OBJ;
            anim->scene.objects[ind].prop.name = "haricot";
            anim->scene.objects[ind].prop.id = -1;
            anim->scene.objects[ind].prop.prior = -1;
            anim->scene.objects[ind].prop.number = -1;
            anim->scene.objects[ind].state = READY_STT;
            anim->scene.objects[ind].description = NULL;

            // ajout immédiat du thread né en runqueue -1
            AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[ind], 0, -1);

            ind++;
            break;
         case BUBBLE_SCHED_INSERT_THREAD:   // nouvelle liaison d'un thread dans une bulle

            adr = GetAdr_Thread_Ev(evt_i);
            adr2 = GetMere_Bulle_Ev(evt_i);

            anim->links = CreateLink(anim->links, GetData(anim->ht_bubbles, adr2), GetData(anim->ht_threads, adr));
            break;
         case SET_THREAD_ID:   // information de l'id

            id = GetId_Thread_Ev(evt_i);
            adr = GetAdr_Thread_Ev(evt_i);
            ind2 = GetData(anim->ht_threads, adr);

            anim->scene.objects[ind2].prop.id = id;            
            break;
         case FUT_RQS_NEWRQ:   // apparition d'une run queue
         case FUT_RQS_NEWLWPRQ:

            dbg_printf("event rq en + :   ");
            
            rpos = GetPlace_Rq_Ev(evt_i);
            lvl = GetLevel_Rq_Ev(evt_i);
            adr = GetAdr_Rq_Ev(evt_i);

            dbg_printf("pos: %d level: %d\n", rpos, lvl);

            if (lvl >= 0)  // la création du lvl -1 est forcée à l'initialisation de toute façon
            {
               CreateRunQueue(&anim->runQueues, rpos, lvl);            
               AddPair(anim->ht_rqslvl, adr, lvl);
               AddPair(anim->ht_rqspos, adr, rpos);
            }
            break;
      }
   }

   int frame = 0;
   int ind3, nb;
   // boucle de chargement de l'animation
   i = 0;
   while (!GetFin_Ev(GetEv(myEvents, i)))
   {
      evt_i = GetEv(myEvents, i);
      i++;
      switch (GetCode_Ev(evt_i))
      {
         case SET_THREAD_NUMBER:  // information classe du thread, n<0: appartien pm2, n>0 utilisateur

            adr = GetAdr_Thread_Ev(evt_i);
            nb = GetNb_Thread_Ev(evt_i);
            ind2 = GetData(anim->ht_threads, adr);

            if (ind2 >= 0 && ind2 < anim->scene.num)
            {
               anim->scene.objects[ind2].prop.number = nb;
            }
               
            break;
         case FUT_SET_THREAD_NAME_CODE:  // nommage d'un thread

            thename = GetNom_Thread_Ev(evt_i);
            adr = GetAdr_Thread_Ev(evt_i);
            ind2 = GetData(anim->ht_threads, adr);

            if (ind2 >= 0)
            {
               int proc;
               proc = GetNumInStr(thename);
               if (proc != -1)
               {
                  AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[ind2], proc, anim->runQueues.level_num - 2);
               }
               anim->scene.objects[ind2].prop.name = thename;
            }
            break;

         case FUT_SWITCH_TO_CODE:  // changement d'activité de thread
            
            adr = GetAdr_Thread_Ev(evt_i);
            adr2 = GetNext_Thread_Ev(evt_i);

            ind2 = GetData(anim->ht_threads, adr);
            ind3 = GetData(anim->ht_threads, adr2);

            dbg_printf("switch adr %lx, ind1: %d, adr %lx, ind2: %d frame %d\n", adr, ind2, adr2, ind3, frame);

            // code final:
            if (ind2 == -1 || ind3 == -1)  // thread jamais né
            {
               break;
            }

            if (frame >= 0 && frame < frames)
            {
               /*
               if (frame % KEYFRAME_SPACING == 0)  // cas initial et cas keyframe
               {
                  anim->animation.frames[frame].keyframe = malloc(sizeof(KeyFrame));
                  // todo: retrouver l'absolue a partir des précedentes
                  
               }
               else  // cas normal: frame relative, frame - 1 existe
               {
               */
                  anim->animation.frames[frame].keyframe = NULL;
                  anim->animation.frames[frame].objChanges[0].index = ind2; 
                  anim->animation.frames[frame].objChanges[1].index = ind3;
                     
                  // passage à l'état dormant
                  anim->animation.frames[frame].objChanges[0].color = READY_COLOR;
                  anim->animation.frames[frame].objChanges[0].stt = READY_STT;
                  anim->animation.frames[frame].objChanges[0].newRQ = -1;
                  // et inversement:
                  anim->animation.frames[frame].objChanges[1].color = WORKING_COLOR;
                  anim->animation.frames[frame].objChanges[1].stt = WORKING_STT;
                  anim->animation.frames[frame].objChanges[1].newRQ = -1;
                  // determine l'ancienne position du thread qui travaillait pour l'échanger
                  // applique les changements à la structure générale:
                  ReadFrame(anim, frame);
               //}
               frame++;
            }
            break;
         case BUBBLE_SCHED_SWITCHRQ:   // déplacement d'un objet

            adr = GetAdr_Bulle_Ev(evt_i);
            adr2 = GetAdr_Rq_Ev(evt_i);

            ind2 = GetData(anim->ht_bubbles, adr);

            if (ind2 == -1)
            {
               ind2 = GetData(anim->ht_threads, adr);
            }
            if (ind2 == -1)
            {
               break;
            }

            rpos = GetData(anim->ht_rqspos, adr2);
            lvl = GetData(anim->ht_rqslvl, adr2);

            AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[ind2], rpos, lvl);

	    int j = 1;
	    /* TODO: recurse */
	    while ((ind3 = GetSon(anim->links, ind2, j++)) != -1)
               AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[ind3], rpos, lvl);
            
            frame++;
            break;
	 case SCHED_THREAD_BLOCKED:
            adr = GetAdr_Thread_Ev(evt_i);

            ind2 = GetData(anim->ht_bubbles, adr);

            if (ind2 == -1)
            {
               ind2 = GetData(anim->ht_threads, adr);
            }
            if (ind2 == -1)
            {
               break;
            }

	    anim->animation.frames[frame].keyframe = NULL;
	    anim->animation.frames[frame].objChanges[0].index = ind2; 
	       
	    // passage à l'état bloqué
	    anim->animation.frames[frame].objChanges[0].color = BLOCKED_COLOR;
	    anim->animation.frames[frame].objChanges[0].stt = BLOCKED_STT;
	    break;
      }
   }

   free_tracetab(myEvents);

/*
   // test
   HashTable* ht = NewHashTable(100);
   AddPair(ht, 0, 1);
   AddPair(ht, 100, 2);
   AddPair(ht, 200, 3);
   AddPair(ht, 3, 5);
   AddPair(ht, 4, 6);

   printf("d %d k %d\n", GetData(ht, 0), 0);
   printf("d %d k %d\n", GetData(ht, 100), 100);
   printf("d %d k %d\n", GetData(ht, 200), 200);
   printf("d %d k %d\n", GetData(ht, 3), 3);
   printf("d %d k %d\n", GetData(ht, 4), 4);
   printf("d %d k %d\n", GetData(ht, 5), 5);

   DeleteHashTable(ht);
   ht = NULL;

   // je créer des objets a la main en attendant mieux
   anim->scene.objects = malloc(3 * sizeof(ScnObj));
   anim->scene.num = 3;

   anim->scene.objects[0].color = ARGB(200, 200, 100, 255);
   anim->scene.objects[0].pos.x = 120;
   anim->scene.objects[0].pos.y = 220;
   anim->scene.objects[0].size.x = 64;
   anim->scene.objects[0].size.y = 64;
   anim->scene.objects[0].type = BUBBLE_OBJ;
   anim->scene.objects[0].prop.name = "bulle de test";
   anim->scene.objects[0].prop.id = 1;
   anim->scene.objects[0].prop.prior = 45;
   anim->scene.objects[0].state = WORKING_STT;
   anim->scene.objects[0].description = NULL;

   anim->scene.objects[1].color = ARGB(240, 255, 0, 0);
   anim->scene.objects[1].pos.x = 220;
   anim->scene.objects[1].pos.y = 120;
   anim->scene.objects[1].size.x = 32;
   anim->scene.objects[1].size.y = 64;
   anim->scene.objects[1].type = THREAD_OBJ;
   anim->scene.objects[1].prop.name = "thread de test";
   anim->scene.objects[1].prop.id = 2;
   anim->scene.objects[1].prop.prior = 90;
   anim->scene.objects[1].state = WORKING_STT;
   anim->scene.objects[1].description = NULL;

   anim->scene.objects[2].color = ARGB(240, 0, 255, 0);
   anim->scene.objects[2].pos.x = 420;
   anim->scene.objects[2].pos.y = 320;
   anim->scene.objects[2].size.x = 32;
   anim->scene.objects[2].size.y = 64;
   anim->scene.objects[2].type = THREAD_OBJ;
   anim->scene.objects[2].prop.name = "haricot vert";
   anim->scene.objects[2].prop.id = 3;
   anim->scene.objects[2].prop.prior = 20;
   anim->scene.objects[2].state = READY_STT;
   anim->scene.objects[2].description = NULL;

   // le premier lien modifie le pointeur
   
   anim->links = CreateLink(anim->links, 0, 1);
   anim->links = CreateLink(anim->links, 0, 2);
   
   DeleteAllRQ(&anim->runQueues);

   CreateRunQueue(&anim->runQueues, 0, 0);
   CreateRunQueue(&anim->runQueues, 1, 0);
   CreateRunQueue(&anim->runQueues, 0, 2);
   CreateRunQueue(&anim->runQueues, 1, 2);

*/

   
/*    CreateRunQueue(&anim->runQueues, 0, 4); */
/*    CreateRunQueue(&anim->runQueues, 0, 1); */
/*    CreateRunQueue(&anim->runQueues, 1, 2); */
/*    CreateRunQueue(&anim->runQueues, 2, 2); */
/*    CreateRunQueue(&anim->runQueues, 1, 4); */
/*    CreateRunQueue(&anim->runQueues, 0, 5); */
/*    CreateRunQueue(&anim->runQueues, 1, 5); */


/*
   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[0], 1, 0);
   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[1], 1, 0);
   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[2], 1, 0);
   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[3], 1, 0);
   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[4], 1, 0);
   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[5], 1, 0);
   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[6], 1, 0);
   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[7], 1, 0);

   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[8], 2, 0);
   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[9], 2, 0);
   
   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[10], 2, 1);
   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[11], 2, 1);

   AddObjectToRunQueue(&anim->runQueues, &anim->scene.objects[9], 5, 0);
*/


   return 0;
}

// permet de répercuter les changements sur certains objets répertoriés dans une frame sur la scène 'globale'
void ReadFrame(AnimElements* anim, int frame)
{
   /* règles:
     dans une frame si le pointeur vers keyframe
     est non nul, les deux structures objets ne veulent
     rien dire.
     si le pointeur est nul, alors au moins le premier objet
     est significatif. si l'indice de l'objet 2 est différent
     de -1 alors il est significatif lui aussi.
   */

   ScnObj* pobj;
   ScnObjLightInfos* pchgs;
   
   if (frame < 0 || frame >= anim->animation.num) // l'indice ne peut pas etre egal au max
      return;
   if (anim->animation.frames[frame].keyframe != NULL) // cas keyframe
   {
   }
   else
   {
      // répercution du premier objet
      pchgs = &anim->animation.frames[frame].objChanges[0];
      pobj = &anim->scene.objects[ pchgs->index ];
      if (pchgs->newRQ >= 0)
         AddObjectToRunQueue(&anim->runQueues, pobj, pchgs->newRQ, pchgs->newLvl);

      pobj->color = pchgs->color;
      pobj->state = pchgs->stt;

      // deuxième objet, si il y a lieu
      pchgs = &anim->animation.frames[frame].objChanges[1];
      if (pchgs->index != -1)
      {
         pobj = &anim->scene.objects[ pchgs->index ];
         if (pchgs->newRQ >= 0)
            AddObjectToRunQueue(&anim->runQueues, pobj, pchgs->newRQ, pchgs->newLvl);
         pobj->color = pchgs->color;
         pobj->state = pchgs->stt;
      }
   }
}

void DrawFixedScene(AnimElements* anim)
{
   Texture* tex = NULL;
   int i;

   DrawRunQueues(anim);
   SetPositions(anim);
   
   for (i = 0; i < anim->scene.num; ++i)  // on dessine toute la scène
   {
#ifndef DRAW_HIDDEN
      if (anim->scene.objects[i].prop.number < 0)  // ne dessine pas les objets internes a pm2
         continue;
#endif
      switch (anim->scene.objects[i].type)
      {
         case THREAD_OBJ:
            tex = anim->thread_tex;
            break;
         case BUBBLE_OBJ:
            tex = anim->bulle_tex;
            break;
      }
      DrawSprite(tex,
                 anim->scene.objects[i].pos.x,
                 anim->scene.objects[i].pos.y,
                 anim->scene.objects[i].size.x,
                 anim->scene.objects[i].size.y,
                 anim->scene.objects[i].color);
   }

   DrawLinks(anim);
}

char* GetDescription(ScnObj* obj)
{
   bool objbub = (obj->type == BUBBLE_OBJ) ? true : false;

   int len = 0;
   if (!objbub)
      len = strlen(obj->prop.name);
   int fields_len = objbub ? 40 : 80;
   fields_len += len;

   if (obj->description == NULL)
   {  // pas encore de chaine de description
      obj->description = malloc(fields_len);
   }
   strcpy(obj->description, "");
   if (!objbub)
   {
      // nom
      strcat(obj->description, "nom : ");
      strcat(obj->description, obj->prop.name);
      strcat(obj->description, "\n");
   }
   // identifiant
   strcat(obj->description, "id : ");
   char itoa[11];    // 4 milliard en decimal = 10 chiffres max
   sprintf(itoa, "%d", obj->prop.id);
   strcat(obj->description, itoa);
   strcat(obj->description, "\n");
   // priorité
   sprintf(itoa, "%d", obj->prop.prior);
   strcat(obj->description, "priorit\xE9 : ");  // 233 = 0xE9 = 'é' en iso-8859-1
   strcat(obj->description, itoa);
   if (!objbub)
   {
      strcat(obj->description, "\n");
      // état
      strcat(obj->description, "\xE9tat : ");
      if (obj->state == WORKING_STT)
         strcat(obj->description, "working\n");
      else if (obj->state == READY_STT)
         strcat(obj->description, "ready\n");
      else if (obj->state == BLOCKED_STT)
         strcat(obj->description, "blocked\n");
      // charge
      strcat(obj->description, "charge : ");
      sprintf(itoa, "%d", obj->prop.load);
      strcat(obj->description, itoa);
   }

   return obj->description;
}


// un link est une liaison entre deux objets pour représenter leur affiliation dans la structure arborescente.
// ils sont dessinés comme des triangles pointant de obj1 vers obj2
// le stockage en mémoire des paires se fait dans une liste chaînée simple.
Links* CreateLink(Links* lnk, int obj1, int obj2)
{
   Links* first = lnk;
   
   if (lnk == NULL)  // cas liste vide
   {
      lnk = malloc(sizeof(Links));
      lnk->next = NULL;
      first = lnk;
   }
   else
   {
      while (lnk->next != NULL)  // on va au dernier maillon
         lnk = lnk->next;
      
      lnk->next = malloc(sizeof(Links));
      lnk = lnk->next;
      lnk->next = NULL;
   }
   lnk->pair.i1 = obj1;
   lnk->pair.i2 = obj2;

   return first;
}

// TODO: avoir plut�t des listes de fils pour les bulles !!
int GetSon(Links *first, int obj, int i) {
   Links *lnk;

   for (lnk = first; lnk; lnk = lnk->next)
      if (lnk->pair.i1 == obj)
         if (!--i)
	    return lnk->pair.i2;
   return -1;
}

// je sais même plus si j'ai testé cette fonction
Links* DeleteLink(Links* first, int obj1, int obj2)
{
   Links* prev = NULL;
   Links* lnk = first;
   
   while (!(lnk->pair.i1 == obj1 && lnk->pair.i2 == obj2))  // recherche du couple
   {
      prev = lnk;
      if (lnk->next == NULL)  // on a pas trouvé le couple on fait rien
         return first;
      lnk = lnk->next;
   }
   // lnk est le maillon concerné
   
   if (prev == NULL)  // cas où on devrait supprimer le premier maillon
   {
      first = lnk->next;
      free(lnk);
   }
   else  // cas général
   {
      prev->next = lnk->next;
      free(lnk);
   }
   return first;
}

void DrawLinks(AnimElements* anim)
{
   Links* lnk = anim->links;

   Vecteur_fl norm;
   Vecteur vect, vect2;
   float dist;
   int i1, i2;
   ScnObj* objs = anim->scene.objects;
   int diminution, v1dim;

   Vecteur center1, center2;

   while (lnk != NULL)  // pour tout element de la liste
   {
      i1 = lnk->pair.i1;
      i2 = lnk->pair.i2;

      center1.x = objs[i1].pos.x + objs[i1].size.x / 2;
      center1.y = objs[i1].pos.y + objs[i1].size.y / 2;
      
      center2.x = objs[i2].pos.x + objs[i2].size.x / 2;
      center2.y = objs[i2].pos.y + objs[i2].size.y / 2;      

      // vecteur liant les deux centres
      vect.x = center2.x - center1.x;
      vect.y = center2.y - center1.y;

      // sa taille:
      dist = sqrt(vect.x * vect.x + vect.y * vect.y);

      // le même normalisé
      norm.x = vect.x / dist;
      norm.y = vect.y / dist;

      // moyenne des deux semi largeurs
      v1dim = (objs[i1].size.x + objs[i1].size.y) / 3.3f;

      // somme des deux diminutions
      diminution = v1dim + (objs[i2].size.x + objs[i2].size.y) / 3.3f;

      // recalcul du vecteur à la bonne taille diminuée
      vect.x = norm.x * (dist - diminution);
      vect.y = norm.y * (dist - diminution);

      // vecteur orthogonal
      vect2.x = norm.y * ARROW_WIDTH;
      vect2.y = -norm.x * ARROW_WIDTH;

      glBegin(GL_TRIANGLES);
      
      glColor4f(0.3, 0.3, 0.3, 0.7);
      glVertex3f(center1.x + v1dim * norm.x + vect2.x, center1.y + v1dim * norm.y + vect2.y, 0);

      glColor4f(0.3, 0.3, 0.3, 0.7);
      glVertex3f(center1.x + v1dim * norm.x - vect2.x, center1.y + v1dim * norm.y - vect2.y, 0);

      glColor4f(0.9, 0.9, 0.9, 0.7);
      glVertex3f(center1.x + v1dim * norm.x + vect.x, center1.y + v1dim * norm.y + vect.y, 0);
      
      glEnd();
      
      lnk = lnk->next;
   }
}

// calcule les positions des objets en fonction de leur
// appartenance aux runqueues
void SetPositions(AnimElements* anim)
{
   int lv, rq, ob, offset = 0, num, sob;
   Queue* q;
   for (lv = 0; lv < anim->runQueues.level_num; ++lv)
   {
      for (rq = 0; rq < anim->runQueues.levels[lv].num; ++rq)
      {
         q = &anim->runQueues.levels[lv].queues[rq];  // chaque queues
#ifndef DRAW_HIDDEN
         num = 0;   // comptage du vrai nombre d'objets a afficher
         for (ob = 0; ob < q->num_obj; ++ob)
            if (q->objects[ob]->prop.number >= 0)
               num++;
#else
         num = q->num_obj;
#endif
         sob = 0;
         for (ob = 0; ob < q->num_obj; ++ob)   // chaque objet
         {
#ifndef DRAW_HIDDEN
            if (q->objects[ob]->prop.number < 0)
               continue;
#endif
            switch (q->objects[ob]->type)
            {
               case THREAD_OBJ:
                  offset = -q->objects[ob]->size.y / 2;  // les threads sont dessinés sous les runqueues
                  break;
               case BUBBLE_OBJ:
                  offset = q->objects[ob]->size.y / 2;   // les bulles dessus
                  break;
            }
            if (num == 1)
               q->objects[ob]->pos.x = q->rect1.x + (q->rect2.x - q->rect1.x) / 2 - q->objects[ob]->size.x / 2;
            else
               q->objects[ob]->pos.x = (float)sob / (num - 1) * (q->rect2.x - q->rect1.x) + q->rect1.x - q->objects[ob]->size.x / 2;
            q->objects[ob]->pos.y = (q->rect1.y + q->rect2.y) / 2 - q->objects[ob]->size.y / 2 + offset;
            sob++;
         }         
      }
   }
}

// créer une run queue dans un niveau a une position donnée
void CreateRunQueue(RQueues* rqs, int rqpos, int level)
{
   /*
     RQueues ->
                niveau0 -> queue0 -> object0
                                     object1
                                     .
                           queue1 -> object0
                                     .
                           .
                           .
                niveau1 -> queue0
                           .
                niveau2
                .
                .
   */
   // eldidou, stp ne fais pas indent-region :-s
   
   int i, num;
   level++;
   if (rqs->level_num <= level)  // nouveau level
   {
      // reallocation des levels:
      rqs->levels = realloc(rqs->levels, sizeof(QueueContainer) * (level + 1));
      for (i = rqs->level_num; i < level; ++i)
      {
         rqs->levels[i].queues = NULL;
         rqs->levels[i].num = 0;
      }
      rqs->level_num = level + 1;
      // allocation d'une rq:
      rqs->levels[level].queues = malloc(sizeof(Queue) * (rqpos + 1));
      rqs->levels[level].num = rqpos + 1;
      for (i = 0; i < rqpos + 1; ++i)
      {  // on initialise les runqueues précédentes meme si elles n'existent pas encore
         rqs->levels[level].queues[i].objects = NULL;
         rqs->levels[level].queues[i].num_obj = 0;
         rqs->levels[level].queues[i].exist = false;
      }
      rqs->levels[level].queues[rqpos].exist = true;

      dbg_printf("nouvelle rq %d niouw lvl %d\n", rqpos, level);  
   }
   else if (level >= 0)  // level existe déjà
   {
      if (rqpos + 1 >= rqs->levels[level].num)
      {
         // reallocation des rq:
         rqs->levels[level].queues = realloc(rqs->levels[level].queues, (rqpos + 1) * sizeof(Queue));
         num = rqs->levels[level].num;
         for (i = num; i < rqpos + 1; ++i)
         {
            rqs->levels[level].queues[i].objects = NULL;
            rqs->levels[level].queues[i].num_obj = 0;
            rqs->levels[level].queues[i].exist = false;
         }
         rqs->levels[level].num = rqpos + 1;
      }
      rqs->levels[level].queues[rqpos].exist = true;

      dbg_printf("nouvelle rq %d lvl %d\n", rqpos, level);  
   }
}

bool CheckRunQExists(RQueues* rqs, int rq, int level)
{
   level++;
   if (rqs != NULL && level >= 0)
   {
      if (rqs->level_num > level)
      {
         if (rqs->levels[level].num > rq)
         {
            if (rqs->levels[level].queues[rq].exist)
               return true;
         }
      }
   }
   return false;
}

void* DelArrayElement(void* begin, size_t elem_szt, int size, int pos)
{
   char* ptr = (char*)begin;

   // lent, bousilleur de pipeline
/*    for (i = pos; i < size - 1; ++i) */
/*    { */
/*       for (j = 0; j < elem_szt; ++j) */
/*          ptr[i * elem_szt + j] = ptr[(i + 1) * elem_szt + j]; */
/*    } */

   // rapide: burst power
   memmove(ptr + pos * elem_szt, ptr + (pos + 1) * elem_szt, elem_szt * (size - pos - 1));

   begin = realloc(begin, elem_szt * (size - 1));
   return begin;
}

void GetRQCoord(RQueues* rqs, ScnObj* obj, int* lv, int* rq, int* rqpos)
{
   int lvi, rqi, ob;
   // recherche voir si l'objet est déjà contenu qq part
   for (lvi = 0; lvi < rqs->level_num; ++lvi)
   {
      for (rqi = 0; rqi < rqs->levels[lvi].num; ++rqi)
      {
         for (ob = 0; ob < rqs->levels[lvi].queues[rqi].num_obj; ++ob)
         {
            if (rqs->levels[lvi].queues[rqi].objects[ob] == obj) // trouvé !!
            {
               // on renseigne les champs et on quitte
               *lv = lvi - 1;
               *rq = rqi;
               *rqpos = ob;
               return;
            }
         }
      }
   }
   *lv = -2;
}

void DeleteObjOfRQs(RQueues* rqs, ScnObj* obj)
{
   int lv, rq, ob;
   
   // recherche voir si l'objet est déjà contenu qq part
   GetRQCoord(rqs, obj, &lv, &rq, &ob);
   lv++;
   if (lv >= 0)     // si trouvé, on va le jarter
   {
      rqs->levels[lv].queues[rq].objects = DelArrayElement(rqs->levels[lv].queues[rq].objects, sizeof(ScnObj*), rqs->levels[lv].queues[rq].num_obj, ob);
      rqs->levels[lv].queues[rq].num_obj--;
   }
}

void AddObjectToRunQueue(RQueues* rqs, ScnObj* obj, int rq, int level)
{
   if (!CheckRunQExists(rqs, rq, level))
   {
      return;
   }
   DeleteObjOfRQs(rqs, obj);
   level++;
   int nm = rqs->levels[level].queues[rq].num_obj;
   // reallocation du tableau d'objets sur cette runqueue
   rqs->levels[level].queues[rq].objects = realloc(rqs->levels[level].queues[rq].objects, (nm + 1) * sizeof(ScnObj*));
   rqs->levels[level].queues[rq].num_obj++;
   rqs->levels[level].queues[rq].objects[nm] = obj;
}

void DeleteAllRQ(RQueues* rqs)
{
   int i;
   for (i = 0; i < rqs->level_num; ++i)
   {
      free(rqs->levels[i].queues);
   }
   free(rqs->levels);
   rqs->levels = NULL;
   rqs->level_num = 0;
}

void DrawRunQueues(AnimElements* anim)
{
   int vspacing, hspacing;
   int i, j;
   int levels = anim->runQueues.level_num;
   
   if (levels == 0)
      return;
   vspacing = anim->area.y / (levels + 1);

   // dessin séparé de chaque niveau:
   int numrq;
   int white;
   Queue* q;
   for (i = 0; i < levels; ++i)
   {
      numrq = anim->runQueues.levels[i].num;
      if (numrq == 0)
          continue;
      hspacing = anim->area.x / numrq;
      white = 0.1 * hspacing;
      
      glBegin(GL_QUADS);
         
      for (j = 0; j < numrq; ++j)
      {
         q = &anim->runQueues.levels[i].queues[j];
         float alpha = (q->exist) ? 1 : 0.4;
         q->rect1.x = white + j * hspacing;
         q->rect1.y = vspacing * (levels - i) - 5;
         q->rect2.x = -white + (j + 1) * hspacing;
         q->rect2.y = vspacing * (levels - i) + 5;
         
         if (i == 0)
            glColor4f(1, 1, 0, alpha);
         else
            glColor4f(0, 0, 1, alpha);
         glVertex3f(q->rect1.x, q->rect1.y, 0);
         glVertex3f(q->rect2.x, q->rect1.y, 0);
         glVertex3f(q->rect2.x, q->rect2.y, 0);
         glVertex3f(q->rect1.x, q->rect2.y, 0);
      }
      
      glEnd();
   }
}

int GetObjUnderMouse(AnimElements* anim)
{
   int i;
   int ind = -1;
   for (i = 0; i < anim->scene.num; ++i)
   {
      Vecteur link;
      switch (anim->scene.objects[i].type)
      {
         case BUBBLE_OBJ:            
            link.x = anim->scene.objects[i].size.x / 2 + anim->scene.objects[i].pos.x - anim->mousePos.x;
            link.y = anim->scene.objects[i].size.y / 2 + anim->scene.objects[i].pos.y - anim->mousePos.y;
            int dist = sqrt(link.x * link.x + link.y * link.y);

            if (dist <= anim->scene.objects[i].size.x / 2)  // on va dire que la bulle n'est pas anisomorphe
            {
               ind = i;
            }
            break;
         case THREAD_OBJ:
            if (anim->mousePos.x > anim->scene.objects[i].pos.x &&
                anim->mousePos.x < anim->scene.objects[i].pos.x + anim->scene.objects[i].size.x &&
                anim->mousePos.y > anim->scene.objects[i].pos.y &&
                anim->mousePos.y < anim->scene.objects[i].pos.y + anim->scene.objects[i].size.y)
            {
               ind = i;
            }
                
            break;
      }
   }
   return ind;
}



void WorkOnAnimation(AnimElements* anim)
{
   static int frames = 0;
   static double oldtime = 0.0;
   double time = chrono_read(anim->time);

   GdkGLContext* glcontext = gtk_widget_get_gl_context(anim->drawzone);
   GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable(anim->drawzone);
   
   /*** OpenGL BEGIN ***/
   if (!gdk_gl_drawable_gl_begin(gldrawable, glcontext))
   {
      // wprintf(L"attention : problème de dessin ogl\n");    // un printf ici risque de flooder
      return;
   }

   // nettoyer le fond et tout redessiner:
   glClear(GL_COLOR_BUFFER_BIT);

   // affichage des infobulles:
   // si le curseur est sur un objet, afficher ses propriétés

   ReadFrame(anim, (int)time);
   gtk_range_set_value(GTK_RANGE(right_scroll_bar), (int)time);
   DrawFixedScene(anim);

   // la souris est-elle au dessus d'un objet ? si oui afficher les infos
   int ind;
   ind = GetObjUnderMouse(anim);
   if (ind != -1)
   {
      DrawToolTip(anim->mousePos.x, anim->mousePos.y, anim->area, GetDescription(&anim->scene.objects[ind]), anim->pIfont);
   }

   // afficher les fps en haut:
   glColor4f(1.0f, 1.0f, 0.0f, 1.0f);
   glTranslated(0, anim->area.y - 15, 0);
   glScalef(0.8, 0.8, 0);
   Print2D(anim->pIfont, "%.2f %dx%d - fps: %.2f", time, anim->area.x, anim->area.y, 1 / (time - oldtime));
   glLoadIdentity();

   /* Swap buffers. */
   if (gdk_gl_drawable_is_double_buffered(gldrawable))
      gdk_gl_drawable_swap_buffers(gldrawable);
   else
      glFlush ();
   
   gdk_gl_drawable_gl_end(gldrawable);
   /*** OpenGL FIN ***/
   ++frames;
   oldtime = time;
}

/* retourne une structure complete */
/* interpolant deux positions en fonction de time */
Vecteur Interp(const Vecteur* v1, const Vecteur* v2, float time)
{
   Vecteur ret;
   float coef, comp;            /* le coefficient et son complementaire */

   coef = 0.5 * sin(M_PI * time - M_PI / 2) + 0.5;
   comp = 1 - coef;
   ret.x = v2->x * coef + v1->x * comp;
   ret.y = v2->y * coef + v1->y * comp;
   
   return ret;
}

void DrawSprite(Texture* texture, int x, int y, int w, int h, long color)
{
   glPushAttrib(GL_TEXTURE_BIT | GL_TRANSFORM_BIT);
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   glColor4ub(Red(color), Green(color), Blue(color), Alpha(color));   
   ActivateTexture(texture);

   glBegin(GL_QUADS);

   glTexCoord2d(0, 1);
   glVertex3f(x, y, 0.0f);

   glTexCoord2d(0, 0);
   glVertex3f(x, h + y, 0.0f);

   glTexCoord2d(1, 0);
   glVertex3f(w + x, h + y, 0.0f);   
   
   glTexCoord2d(1, 1);
   glVertex3f(w + x, y, 0.0f);
       
   glEnd();
   
   glPopMatrix();
   glPopAttrib();
}

void DrawToolTip(int x, int y, Vecteur res, const char* message, GPFont* ft)
{
   static Texture* corner = NULL;
   static bool firstcall = true;
   
   if (firstcall)  // premier appel a la fonction
   {
      corner = open_texture("imgs/corner.png");  // on tente de charger
      firstcall = false;  // ce n'est plus le premier appel
   }
   if (corner == NULL)  // si le chargement a foiré
   {
      return;  // on sort vite (au premier appel et aux prochains)
   }

   // déterminons les dimensions du rectangle englobant la chaine
   // d'abord chercher la plus grande longueur
   int i;
   int len = strlen(message);
   int lines = 1;
   char* message2 = malloc(len + 1);
   memcpy(message2, message, len + 1);  // on fait une nouvelle chaine message avec des \0 au lieu de \n
   for (i = 0; i < len; ++i)
   {      
      if (message2[i] == '\n')
      {
         message2[i] = '\0';
         ++lines;
      }
   }   
   // on peut chercher la max pix len maintenant:
   int oi = 0;
   int maxpixlen = 0;
   int pixlen = 0;
   int* str_offsets = malloc(sizeof(int) * lines);   
   for (i = 0; i < lines; ++i)
   {
      str_offsets[i] = oi;  // remplissage du tableau
      len = strlen(message2 + oi);
      pixlen = GetStringPixelWidth(ft, message2 + oi);
      if (pixlen > maxpixlen)
      {
         maxpixlen = pixlen;
      }
      oi += len + 1;  // on passe a la chaîne d'après en sautant le \0 terminal
   }
   Vecteur rect;
   rect.x = maxpixlen;
   rect.y = lines * 12;

   if (rect.x + x + 32 > res.x)  // la bulle va sortir des bords
      x -= rect.x + 32;
   if (rect.y + y + 32 > res.y)
      y -= rect.y + 32;
      
   // commencons a dessiner
   // d'abord préparer le terrain
   glPushAttrib(GL_TEXTURE_BIT | GL_TRANSFORM_BIT);
   glMatrixMode(GL_MODELVIEW);
   glPushMatrix();
   glLoadIdentity();

   // blanc semi transparent
   glColor4f(1.f, 1.f, 1.f, 0.7f);

   // premier rectangle central
   glBegin(GL_QUADS);

   glVertex3f(x + 16, y + 16, 0.0f);
   glVertex3f(x + 16, y + rect.y + 16, 0.0f);
   glVertex3f(x + 16 + rect.x, y + rect.y + 16, 0.0f);   
   glVertex3f(x + 16 + rect.x, y + 16, 0.0f);

   glEnd();

   DrawSprite(corner, x, y + 16 + rect.y, 16, 16, ARGB(178, 255, 255, 255));
   DrawSprite(corner, x + 32 + rect.x, y + 16 + rect.y, -16, 16, ARGB(178, 255, 255, 255));
   DrawSprite(corner, x + 32 + rect.x, y + 16, -16, -16, ARGB(178, 255, 255, 255));
   DrawSprite(corner, x, y + 16, 16, -16, ARGB(178, 255, 255, 255));

   // faire les 4 côtés manquants:
   glBegin(GL_QUADS);

   glVertex3f(x + 16, y, 0.0f);
   glVertex3f(x + 16, y + 16, 0.0f);
   glVertex3f(x + 16 + rect.x, y + 16, 0.0f);   
   glVertex3f(x + 16 + rect.x, y, 0.0f);

   glVertex3f(x + 16, y + rect.y + 16, 0.0f);
   glVertex3f(x + 16, y + rect.y + 32, 0.0f);
   glVertex3f(x + 16 + rect.x, y + rect.y + 32, 0.0f);   
   glVertex3f(x + 16 + rect.x, y + rect.y + 16, 0.0f);

   glVertex3f(x, y + 16, 0.0f);
   glVertex3f(x, y + rect.y + 16, 0.0f);
   glVertex3f(x + 16, y + rect.y + 16, 0.0f);   
   glVertex3f(x + 16, y + 16, 0.0f);

   glVertex3f(x + 16 + rect.x, y + 16, 0.0f);
   glVertex3f(x + 16 + rect.x, y + rect.y + 16, 0.0f);
   glVertex3f(x + 32 + rect.x, y + rect.y + 16, 0.0f);   
   glVertex3f(x + 32 + rect.x, y + 16, 0.0f);

   glEnd();

   glTranslatef(x + 16, y + 16 + lines * 12, 0);
   glColor4f(0.f, 0.0f, 0.7f, 1.0f);
   // lignes de texte
   for (i = 0; i < lines; ++i)
   {
      glTranslatef(0.f, -12, 0.f);
      Print2D(ft, message2 + str_offsets[i]);
   }

   free(str_offsets);  // ces free là sont super importants
   free(message2);
   glPopMatrix();
   glPopAttrib();
}

char* ConfigGetTraceFileName(char* configfile)
{
   FILE* f = NULL;
   static char str[512];
   int n;

   f = fopen(configfile, "r");
   if (f == NULL) {
	perror("fopen");
   	fprintf(stderr,"can't open %s\n",configfile);
      return NULL;
   }
   do
   {
      fgets(str, 512, f);
   } while (str[0] == '#');
   
   n = strlen(str);
   if (str[n-1] == '\n')
	   str[n-1] = '\0';

   return str;
}
