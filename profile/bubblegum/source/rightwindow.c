/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2006 Sébastien HARDY <mailto:hardy@enseirb.fr>
 * Copyright (C) 2007 Raphaël BOIS <mailto:bois@enseirb.fr>
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

#include "rightwindow.h"
#include "load.h"

/*! \todo remove global variables. */
AnimElements* anim;
GtkWidget *right_scroll_bar;

static GtkWidget *
init_drawable_zone(AnimationData *p_anim);

static GtkWidget *
init_right_toolbar(AnimationData *p_anim);

static gboolean
Redraw_dz(gpointer p_data);

static void
Realize_dz(GtkWidget* widget, gpointer p_data);

static gboolean
Reshape_dz(GtkWidget* widget, GdkEventConfigure* ev, gpointer p_data);

static gboolean
MouseMove_dz(GtkWidget* widget, GdkEventMotion* ev, gpointer p_data);


/*! Builds the right part of the gui.
 *  
 *  \param p_anim   a pointer to an AnimationData structure.
 *
 *  \return A pointer to a VBox that contains right interface on succes. If an
 *          error occured, returns NULL.
 */
GtkWidget*
right_window_init (AnimationData *p_anim) {
    GtkWidget     *vbox_right = NULL;
    GtkWidget     *drawzone   = NULL;
    GtkWidget     *toolbar    = NULL;
    GtkAdjustment *adj        = NULL;
    GtkWidget     *scroll     = NULL;

    drawzone = init_drawable_zone (p_anim);

    if (drawzone) {
        p_anim->drawzone = drawzone;
        /* \todo Remove global variable. */
        anim = p_anim;
        vbox_right = gtk_vbox_new (FALSE, 0);
        gtk_box_pack_start (GTK_BOX (vbox_right), drawzone, TRUE, TRUE, 0);
        gtk_drawing_area_size (GTK_DRAWING_AREA (drawzone), 100, 100);
    }

    if (vbox_right) {
        toolbar = init_right_toolbar (p_anim);

        if (toolbar) {
            gtk_box_pack_end (GTK_BOX (vbox_right), toolbar, FALSE, FALSE, 0);

            adj = (GtkAdjustment*) gtk_adjustment_new (0, 0, 110, 1, 10, 10);
            scroll = gtk_hscrollbar_new (adj);

            /*! \todo Remove global variable */
            right_scroll_bar = scroll;

            gtk_box_pack_end (GTK_BOX (vbox_right), scroll, FALSE, FALSE, 0);
            g_signal_connect (G_OBJECT (scroll), "value-changed",
                              G_CALLBACK (AnimationSet), p_anim);
        } else {
            gtk_widget_destroy (drawzone);
            drawzone = NULL;
            gtk_widget_destroy (vbox_right);
            vbox_right = NULL;
        }
    }

    return vbox_right;
}


/*! Initializes the drawing area.
 *
 *  \param p_anim   a pointer to an AnimationData structure.
 *
 *  \return a pointer to a drawing area widget. On error, returns NULL.
 */
static GtkWidget *
init_drawable_zone (AnimationData *p_anim) {
    GtkWidget   *drawzone = NULL;
    GdkGLConfig *glconf   = NULL;

    drawzone = gtk_drawing_area_new ();

    /* Sets gl modes. */
    glconf = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGBA | GDK_GL_MODE_DOUBLE |
                                        GDK_GL_MODE_ALPHA);
    if (glconf == NULL) { /* Can't enable double buffer mode  */
        glconf = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGBA | GDK_GL_MODE_ALPHA);
        wprintf (L"info : initiliasation en simple buffer.\n");
    }
    if (glconf == NULL) { /* alpha not supported ? */
        glconf = gdk_gl_config_new_by_mode (GDK_GL_MODE_RGBA);
        wprintf (L"info : initialisation sans couche alpha.\n");
    }
    if (glconf == NULL) {
        wprintf (L"Problème de configuration opengl !\n");
        wprintf (L"Il n'y aura pas d'affichage\n");
        gtk_widget_destroy (drawzone);
        return NULL;
    }
    if (!gtk_widget_set_gl_capability (drawzone, glconf, NULL,
                                       TRUE, GDK_GL_RGBA_TYPE)) {
        wprintf(L"Echec lors de la configuration de la fenêtre openGL !\n");
        gtk_widget_destroy (drawzone);
        return NULL;
    }

    /* Sets events capture for callback methods. */
    gtk_widget_set_events (drawzone,
                           GDK_BUTTON_RELEASE_MASK |
                           GDK_BUTTON_PRESS_MASK |
                           GDK_POINTER_MOTION_MASK);

    /* mouse motion event */
    g_signal_connect (G_OBJECT (drawzone), "motion_notify_event",
                      G_CALLBACK (MouseMove_dz), p_anim);

    /* realize event */
    g_signal_connect (G_OBJECT (drawzone), "realize",
                      G_CALLBACK (Realize_dz), p_anim);

    /* zone resizing event */
    g_signal_connect (G_OBJECT (drawzone), "configure_event",
                      G_CALLBACK (Reshape_dz), p_anim);

    return drawzone;
}


