

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/time.h>
#include "gtk/gtk.h"
#include "libgimp/gimp.h"
#include "libgimp/gimpui.h"

#include <pm2.h>
#include "rpc_defs.h"
#include "regul.h"

char GIMP_PLUGIN[256]; /*   "/usr/local/lib/gimp/0.99/plug-ins/pm2-pix" */

static struct timeval _t1, _t2;
static struct timezone _tz;
static unsigned long _temps_residuel = 0;

#define top1() gettimeofday(&_t1, &_tz)

#define top2() gettimeofday(&_t2, &_tz)

void init_cpu_time(void)
{
   top1(); top2();
   _temps_residuel = 1000000L * _t2.tv_sec + _t2.tv_usec - 
                     (1000000L * _t1.tv_sec + _t1.tv_usec );
}

unsigned long cpu_time(void)
{
   return 1000000L * _t2.tv_sec + _t2.tv_usec - 
           (1000000L * _t1.tv_sec + _t1.tv_usec ) - _temps_residuel;
}

/* Declare local functions. */

static void	query	(void);
static void	run	(gchar	 *name,
			 gint	 nparams,
			 GParam	 *param,
			 gint	 *nreturn_vals,
			 GParam	 **return_vals);

static gint	pixelize_dialog (void);
static void	pixelize_close_callback	 (GtkWidget *widget,
					  gpointer   data);
static void	pixelize_ok_callback	 (GtkWidget *widget,
					  gpointer   data);
static void	pixelize_onepernode_callback	 (GtkWidget *widget,
						  gpointer   data);
static void     pixelize_scale_callback (GtkAdjustment *adjustment, 
					 gpointer data);
static void	pixelize_distrib_callback	 (GtkWidget *widget,
						  gpointer   data);

static void	pixelize	(GDrawable *drawable);
static void	do_pixelize	(GDrawable *drawable, gint pixelwidth, gint tile_width);
static gint	pixelize_sub ( gint pixelwidth, gint bpp, PixelArea area );

void   entscale_int_new ( GtkWidget *table, gint x, gint y,
			  gchar *caption, gint *intvar, 
			  gint min, gint max, gint constraint,
			  EntscaleIntCallbackFunc callback,
			  gpointer data );
static void   entscale_int_destroy_callback (GtkWidget *widget,
					     gpointer data);
static void   entscale_int_scale_update (GtkAdjustment *adjustment,
					 gpointer      data);
static void   entscale_int_entry_update (GtkWidget *widget,
					 gpointer   data);


GPlugInInfo PLUG_IN_INFO =
{
  NULL,	   /* init_proc */
  NULL,	   /* quit_proc */
  query,   /* query_proc */
  run	   /* run_proc */
};

static PixelizeValues pvals =
{
  8,
  0,
  DISTRIB_RANDOM
};

static PixelizeInterface pint =
{
  FALSE
};

static int nb_modules, *les_modules;

static GPixelRgn src_rgn, dest_rgn;
static GtkWidget *proc_scale, *proc_label;

static gint progress, max_progress, courageux, init = FALSE;

static marcel_sem_t sem_wait;

/***** Functions *****/

/*
  Pixelize Effect
 */

BEGIN_SERVICE(TILE)
  LRPC_REQ(RESULT) result;

  pm2_enable_migration();

  result.changed = pixelize_sub(req.pixelwidth, req.bpp, req.area);

  result.area = req.area;
  result.data = req.data;

  pm2_disable_migration();

  result.module = pm2_self();

  QUICK_ASYNC_LRPC(les_modules[0], RESULT, &result);

END_SERVICE(TILE)

BEGIN_SERVICE(RESULT)

  lock_task();

  if(!req.changed && req.module != pm2_self())
     gimp_pixel_rgn_get_rect(&src_rgn, req.data, 
			     req.area.x, req.area.y, req.area.w, req.area.h);

  gimp_pixel_rgn_set_rect(&dest_rgn, req.data,
			  req.area.x, req.area.y, req.area.w, req.area.h);

  progress += req.area.w * req.area.h;
  gimp_progress_update ((double) progress / (double) max_progress);

  if(req.changed)
     courageux++;

  unlock_task();

  marcel_sem_V(&sem_wait);

END_SERVICE(RESULT)

