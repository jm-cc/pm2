/**********************************************************************
 * File  : animateur.h
 * Author: Lightness1024
 *         mailto:oddouv@enseirb.fr
 * Date  : 21/04/2006
 *********************************************************************/

#ifndef ANIMATEUR_H
#define ANIMATEUR_H 1

#include <wchar.h>
#include <string.h>
#include <math.h>
#include <gtk/gtk.h>
#include <stdlib.h>
#include <stdio.h>
#include <gtk/gtkgl.h>
#include <GL/gl.h>
#include <GL/glu.h>

#ifndef __USE_ISOC99
#ifndef true
typedef unsigned char bool;
#define true 1
#define false 0
#endif
#else
#include <stdbool.h>
#endif

#include "timer.h"
#include "texture.h"
#include "polices.h"
#include "hachage.h"

// définit la valeur d'espacement des frames absolues
// (stockant la totalité de la scène)
// augmenter la valeur diminuera l'occupation mémoire
// la diminuer augmentera la réactivité de rafraichissement
// lors de sauts dans la timeline.
#define KEYFRAME_SPACING 20

#define WORKING_COLOR ARGB(200, 255, 0, 0)
#define READY_COLOR ARGB(200, 0, 255, 0)
#define BLOCKED_COLOR ARGB(200, 0, 0, 255)

#define CONFIG_FILE_NAME "animateur.conf"

#define ARROW_WIDTH 7

//#define DRAW_HIDDEN

// décommenter pour activer la sortie debug
//#define VERBOSE_DEBUG

typedef unsigned char byte;

typedef struct IPair_tag
{
      int i1, i2;
} IPair;


struct Links_tag;
typedef struct Links_tag Links;

struct Links_tag
{
      IPair pair;
      Links* next;
};

typedef struct Vecteur_tag
{
      int x;
      int y;
} Vecteur;

typedef struct Vecteur_fl_tag
{
      float x;
      float y;
} Vecteur_fl;

typedef enum OBJTYPE_tag
{
   BUBBLE_OBJ,
   THREAD_OBJ
} OBJTYPE;

typedef enum STATE_tag
{
   WORKING_STT,
   READY_STT,
   BLOCKED_STT
} STATE;

typedef struct Properties_tag
{
      char* name;
      int id;
      int prior;
      int load;
      int number;
      int holder;
} Properties;

typedef struct ScnObj_tag  // objet de scene
{
      long color;
      Vecteur pos;
      Vecteur size;
      OBJTYPE type;
      Properties prop;
      STATE state;
      
      char* description;  // ne doit pas être accedé directement mais construit avec GetDescription
} ScnObj;

typedef struct Scene_tag  // contient les objets de la scène
{
      ScnObj* objects;
      size_t num;
} Scene;

// informations sur les run queues
typedef struct Queue_tag
{
      ScnObj** objects;
      int num_obj;
      bool exist;
      Vecteur rect1;  // coin inf gauche
      Vecteur rect2;  // coin sup droit
} Queue;

typedef struct QueueContainer_tag
{
      Queue* queues;
      size_t num;
} QueueContainer;

typedef struct RQueues_tag
{
      QueueContainer* levels;
      int level_num;
} RQueues;

// informations sur l'animation
typedef struct ScnObjLightInfos_tag
{
      int index;
      long color;      
      int newRQ;  // nouvelle position
      int newLvl;      
      STATE stt;
} ScnObjLightInfos;

// frame clé restockant le total des positions de la scène
typedef struct KeyFrame_tag
{
      ScnObjLightInfos* absoluteScene;
      size_t num;
} KeyFrame;

// par frame, ne stocker qu'un mouvement
typedef struct Frame_tag
{
      // jusqu'a deux objets modifiés simultanément par frame
      ScnObjLightInfos objChanges[2];
      KeyFrame* keyframe;
} Frame;

/* règles :
     Dans une frame si le pointeur vers keyframe
     est non nul, les deux structures objets ne veulent
     rien dire.
     Si le pointeur est nul, alors au moins le premier objet
     est significatif. Si l'indice de l'objet 2 est différent
     de -1 alors il est significatif lui aussi.
*/

// tableau de frames
typedef struct FramesAr_tag
{
      Frame* frames;
      size_t num;
} FramesAr;

typedef struct AnimElements_tag
{
      bool scene_loaded; /* Is an animation loaded ? */

      Chrono* time;
      GtkWidget* drawzone;
      GPFont* pIfont;
      Vecteur area;  // résolution de la zone
      Vecteur mousePos;
      // textures
      Texture* bulle_tex;
      Texture* thread_tex;
      // scène
      Scene scene;
      Links* links;
      RQueues runQueues;
      FramesAr animation;
      
      HashTable* ht_threads;
      HashTable* ht_bubbles;
      HashTable* ht_rqslvl;
      HashTable* ht_rqspos;      
} AnimElements;


/* déplace un point entre deux points */
/* par rapport a un coefficient de temps normalisé entre 0 et 1 */
/* avec un petit effet smoothy */
Vecteur Interp(const Vecteur* v1, const Vecteur* v2, float time);

AnimElements* AnimationNew(GtkWidget* drawzone);
void AnimationReset(AnimElements *anim, const char *file);
void AnimationStop(GtkWidget *widget, gpointer data);
void AnimationRewind(GtkWidget *widget, gpointer data);
void AnimationBackPlay(GtkWidget *widget, gpointer data);
void AnimationPause(GtkWidget *widget, gpointer data);
void AnimationPlay(GtkWidget *widget, gpointer data);
void AnimationForward(GtkWidget *widget, gpointer data);
//void AnimationSet(GtkRange *range, GtkScrollType *scroll, gdouble value, gpointer user_data);
void AnimationSet(void *a, void*b, void*c, void*d);
void WorkOnAnimation(AnimElements* anim);

void DrawSprite(Texture* texture, int x, int y, int w, int h, long color);

void DrawToolTip(int x, int y, Vecteur res, const char* message, GPFont* ft);

int LoadScene(AnimElements* anim, const char *tracefile);
void DrawFixedScene(AnimElements* anim);

char* GetDescription(ScnObj* obj);
int GetObjUnderMouse(AnimElements* anim);

void DrawLinks(AnimElements* anim);
Links* DeleteLink(Links* lnk, int obj1, int obj2);
Links* CreateLink(Links* lnk, int obj1, int obj2);
int GetSon(Links* lnk, int obj, int i);

void CreateRunQueue(RQueues* rqs, int rqpos, int level);
void DeleteAllRQ(RQueues* rqs);
void AddObjectToRunQueue(RQueues* rqs, ScnObj* obj, int rq, int level);
void DrawRunQueues(AnimElements* anim);
bool CheckRunQExists(RQueues* rqs, int rq, int level);
void DeleteObjOfRQs(RQueues* rqs, ScnObj* obj);
void GetRQCoord(RQueues* rqs, ScnObj* obj, int* lv, int* rq, int* rqpos);

void* DelArrayElement(void* begin, size_t elem_szt, int size, int pos);

void SetPositions(AnimElements* anim);

void ReadFrame(AnimElements* anim, int frame);

char* ConfigGetTraceFileName(char* configfile);

#endif