/*! Initializes the right toolbar.
 *
 *  \param p_anim   a pointer to an AnimationData structure.
 *
 *  \return a pointer to a toolbar widget.
 */
static GtkWidget *
init_right_toolbar (AnimationData *p_anim) {
    GtkWidget *toolbar = gtk_toolbar_new();

    GtkWidget *play    = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PLAY,
                                                   GTK_ICON_SIZE_SMALL_TOOLBAR);
    GtkWidget *revplay = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PREVIOUS,
                                                   GTK_ICON_SIZE_SMALL_TOOLBAR);
    GtkWidget *pause   = gtk_image_new_from_stock (GTK_STOCK_MEDIA_PAUSE,
                                                   GTK_ICON_SIZE_SMALL_TOOLBAR);
    GtkWidget *forward = gtk_image_new_from_stock (GTK_STOCK_MEDIA_FORWARD,
                                                   GTK_ICON_SIZE_SMALL_TOOLBAR);
    GtkWidget *rewind  = gtk_image_new_from_stock (GTK_STOCK_MEDIA_REWIND,
                                                   GTK_ICON_SIZE_SMALL_TOOLBAR);
    GtkWidget *stop    = gtk_image_new_from_stock (GTK_STOCK_MEDIA_STOP,
                                                   GTK_ICON_SIZE_SMALL_TOOLBAR);
    GtkWidget *convert = gtk_image_new_from_stock (GTK_STOCK_REFRESH,
                                                   GTK_ICON_SIZE_SMALL_TOOLBAR);

    /*! \todo Maybe replace callback methods. */
    gtk_toolbar_append_element (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                NULL, NULL, "Retour", NULL, stop,
                                G_CALLBACK (AnimationStop), p_anim);
    gtk_toolbar_append_element (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                NULL, NULL, "Rembobiner", NULL, rewind,
                                G_CALLBACK (AnimationRewind), p_anim);
    gtk_toolbar_append_element (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                NULL, NULL, "Lecture arrière", NULL, revplay,
                                G_CALLBACK (AnimationBackPlay), p_anim);
    gtk_toolbar_append_element (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                NULL, NULL, "Pause", NULL, pause,
                                G_CALLBACK (AnimationPause), p_anim);
    gtk_toolbar_append_element (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                NULL, NULL, "Lecture", NULL, play,
                                G_CALLBACK (AnimationPlay), p_anim);
    gtk_toolbar_append_element (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                NULL, NULL, "Avance rapide", NULL, forward,
                                G_CALLBACK (AnimationForward), p_anim);
    
    gtk_toolbar_append_space (GTK_TOOLBAR (toolbar));
    
    gtk_toolbar_append_element (GTK_TOOLBAR (toolbar), GTK_TOOLBAR_CHILD_BUTTON,
                                NULL, NULL, "Convertir en Flash", NULL, convert,
                                G_CALLBACK (Temp), p_anim);
    
    return toolbar;
}


/*! Callback method to finalize openGL drawable zone configuration.
 *
 *  \param widget   a pointer to a drawing area widget.
 *  \param data     a pointer to data passed to the callback method.
 */