static void
pixelize (GDrawable *drawable)
{
  gint pixelwidth;
  int argc = 1;
  char *argv[] = { GIMP_PLUGIN, NULL };

  sprintf(GIMP_PLUGIN, "%s/.gimp/plug-ins/pm2-pix", getenv("HOME"));

  init_cpu_time();

  pm2_init_rpc();

  DECLARE_LRPC_WITH_NAME(TILE, "tile", DO_NOT_OPTIMIZE);
  DECLARE_LRPC_WITH_NAME(RESULT, "result", OPTIMIZE_IF_LOCAL);

  regul_init_rpc();

#ifdef DEBUG
  if(pvals.nbprocs == 0)
    printf("Starting one process per node\n");
  else
    printf("Starting %d processes\n", pvals.nbprocs);

  switch(pvals.distrib) {
  case DISTRIB_RANDOM : printf("Distributing threads randomly\n"); break;
  case DISTRIB_CYCLIC : printf("Distributing threads cyclicly\n"); break;
  case DISTRIB_ON_FIRST : printf("Allocating all threads on first node\n"); break;
  default : fprintf(stderr, "Error: unknown distribution parameter\n");
  }
#endif

  pm2_init(&argc, argv, pvals.nbprocs, &les_modules, &nb_modules);

  regul_init(les_modules, nb_modules);

  pixelwidth = pvals.pixelwidth;

  if (pixelwidth < 0)
    pixelwidth = - pixelwidth;
  if (pixelwidth < 1)
    pixelwidth = 1;

  marcel_sem_init(&sem_wait, 0);

  do_pixelize( drawable, pixelwidth, gimp_tile_width() );

  pm2_kill_modules(les_modules, nb_modules);

  pm2_exit();
}

int distrib_random_func()
{
  return les_modules[rand() % nb_modules];
}

int distrib_cyclic_func()
{
  static int n = 0;

  return les_modules[++n % nb_modules];
}

int distrib_on_first_func()
{
  return les_modules[0];
}

static int (*distrib_func[3])() = { 
  distrib_random_func, 
  distrib_cyclic_func, 
  distrib_on_first_func
};

/*
   This function operates on PixelArea, whose width and height are
   multiply of pixel width, and less than the tile size (to enhance
   its speed).

   If any coordinates of mask boundary is not multiply of pixel width
   ( e.g.  x1 % pixelwidth != 0 ), operates on the region whose width
   or height is the remainder.
 */
static void
do_pixelize ( GDrawable *drawable, gint pixelwidth, gint tile_width )
{
  gint bpp;
  gint x1, y1, x2, y2;
  LRPC_REQ(TILE) req;
  gint total = 0;

  req.num = 0;
  req.pixelwidth = pixelwidth;

  gimp_drawable_mask_bounds (drawable->id, &x1, &y1, &x2, &y2);
  gimp_pixel_rgn_init (&src_rgn, drawable, x1, y1, x2-x1, y2-y1, FALSE, FALSE);
  gimp_pixel_rgn_init (&dest_rgn, drawable, x1, y1, x2-x1, y2-y1, TRUE, TRUE);

  /* Initialize progress */
  progress = 0;
  max_progress = (x2 - x1) * (y2 - y1);

  req.bpp = bpp = drawable->bpp;

  req.area.data = req.data;
  req.area.width = ( tile_width / pixelwidth ) * pixelwidth;

  top1();
  for( req.area.y = y1; req.area.y < y2;
       req.area.y += req.area.width - ( req.area.y % req.area.width ) )
    {
      req.area.h = req.area.width - ( req.area.y % req.area.width );
      req.area.h = MIN( req.area.h, y2 - req.area.y );
      for( req.area.x = x1; req.area.x < x2;
	   req.area.x += req.area.width - ( req.area.x % req.area.width ) )
	{
	  req.area.w = req.area.width - ( req.area.x % req.area.width );
	  req.area.w = MIN( req.area.w, x2 - req.area.x );

	  req.num++;

	  lock_task();
	  gimp_pixel_rgn_get_rect(&src_rgn, req.data, 
				  req.area.x, req.area.y, req.area.w, req.area.h);
	  unlock_task();

	  ASYNC_LRPC((*distrib_func[pvals.distrib])(), TILE, STD_PRIO, DEFAULT_STACK, &req);
	}
    }

  total = req.num;
  while(req.num--) {
    marcel_sem_P(&sem_wait);
  }

  top2();

  regul_exit();

  printf("%d%% des threads ont réellement travaillé (%d/%d)\n", (int)((double)courageux/(double)total*100.0), courageux, total);

  printf("Temps = %ld.%03ldms\n", cpu_time()/1000, cpu_time()%1000);

  /*  update the pixelized region  */
  gimp_drawable_flush (drawable);
  gimp_drawable_merge_shadow (drawable->id, TRUE);
  gimp_drawable_update (drawable->id, x1, y1, (x2 - x1), (y2 - y1));
}

