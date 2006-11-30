/**********************************************************************
 * File  : rightwindow.c
 * Author: Hardy
 * Date  : 28/03/2006
 *********************************************************************/

#include "rightwindow.h"
#include "load.h"

AnimElements* anim;
GtkWidget *right_scroll_bar;

GtkWidget* make_right_window(GtkWidget* parent)
{
   GtkWidget* vb_right_interface = gtk_vbox_new(FALSE, 0);

   make_drawable_zone(vb_right_interface);
   make_right_toolbar(vb_right_interface, parent);

   return GTK_WIDGET(vb_right_interface);
}

void make_drawable_zone(GtkWidget* vb_right_interface)
{
   GtkWidget* drawzone;

   drawzone = gtk_drawing_area_new();

   // on enfiche la drawing area dans la place qui reste dans notre partie du hpane
   gtk_box_pack_start(GTK_BOX(vb_right_interface), drawzone, TRUE, TRUE, 0);

   // ceci a pour effet de fixer la taille MINIMUM définitive de la drawzone, en mode fill et expand
   // si on spécifie une taille plus petite que le widget n'a déjà, il sera étendu.
   // si on lui spécifie une taille plus grande, des barres de défilement sont créées.
   gtk_drawing_area_size(GTK_DRAWING_AREA(drawzone), 500, 400);

   // on commence a configurer les modes opengl
   GdkGLConfig* glconf = NULL;
   glconf = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE | GDK_GL_MODE_ALPHA);
   if (glconf == NULL)
   { /* peut etre un probleme, double buffer inacceptable sur cette config ? */
      glconf = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA | GDK_GL_MODE_ALPHA);
      printf("info : initiliasation en simple buffer (double impossible)\n");
   }
   if (glconf == NULL)
   { /* peut etre un probleme, alpha inacceptable sur cette config ? */
      glconf = gdk_gl_config_new_by_mode(GDK_GL_MODE_RGBA);
      printf("info : initiliasation sans alpha (ce sera moins joli)\n");
   }
   if (glconf == NULL)
   {
      wprintf(L"aie : problème de configuration opengl !\n");
      wprintf(L"Il n'y aura pas d'affichage\n");
   } else
   if (!gtk_widget_set_gl_capability(drawzone, glconf, NULL, TRUE, GDK_GL_RGBA_TYPE))
   { /* ok, ya un truc qui n'a pas marché */
      printf("aie : La promotion au rang de widget OpenGL a failli lamentablement.\n");
   }

   anim = AnimationNew(drawzone);  // creation des objets necessaires pour l'animateur

   // on va lui donner une autre promotion au widget (waw c'est un widget colonel maintenant)
   // il peut recevoir des evennements bas niveau de mouvement de souris:
   gtk_widget_set_events(drawzone,
                         GDK_BUTTON_RELEASE_MASK |
                         GDK_BUTTON_PRESS_MASK |
                         GDK_POINTER_MOTION_MASK);

   // quand la souris bouge sur la zone on a des trucs a regarder:
   g_signal_connect(G_OBJECT(drawzone), "motion_notify_event", G_CALLBACK(MouseMove_dz), anim);

   // on lie l'evennement a occurence unique: realize
   g_signal_connect(G_OBJECT(drawzone), "realize", G_CALLBACK(Realize_dz), anim);

   // on lie aussi le redimensionnement pour rester au courant de la taille de la zone:
   g_signal_connect(G_OBJECT(drawzone), "configure_event", G_CALLBACK(Reshape_dz), anim);

   // on rajoute un pti timer qui appelle une callback redraw toutes les 50 ms
   g_timeout_add(50, Redraw_dz, anim);
   // c'est le seul moyen que j'ai trouvé pour faire de l'animation même en laissant
   // le flot d'execution s'engoufrer dans la fonction gtk_main dans laquelle nous
   // n'avons plus la main.
}

// réglages des parametres opengl définitifs
void Realize_dz(GtkWidget *widget, gpointer data)
{
   GdkGLContext* glcontext = gtk_widget_get_gl_context(widget);
   GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable(widget);
   
   if (gdk_gl_drawable_gl_begin(gldrawable, glcontext))
   {
      // zone d'initialisation des états opengl
      
      glClearColor(0.0, 0.0, 0.0, 1.0);  // fond noir
      glDisable(GL_DEPTH);       // pas de zbuffer
      
      glLoadIdentity();
      glEnable(GL_BLEND);   // transparence
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glDisable(GL_CULL_FACE);   // pas de culling

      PushScreenCoordinateMatrix();   // coordonnées en pixels

      anim->bulle_tex = open_texture("imgs/bulle-translucy.png");
      anim->thread_tex = open_texture("imgs/thread.png");

      if (anim->bulle_tex == NULL || anim->thread_tex == NULL)
      {
         exit(1);
      }
      
      int i;
      for (i = 0; i < 2; ++i)  // on nettoie une premiere fois les deux buffers
      {
         glClear(GL_COLOR_BUFFER_BIT);
         /* Swap buffers. */
         if (gdk_gl_drawable_is_double_buffered(gldrawable))
            gdk_gl_drawable_swap_buffers(gldrawable);
         else
            glFlush ();
      }

      gchar buf[1024];
      g_snprintf(buf, sizeof(buf), "%s/profile/bubblegum/font/font.ttf", pm2_root());

      // chargement de la police et des call lists:
      anim->pIfont = InitGPFont(buf, 10);
      if (anim->pIfont == NULL)
      {
         wprintf(L"le chargement de ./font/font.ttf à échoué\n");
         exit(1);
      }
      
      gdk_gl_drawable_gl_end(gldrawable);
   }
   else
   {
      printf("attention : opengl inactif\n");
   }
}