static void
Realize_dz (GtkWidget *widget, gpointer data) {
    GdkGLContext  *glcontext  = gtk_widget_get_gl_context (widget);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);
    int i;
    /*! \todo Remove magic number. */
    gchar buf[1024];

    AnimationData *p_anim = (AnimationData *) data;

    if (!gdk_gl_drawable_gl_begin (gldrawable, glcontext)) {
        wprintf(L"Attention : opengl inactif.\n");
        return;
    }

    /* Initialize openGL default status. */
    glClearColor(0.0, 0.0, 0.0, 1.0); /* Black background color. */
    glDisable(GL_DEPTH);              /* No zbuffer. */
    
    glLoadIdentity();
    glEnable(GL_BLEND);               /* Transparency. */
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);   /* No back face culling. */
    
    PushScreenCoordinateMatrix(); /* Sets pixels coordinates. */
    
#if 1 /*! \todo remove texturing. No longer used. */
    /* Loads textures */
    anim->bulle_tex = open_texture("imgs/bulle-translucy.png");
    anim->thread_tex = open_texture("imgs/thread.png");
    
    if (anim->bulle_tex == NULL || anim->thread_tex == NULL)
        exit(1);
#endif
    
    /* Clears buffers. */
    for (i = 0; i < 2; i++) {
        glClear (GL_COLOR_BUFFER_BIT);
        /* Swap buffers. */
        if (gdk_gl_drawable_is_double_buffered (gldrawable))
            gdk_gl_drawable_swap_buffers (gldrawable);
        else
            glFlush();
    }
    
    g_snprintf (buf, sizeof (buf),
               "%s/profile/bubblegum/font/font.ttf", pm2_root());
    
    // chargement de la police et des call lists:
    
    p_anim->pIfont = InitGPFont(buf, 10);
    if (p_anim->pIfont == NULL) {
        wprintf(L"le chargement de ./font/font.ttf à échoué\n");
        exit(1);
    }

    /* End of configuration. */
    gdk_gl_drawable_gl_end (gldrawable);
    
    /* Sets timer to redraw animation. */
    g_timeout_add(50, Redraw_dz, data);
}


/*! Callback method to redraw animation.
 *
 *  \param p_data   a pointer to an #AnimationData structure.
 *
 *  \return TRUE.
 */
static gboolean
Redraw_dz (gpointer p_data) {
    WorkOnAnimation((AnimationData *) p_data);
    return TRUE;
}


/*! Callback method to resize the drawing area
 *
 *  \param widget   a pointer to the drawing area to resize.
 *  \param ev       a pointer to a #GdkEventConfigure structure.
 *  \param p_data   a pointer to an #AnimationData structure.
 *
 *  \return TRUE.
 */
static gboolean
Reshape_dz (GtkWidget* widget, GdkEventConfigure* ev, gpointer p_data) {
    AnimationData *p_anim     = (AnimationData *) p_data;
    GdkGLContext  *glcontext  = gtk_widget_get_gl_context (widget);
    GdkGLDrawable *gldrawable = gtk_widget_get_gl_drawable (widget);

    widget = NULL;  /*  */

    /* Resize drawing area coordinates. */
    p_anim->area.x = ev->width;
    p_anim->area.y = ev->height;

    if (gdk_gl_drawable_gl_begin (gldrawable, glcontext)) {
        glViewport (0, 0, ev->width, ev->height);
        
        PopProjectionMatrix();
        PushScreenCoordinateMatrix();
        
        gdk_gl_drawable_gl_end (gldrawable);
    }
   
    return TRUE;   
}


/*! Callback method to capture mouse motion.
 *
 *  \param widget   a pointer to the drawing area.
 *  \param ev       a pointer to a #GdkEventMotion structure.
 *  \param p_data   a pointer to an #AnimationData structure.
 *
 *  \return TRUE.
 */
static gboolean
MouseMove_dz(GtkWidget* widget, GdkEventMotion* ev, gpointer p_data) {
    widget = NULL;  /* unused. */

    anim->mousePos.x = ev->x;
    anim->mousePos.y = anim->area.y - ev->y;   
   
    return TRUE;
}



#if 0 /*! \todo Remove. No longer in use. */
// quand on repositionne l'ascenseur horizontal
void OnPositionChange(GtkWidget* pWidget, gpointer data)
{
    if (pWidget == NULL)
        return;

    data = 0;
}
#endif