/*
  This function acts on one PixelArea.	Since there were so many
  nested FORs in do_pixelize(), I put a few of them here...
  */

#define moyenne(a,b,coef)        (((a) + (b) * (coef)) / (1 + (coef)))

static gint
pixelize_sub( gint pixelwidth, gint bpp, PixelArea area )
{
  glong average[4];		/* bpp <= 4 */
  gint	x, y, w, h;
  guchar *buf_row, *buf, last[4];
  gint	row, col;
  gint	rowstride;
  gint	count;
  gint	i;
  gint changed, heterogeneous;

  rowstride = area.w * bpp;

  changed = FALSE;

  for( y = area.y; y < area.y + area.h; y += pixelwidth - ( y % pixelwidth ) )
    {
      h = pixelwidth - ( y % pixelwidth );
      h = MIN( h, area.y + area.h - y );
      for( x = area.x; x < area.x + area.w; x += pixelwidth - ( x % pixelwidth ) )
	{
	  w = pixelwidth - ( x % pixelwidth );
	  w = MIN( w, area.x + area.w - x );

	  do {

	  for( i = 0; i < bpp; i++ )
	    average[i] = 0;
	  count = 0;

	  /* Read */
	  buf_row = area.data + (y-area.y)*rowstride + (x-area.x)*bpp;

	  for( i = 0; i < bpp; i++ )
	    last[i] = buf_row[i];

	  heterogeneous = FALSE;

	  for( row = 0; row < h; row++ )
	    {
	      buf = buf_row;
	      for( col = 0; col < w; col++ )
		{
		  for( i = 0; i < bpp; i++ ) {
		    average[i] += buf[i];
		    if(buf[i] != last[i])
		      heterogeneous = TRUE;
		    last[i] = buf[i];
		  }
		  count++;
		  buf += bpp;
		}
	      buf_row += rowstride;
	    }

	  if(heterogeneous)
	    {

	      changed = TRUE;

	      { int j = 100000; while(--j) ; }
	    /* Average */
	    if ( count > 0 )
	      {
		for( i = 0; i < bpp; i++ )
		  average[i] /= count;
	      }

	    /* Write */
	    buf_row = area.data + (y-area.y)*rowstride + (x-area.x)*bpp;

	    for( row = 0; row < h; row++ )
	      {
		buf = buf_row;
		for( col = 0; col < w; col++ )
		  {
		    for( i = 0; i < bpp; i++ )
		      buf[i] = moyenne(buf[i],average[i],1);
		    count++;
		    buf += bpp;
		  }
		buf_row += rowstride;
	      }
	    }
	  } while(FALSE);
	}
    }
  return changed;
}

int pm2_main(int argc, char *argv[])
{ 
  if(getenv("MAD_MY_NUMBER") == NULL)
    return gimp_main (argc, argv);
  else {
    pm2_init_rpc();
    
    DECLARE_LRPC_WITH_NAME(TILE, "tile", DO_NOT_OPTIMIZE);

    regul_init_rpc();

    pm2_init(&argc, argv, ONE_PER_NODE, &les_modules, &nb_modules);

    regul_init(les_modules, nb_modules);

    pm2_exit();
  }
  return 0;
}

/*
  Gimp registration and dialog box
  */

static void
query()
{
  static GParamDef args[]=
    {
      { PARAM_INT32, "run_mode", "Interactive, non-interactive" },
      { PARAM_IMAGE, "image", "Input image (unused)" },
      { PARAM_DRAWABLE, "drawable", "Input drawable" },
      { PARAM_INT32, "pixelwidth", "Pixel width	 (the decrease in resolution)" },
      { PARAM_INT32, "nbprocs", "Nb of PM2 processes (0 for one per node)" },
      { PARAM_INT32, "distrib", "Threads distribution policy" }
   };
  static GParamDef *return_vals = NULL;
  static gint nargs = sizeof (args) / sizeof (args[0]);
  static gint nreturn_vals = 0;

  gimp_install_procedure ("plug_in_pm2-smooth-pixelize",
			  "Pixelize the contents of the specified drawable",
			  "Pixelize the contents of the specified drawable with speficied pixelizing width.",
			  "Spencer Kimball & Peter Mattis, Tracy Scott, Eiichi Takamori, parallelized on top of PM2 by Raymond Namyst",
			  "Spencer Kimball & Peter Mattis, Tracy Scott",
			  "1995",
			  "<Image>/Filters/Image/PM2/Pixelize Smoothly",
			  "RGB*, GRAY*",
			  PROC_PLUG_IN,
			  nargs, nreturn_vals,
			  args, return_vals);
}