/* callback du timer de la glib */
gboolean Redraw_dz(gpointer anim_parm)
{
   if (anim->pIfont)  // tant que realize n'a pas été produit, pas d'animation
      WorkOnAnimation(anim);
   
   return TRUE;   
}

/* callback de "configure_event" */
gboolean Reshape_dz(GtkWidget* widget, GdkEventConfigure* ev, gpointer anim_parm)
{
   widget = NULL;  // inutilisé

   // lorsque la fenêtre est redimensionnée, il faut redimensionner la zone opengl et adapter la projection.
   anim->area.x = ev->width;
   anim->area.y = ev->height;

   GdkGLContext* glcontext = gtk_widget_get_gl_context(anim->drawzone);
   GdkGLDrawable* gldrawable = gtk_widget_get_gl_drawable(anim->drawzone);

   if (gdk_gl_drawable_gl_begin(gldrawable, glcontext))
   {
      glViewport(0, 0, ev->width, ev->height);
      
      PopProjectionMatrix();
      PushScreenCoordinateMatrix();

      gdk_gl_drawable_gl_end(gldrawable);
   }
   
   return TRUE;   
}

/* callback de "motion_notify_event" */
gboolean MouseMove_dz(GtkWidget* widget, GdkEventMotion* ev, gpointer anim_parm)
{
   widget = NULL;  // inutilisé

   anim->mousePos.x = ev->x;
   anim->mousePos.y = anim->area.y - ev->y;   
   
   return TRUE;
}

void make_right_toolbar(GtkWidget* vb_right_interface, GtkWidget* parent)
{
   GtkWidget* toolbar = gtk_toolbar_new();
   gtk_box_pack_end(GTK_BOX(vb_right_interface), toolbar, FALSE, FALSE, 0);

   GtkWidget* player = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PLAY, GTK_ICON_SIZE_SMALL_TOOLBAR);
   GtkWidget* back_player = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PREVIOUS, GTK_ICON_SIZE_SMALL_TOOLBAR);
   GtkWidget* pauser = gtk_image_new_from_stock(GTK_STOCK_MEDIA_PAUSE, GTK_ICON_SIZE_SMALL_TOOLBAR);
   GtkWidget* forwarder = gtk_image_new_from_stock(GTK_STOCK_MEDIA_FORWARD, GTK_ICON_SIZE_SMALL_TOOLBAR);
   GtkWidget* rewinder = gtk_image_new_from_stock(GTK_STOCK_MEDIA_REWIND, GTK_ICON_SIZE_SMALL_TOOLBAR);
   GtkWidget* stopper = gtk_image_new_from_stock(GTK_STOCK_MEDIA_STOP, GTK_ICON_SIZE_SMALL_TOOLBAR);
   GtkWidget* flash_converter = gtk_image_new_from_stock(GTK_STOCK_REFRESH, GTK_ICON_SIZE_SMALL_TOOLBAR);
  
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL, "Retour", NULL, stopper, G_CALLBACK(AnimationStop), parent);

   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL, "Rembobiner", NULL, rewinder, G_CALLBACK(AnimationRewind), parent);

   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL, "Lecture arrière", NULL, back_player, G_CALLBACK(AnimationBackPlay), parent);
  
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL, "Pause", NULL, pauser, G_CALLBACK(AnimationPause), parent);

   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL, "Lecture", NULL, player, G_CALLBACK(AnimationPlay), parent);

   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL, "Avance rapide", NULL, forwarder, G_CALLBACK(AnimationForward), parent);
  
   gtk_toolbar_append_space(GTK_TOOLBAR(toolbar));

   
   GtkAdjustment* adj = (GtkAdjustment*)gtk_adjustment_new(0, 0, 110, 1, 10, 10);
   
   right_scroll_bar = gtk_hscrollbar_new(adj);
   gtk_box_pack_end(GTK_BOX(vb_right_interface), right_scroll_bar, FALSE, FALSE, 0);
   g_signal_connect(G_OBJECT(right_scroll_bar), "value-changed", G_CALLBACK(OnPositionChange), NULL);

  
   gtk_toolbar_append_element(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_CHILD_BUTTON, NULL, NULL, "Convertir en Flash", NULL, flash_converter, G_CALLBACK(Temp), vb_right_interface);
}

// quand on repositionne l'ascenseur horizontal
void OnPositionChange(GtkWidget* pWidget, gpointer data)
{
   if (pWidget == NULL)
      return;

   data = 0;
}