static void
run (gchar   *name,
     gint    nparams,
     GParam  *param,
     gint    *nreturn_vals,
     GParam  **return_vals)
{
  static GParam values[1];
  GDrawable *drawable;
  GRunModeType run_mode;
  GStatusType status = STATUS_SUCCESS;

  run_mode = param[0].data.d_int32;

  *nreturn_vals = 1;
  *return_vals = values;

  values[0].type = PARAM_STATUS;
  values[0].data.d_status = status;

  /*  Get the specified drawable  */
  drawable = gimp_drawable_get (param[2].data.d_drawable);

  switch (run_mode)
    {
    case RUN_INTERACTIVE:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_pm2-smooth-pixelize", &pvals);

      /*  First acquire information with a dialog  */
      if (! pixelize_dialog ())
	{
	  gimp_drawable_detach (drawable);
	  return;
	}
      break;

    case RUN_NONINTERACTIVE:
      /*  Make sure all the arguments are there!  */
      if (nparams != 6)
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  pvals.pixelwidth = (gdouble) param[3].data.d_int32;
	}
      if ((status == STATUS_SUCCESS) &&
	  pvals.pixelwidth <= 0 )
	status = STATUS_CALLING_ERROR;
      if (status == STATUS_SUCCESS)
	{
	  pvals.nbprocs = (int) param[4].data.d_int32;
	}
      if ((status == STATUS_SUCCESS) &&
	  pvals.nbprocs <= 0 )
	status = STATUS_CALLING_ERROR;
      pvals.distrib = param[5].data.d_int32;
      break;

    case RUN_WITH_LAST_VALS:
      /*  Possibly retrieve data  */
      gimp_get_data ("plug_in_pm2-smooth-pixelize", &pvals);
      break;

    default:
      break;
    }

  if (status == STATUS_SUCCESS)
    {
      /*  Make sure that the drawable is gray or RGB color  */
      if (gimp_drawable_color (drawable->id) || gimp_drawable_gray (drawable->id))
	{
	  gimp_progress_init ("Pixelizing in parallel...");

	  /*  set the tile cache size  */
	  gimp_tile_cache_ntiles (TILE_CACHE_SIZE);

	  /*  run the pixelize effect  */
	  pixelize (drawable);

	  if (run_mode != RUN_NONINTERACTIVE)
	    gimp_displays_flush ();

	  /*  Store data  */
	  if (run_mode == RUN_INTERACTIVE)
	    gimp_set_data ("plug_in_pm2-smooth-pixelize", &pvals, sizeof (PixelizeValues));
	}
      else
	{
	  /* gimp_message ("pixelize: cannot operate on indexed color images"); */
	  status = STATUS_EXECUTION_ERROR;
	}
    }

  values[0].data.d_status = status;

  gimp_drawable_detach (drawable);
}


static gint
pixelize_dialog ()
{
  GtkWidget *dlg;
  GtkWidget *frame;
  GtkWidget *table;
  GtkWidget *button;
  GtkObject *scale_data;
  GSList *group = NULL;
  GtkWidget *toggle_vbox;
  GtkWidget *toggle;
  gchar **argv;
  gint	argc;
  gint distrib_cyclic = (pvals.distrib == DISTRIB_CYCLIC);
  gint distrib_random = (pvals.distrib == DISTRIB_RANDOM);
  gint distrib_on_first = (pvals.distrib == DISTRIB_ON_FIRST);

  argc = 1;
  argv = g_new (gchar *, 1);
  argv[0] = g_strdup ("pm2-pixelize");

  gtk_init (&argc, &argv);
  gtk_rc_parse (gimp_gtkrc ());


  dlg = gtk_dialog_new ();
  init = TRUE;
  gtk_window_set_title (GTK_WINDOW (dlg), "PM2 \"smooth\" Pixelize");
  gtk_window_position (GTK_WINDOW (dlg), GTK_WIN_POS_MOUSE);
  gtk_signal_connect (GTK_OBJECT (dlg), "destroy",
		      (GtkSignalFunc) pixelize_close_callback,
		      NULL);

  /*  Action area  */
  button = gtk_button_new_with_label ("OK");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect (GTK_OBJECT (button), "clicked",
		      (GtkSignalFunc) pixelize_ok_callback,
		      dlg);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_grab_default (button);
  gtk_widget_show (button);

  button = gtk_button_new_with_label ("Cancel");
  GTK_WIDGET_SET_FLAGS (button, GTK_CAN_DEFAULT);
  gtk_signal_connect_object (GTK_OBJECT (button), "clicked",
			     (GtkSignalFunc) gtk_widget_destroy,
			     GTK_OBJECT (dlg));
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->action_area), button, TRUE, TRUE, 0);
  gtk_widget_show (button);

  /*  parameter settings  */
  frame = gtk_frame_new ("Filter Settings");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  entscale_int_new (table, 0, 0, "Pixel Width:", &pvals.pixelwidth,
		    2, gimp_tile_width(), TRUE,
		    NULL, NULL);
  gtk_widget_show (table);
  gtk_widget_show (frame);

  /**
  ***	This code is stolen from gimp-0.99.x/app/blend.c
  **/

  /*  PM2 settings  */
  frame = gtk_frame_new ("PM2 Configuration");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  table = gtk_table_new (3, 2, FALSE);
  gtk_container_border_width (GTK_CONTAINER (table), 10);
  gtk_container_add (GTK_CONTAINER (frame), table);

  button = gtk_check_button_new_with_label ("One proc per node");
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (button), (pvals.nbprocs == 0) );
  gtk_signal_connect (GTK_OBJECT (button), "toggled",
		      (GtkSignalFunc) pixelize_onepernode_callback,
		      &pvals.nbprocs );
  gtk_table_attach (GTK_TABLE(table), button, 0, 2, 0, 1,
		    GTK_FILL, 0, 0, 0);
  gtk_widget_show (button);
  gtk_widget_show (table);

  proc_label = gtk_label_new ("Nb of processes:");
  gtk_misc_set_alignment (GTK_MISC (proc_label), 0.0, 1.0);
  gtk_table_attach (GTK_TABLE (table), proc_label, 0, 1, 1, 2,
		    GTK_SHRINK | GTK_FILL, GTK_SHRINK | GTK_FILL, 0, 2);
  gtk_widget_set_sensitive (proc_label, (pvals.nbprocs != 0));
  gtk_widget_show (proc_label);

  scale_data = gtk_adjustment_new (pvals.nbprocs,
				   1.0, 9.0, 1.0, 1.0, 1.0);
  proc_scale = gtk_hscale_new (GTK_ADJUSTMENT (scale_data));
  gtk_scale_set_digits (GTK_SCALE (proc_scale), 0);
  gtk_scale_set_value_pos (GTK_SCALE (proc_scale), GTK_POS_TOP);
  gtk_signal_connect (GTK_OBJECT (scale_data), "value_changed",
		      (GtkSignalFunc) pixelize_scale_callback,
		      &pvals.nbprocs);
  gtk_table_attach (GTK_TABLE (table), proc_scale, 1, 2, 1, 2,
		    GTK_EXPAND | GTK_SHRINK | GTK_FILL, GTK_SHRINK, 4, 2);
  gtk_widget_set_sensitive (proc_scale, (pvals.nbprocs != 0));
  gtk_widget_show (proc_scale);
  gtk_widget_show (table);
  gtk_widget_show (frame);

  frame = gtk_frame_new ("Threads initial distribution");
  gtk_frame_set_shadow_type (GTK_FRAME (frame), GTK_SHADOW_ETCHED_IN);
  gtk_container_border_width (GTK_CONTAINER (frame), 10);
  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dlg)->vbox), frame, TRUE, TRUE, 0);

  toggle_vbox = gtk_vbox_new (FALSE, 5);
  gtk_container_border_width (GTK_CONTAINER (toggle_vbox), 5);
  gtk_container_add (GTK_CONTAINER (frame), toggle_vbox);

  toggle = gtk_radio_button_new_with_label (group, "Randomized");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) pixelize_distrib_callback,
		      &distrib_random);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), distrib_random);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "Cyclic");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) pixelize_distrib_callback,
		      &distrib_cyclic);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), distrib_cyclic);
  gtk_widget_show (toggle);

  toggle = gtk_radio_button_new_with_label (group, "Only on first node");
  group = gtk_radio_button_group (GTK_RADIO_BUTTON (toggle));
  gtk_box_pack_start (GTK_BOX (toggle_vbox), toggle, FALSE, FALSE, 0);
  gtk_signal_connect (GTK_OBJECT (toggle), "toggled",
		      (GtkSignalFunc) pixelize_distrib_callback,
		      &distrib_on_first);
  gtk_toggle_button_set_state (GTK_TOGGLE_BUTTON (toggle), distrib_on_first);
  gtk_widget_show (toggle);

  gtk_widget_show (toggle_vbox);
  gtk_widget_show (frame);

  gtk_widget_show (dlg);

  init = FALSE;

  gtk_main ();
  gdk_flush ();

  if(distrib_random)
    pvals.distrib = DISTRIB_RANDOM;
  else if(distrib_cyclic)
    pvals.distrib = DISTRIB_CYCLIC;
  else
    pvals.distrib = DISTRIB_ON_FIRST;

  return pint.run;
}

/*  Pixelize interface functions  */

static void
pixelize_close_callback (GtkWidget *widget,
			 gpointer   data)
{
  gtk_main_quit ();
}

static void
pixelize_ok_callback (GtkWidget *widget,
		      gpointer	 data)
{
  pint.run = TRUE;
  gtk_widget_destroy (GTK_WIDGET (data));
}

static void
pixelize_onepernode_callback(GtkWidget *widget,
			     gpointer data)
{
  gint			*toggle_val = (gint *) data;
  static gint backup = 1;

  if (GTK_TOGGLE_BUTTON (widget)->active) {
    backup = *toggle_val;
    *toggle_val = 0;
  } else {
    *toggle_val = backup;
  }

  if(!init) {
    gtk_widget_set_sensitive (proc_label, (*toggle_val != 0));
    gtk_widget_set_sensitive (proc_scale, (*toggle_val != 0));
  }
}

static void
pixelize_scale_callback (GtkAdjustment *adjustment, gpointer data)
{
  if(!init)
    *(gint*) data = (gint) adjustment->value;
}

static void
pixelize_distrib_callback (GtkWidget *widget,
			   gpointer   data)
{
  int *toggle_val;

  toggle_val = (int *) data;

  if (GTK_TOGGLE_BUTTON (widget)->active)
    *toggle_val = TRUE;
  else
    *toggle_val = FALSE;
}

/* ====================================================================== */

/*
  Entry and Scale pair 1.03

  TODO:
  - Do the proper thing when the user changes value in entry,
  so that callback should not be called when value is actually not changed.
  - Update delay
 */

/*
 *  entscale: create new entscale with label. (int)
 *  1 row and 2 cols of table are needed.
 *  Input:
 *    x, y:       starting row and col in table
 *    caption:    label string
 *    intvar:     pointer to variable
 *    min, max:   the boundary of scale
 *    constraint: (bool) true iff the value of *intvar should be constraint
 *                by min and max
 *    callback:	  called when the value is actually changed
 *    call_data:  data for callback func
 */
void
entscale_int_new ( GtkWidget *table, gint x, gint y,
		   gchar *caption, gint *intvar,
		   gint min, gint max, gint constraint,
		   EntscaleIntCallbackFunc callback,
		   gpointer call_data)
{
  EntscaleIntData *userdata;
  GtkWidget *hbox;
  GtkWidget *label;
  GtkWidget *entry;
  GtkWidget *scale;
  GtkObject *adjustment;
  gchar    buffer[256];
  gint	    constraint_val;

  userdata = g_new ( EntscaleIntData, 1 );

  label = gtk_label_new (caption);
  gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);

  /*
    If the first arg of gtk_adjustment_new() isn't between min and
    max, it is automatically corrected by gtk later with
    "value_changed" signal. I don't like this, since I want to leave
    *intvar untouched when `constraint' is false.
    The lines below might look oppositely, but this is OK.
   */
  userdata->constraint = constraint;
  if( constraint )
    constraint_val = *intvar;
  else
    constraint_val = ( *intvar < min ? min : *intvar > max ? max : *intvar );

  userdata->adjustment = adjustment = 
    gtk_adjustment_new ( constraint_val, min, max, 1.0, 1.0, 0.0);
  scale = gtk_hscale_new ( GTK_ADJUSTMENT(adjustment) );
  gtk_widget_set_usize (scale, ENTSCALE_INT_SCALE_WIDTH, 0);
  gtk_scale_set_draw_value (GTK_SCALE (scale), FALSE);

  userdata->entry = entry = gtk_entry_new ();
  gtk_widget_set_usize (entry, ENTSCALE_INT_ENTRY_WIDTH, 0);
  sprintf( buffer, "%d", *intvar );
  gtk_entry_set_text( GTK_ENTRY (entry), buffer );

  userdata->callback = callback;
  userdata->call_data = call_data;

  /* userdata is done */
  gtk_object_set_user_data (GTK_OBJECT(adjustment), userdata);
  gtk_object_set_user_data (GTK_OBJECT(entry), userdata);

  /* now ready for signals */
  gtk_signal_connect (GTK_OBJECT (entry), "changed",
		      (GtkSignalFunc) entscale_int_entry_update,
		      intvar);
  gtk_signal_connect (GTK_OBJECT (adjustment), "value_changed",
		      (GtkSignalFunc) entscale_int_scale_update,
		      intvar);
  gtk_signal_connect (GTK_OBJECT (entry), "destroy",
		      (GtkSignalFunc) entscale_int_destroy_callback,
		      userdata );

  /* start packing */
  hbox = gtk_hbox_new (FALSE, 5);
  gtk_box_pack_start (GTK_BOX (hbox), scale, TRUE, TRUE, 0);
  gtk_box_pack_start (GTK_BOX (hbox), entry, FALSE, TRUE, 0);

  gtk_table_attach (GTK_TABLE (table), label, x, x+1, y, y+1,
		    GTK_FILL, GTK_FILL, 0, 0);
  gtk_table_attach (GTK_TABLE (table), hbox, x+1, x+2, y, y+1,
		    GTK_EXPAND | GTK_FILL, GTK_FILL, 0, 0);

  gtk_widget_show (label);
  gtk_widget_show (entry);
  gtk_widget_show (scale);
  gtk_widget_show (hbox);
}  


/* when destroyed, userdata is destroyed too */
static void
entscale_int_destroy_callback (GtkWidget *widget,
			       gpointer data)
{
  EntscaleIntData *userdata;

  userdata = data;
  g_free ( userdata );
}

static void
entscale_int_scale_update (GtkAdjustment *adjustment,
			   gpointer      data)
{
  EntscaleIntData *userdata;
  GtkEntry	*entry;
  gchar		buffer[256];
  gint		*intvar = data;
  gint		new_val;

  userdata = gtk_object_get_user_data (GTK_OBJECT (adjustment));

  new_val = (gint) adjustment->value;

  *intvar = new_val;

  entry = GTK_ENTRY( userdata->entry );
  sprintf (buffer, "%d", (int) new_val );
  
  /* avoid infinite loop (scale, entry, scale, entry ...) */
  gtk_signal_handler_block_by_data ( GTK_OBJECT(entry), data );
  gtk_entry_set_text ( entry, buffer);
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(entry), data );

  if (userdata->callback)
    (*userdata->callback) (*intvar, userdata->call_data);
}

static void
entscale_int_entry_update (GtkWidget *widget,
			   gpointer   data)
{
  EntscaleIntData *userdata;
  GtkAdjustment	*adjustment;
  int		new_val, constraint_val;
  int		*intvar = data;

  userdata = gtk_object_get_user_data (GTK_OBJECT (widget));
  adjustment = GTK_ADJUSTMENT( userdata->adjustment );

  new_val = atoi (gtk_entry_get_text (GTK_ENTRY (widget)));
  constraint_val = new_val;
  if ( constraint_val < adjustment->lower )
    constraint_val = adjustment->lower;
  if ( constraint_val > adjustment->upper )
    constraint_val = adjustment->upper;

  if ( userdata->constraint )
    *intvar = constraint_val;
  else
    *intvar = new_val;

  adjustment->value = constraint_val;
  gtk_signal_handler_block_by_data ( GTK_OBJECT(adjustment), data );
  gtk_signal_emit_by_name ( GTK_OBJECT(adjustment), "value_changed");
  gtk_signal_handler_unblock_by_data ( GTK_OBJECT(adjustment), data );
  
  if (userdata->callback)
    (*userdata->callback) (*intvar, userdata->call_data);
}
