
/*
Personnal comments from the author about the source code:
This code has been written with the following idea in mind: "It must work quickly." It is therefore not written in good style and definitely not an example to follow when writing C programs.
*/

#include "pm2.h"
#define NeedFunctionPrototypes 1
#include <X11/X.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>

#define XRESOLUTION 320
#define YRESOLUTION 200
#define EVENTSTRSIZE 1
#define FRAMEDELAY 40
#define RECTSIZE 20
#define BALLSIZE 2
#define LINEWIDTH 2
#define DISPLAY ":0.0"
#define MAXRECTAXIALSPEED 3
#define MAXBALLAXIALSPEED 5

dsm_mutex_t ball_lock;

BEGIN_DSM_DATA
int ball_owner = 0;
float ball_x = 3;
float ball_y = 3;
float ball_x_dir = 1.0;
float ball_y_dir = 1.0;
DSM_NEWPAGE
int toto1 = 0;
END_DSM_DATA

static unsigned cur_proc = 0;

static unsigned DISPLAY_RECT;
static unsigned BLACK_RECT;
static unsigned DISPLAY_BALL;
static unsigned BLACK_BALL;
static unsigned CREATE_RECT;

/****************************/
unsigned node2pix(unsigned node);
static void the_RECT(int where);
static void create_RECT(void);
static void display_RECT(void);
static void display_RECT_func(int param);
static void display_BALL(void);
static void display_BALL_func(int param);
static void black_RECT(void);
static void black_RECT_func(int param);
static void black_BALL(void);
static void black_BALL_func(int param);
static __inline__ int next_proc(void);
void init_X(Display **my_display_ptr_ptr, Window *the_root_window_ptr,Window *the_top_window_ptr);
void CreateNonSizableBlackIOWindow(int x, int y,int width,int height,Display *the_display_ptr,Window the_parent_window,Window *the_window_ptr);
/****************************/
Display *my_display_ptr;
Window my_root_window;
Window my_first_window;
XGCValues my_gc_values; 
GC my_GC;
XColor blue;
XColor red;
XColor green;
XColor yellow;
XColor black;
/****************************/

unsigned node2pix(unsigned node){
  switch(node){
  case 0:
    return green.pixel;
  case 1:
    return blue.pixel;
  case 2:
    return red.pixel;
  case 3:
    return yellow.pixel;
  default:
    return node2pix(node % 4);
  }
}


static void the_RECT(int where)
{
  int my_number = where / (XRESOLUTION * YRESOLUTION);
  float x = where % XRESOLUTION;
  float y = (where % (XRESOLUTION * YRESOLUTION)) / XRESOLUTION;
  int xint = (int) x;
  int ball_x_int;
  int yint = (int) y;
  int ball_y_int;
  int wherefrom = pm2_self();
  float xdirection, ydirection;
  

  srand(time(NULL));
  xdirection = (float) MAXRECTAXIALSPEED - (((float)( MAXRECTAXIALSPEED * 2 * rand()))/((float)(RAND_MAX)));
  ydirection =(float) MAXRECTAXIALSPEED - (((float)( MAXRECTAXIALSPEED * 2 * rand()))/((float)(RAND_MAX)));

  while(1){
    dsm_mutex_lock(&ball_lock);
    if(ball_owner == my_number){
      pm2_rawrpc_begin(0, BLACK_BALL, NULL);
      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &ball_x_int, 1);
      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &ball_y_int, 1);
      pm2_rawrpc_end();

      ball_x += ball_x_dir;
      ball_y += ball_y_dir;
      ball_x_int = xint + (int)ball_x;
      ball_y_int = yint + (int)ball_y;
      
      if((ball_x + BALLSIZE >= RECTSIZE - LINEWIDTH)||(ball_x <= 0 + LINEWIDTH))
	ball_x_dir = -ball_x_dir;
      if((ball_y + BALLSIZE >= RECTSIZE - LINEWIDTH)||(ball_y <= 0 + LINEWIDTH))
	ball_y_dir = -ball_y_dir;
      
      pm2_rawrpc_begin(0, DISPLAY_BALL, NULL);
      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &wherefrom, 1);
      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &ball_x_int, 1);
      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &ball_y_int, 1);
      pm2_rawrpc_end();
    }
    dsm_mutex_unlock(&ball_lock);
    
    xint = (int)x;
    yint = (int)y;
    
    pm2_rawrpc_begin(0, BLACK_RECT, NULL);
    pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &xint, 1);
    pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &yint, 1);
    pm2_rawrpc_end();
    
      
    x += xdirection;
    y += ydirection;
    xint = (int)x;
    yint = (int)y;
    
    if((x + RECTSIZE >= XRESOLUTION)||(x <= 0))
      xdirection = -xdirection;
    if((y + RECTSIZE >= YRESOLUTION)||(y <= 0))
      ydirection = -ydirection;
    
    pm2_rawrpc_begin(0, DISPLAY_RECT, NULL);
    pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &wherefrom, 1);
    pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &xint, 1);
    pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &yint, 1);
    pm2_rawrpc_end();
    
    marcel_delay(FRAMEDELAY);
  }
}

static void create_RECT(void)
{
  int x, y, no_thread, param;

  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &no_thread, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &x, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &y, 1);
  pm2_rawrpc_waitdata();
  param = (x + y * XRESOLUTION + no_thread * XRESOLUTION * YRESOLUTION); // Horrible hack!!!
  pm2_thread_create(the_RECT, (void *)param);
  return;
}



static void display_RECT(void)
{
  unsigned node;
  unsigned x;
  unsigned y;
  unsigned param;
  if(pm2_self() != 0){
    fprintf(stderr,"A big error occurred...\n");
    pm2_halt();
    exit(-1);
  }
  
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &node, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &x, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &y, 1);
  pm2_rawrpc_waitdata();
  
  param = (x + y * XRESOLUTION + node * XRESOLUTION * YRESOLUTION); // Horrible hack!!!
  pm2_thread_create(display_RECT_func, (void *)param);
  return;
}

static void display_RECT_func(int param)
{
  int node = param / (XRESOLUTION * YRESOLUTION);
  int x = param % XRESOLUTION;
  int y = (param % (XRESOLUTION * YRESOLUTION)) / XRESOLUTION;

  lock_task();
  my_gc_values.foreground = node2pix(node);
  XChangeGC( my_display_ptr, my_GC, GCForeground, &my_gc_values);
  XDrawRectangle(my_display_ptr, my_first_window, my_GC,x,y,RECTSIZE,RECTSIZE);
  unlock_task();

  XFlush(my_display_ptr);
  return;
}

static void display_BALL(void)
{
  unsigned node;
  unsigned x;
  unsigned y;
  unsigned param;
  if(pm2_self() != 0){
    fprintf(stderr,"A big error occurred...\n");
    pm2_halt();
    exit(-1);
  }
  
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &node, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &x, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &y, 1);
  pm2_rawrpc_waitdata();
  
  param = (x + y * XRESOLUTION + node * XRESOLUTION * YRESOLUTION); // Horrible hack!!!
  pm2_thread_create(display_BALL_func, (void *)param);
  return;
}

static void display_BALL_func(int param)
{
  int node = param / (XRESOLUTION * YRESOLUTION);
  int x = param % XRESOLUTION;
  int y = (param % (XRESOLUTION * YRESOLUTION)) / XRESOLUTION;

  lock_task();
  my_gc_values.foreground = node2pix(node);
  XChangeGC( my_display_ptr, my_GC, GCForeground, &my_gc_values);
  XDrawRectangle(my_display_ptr, my_first_window, my_GC,x,y,BALLSIZE,BALLSIZE);
  unlock_task();

  XFlush(my_display_ptr);
  return;
}



static void black_RECT(void)
{
  unsigned x;
  unsigned y;
  unsigned param;
  if(pm2_self() != 0){
    fprintf(stderr,"A big error occurred...\n");
    pm2_halt();
  }
  
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &x, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &y, 1);
  pm2_rawrpc_waitdata();
  
  param = (x + y * XRESOLUTION); // Horrible hack!!!
  pm2_thread_create(black_RECT_func, (void *)param);
  return;

}

static void black_RECT_func(int param)
{
  int x = param % XRESOLUTION;
  int y = param / XRESOLUTION;

  lock_task();
  my_gc_values.foreground = black.pixel;
  XChangeGC( my_display_ptr, my_GC, GCForeground, &my_gc_values);
  XDrawRectangle(my_display_ptr, my_first_window, my_GC,x,y,RECTSIZE,RECTSIZE);
  unlock_task();

  XFlush(my_display_ptr);
  return;
}


static void black_BALL(void)
{
  unsigned x;
  unsigned y;
  unsigned param;

  if(pm2_self() != 0){
    fprintf(stderr,"A big error occurred...\n");
    pm2_halt();
  }
  
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &x, 1);
  pm2_unpack_int(SEND_CHEAPER, RECV_CHEAPER, &y, 1);
  pm2_rawrpc_waitdata();
  
  param = (x + y * XRESOLUTION); // Horrible hack!!!
  pm2_thread_create(black_BALL_func, (void *)param);
  return;
}


static void black_BALL_func(int param)
{
  int x = param % XRESOLUTION;
  int y = param / XRESOLUTION;

  lock_task();
  my_gc_values.foreground = black.pixel;
  XChangeGC( my_display_ptr, my_GC, GCForeground, &my_gc_values);
  XDrawRectangle(my_display_ptr, my_first_window, my_GC,x,y,BALLSIZE,BALLSIZE);
  unlock_task();

  XFlush(my_display_ptr);
  return;
}



static __inline__ int next_proc(void)
{
  lock_task();

  do {
    cur_proc = (cur_proc+1) % pm2_config_size();
  } while(cur_proc == pm2_self());

  unlock_task();
  return cur_proc;
}


int pm2_main(int argc, char **argv)
{
  int x,y,n;
  char the_end[10];
  XEvent my_event;
  char EventString[EVENTSTRSIZE];
  int nb_threads_created = 0;

  srand(time(NULL));
  
  pm2_rawrpc_register(&DISPLAY_RECT, display_RECT);
  pm2_rawrpc_register(&BLACK_RECT, black_RECT);
  pm2_rawrpc_register(&DISPLAY_BALL, display_BALL);
  pm2_rawrpc_register(&BLACK_BALL, black_BALL);
  pm2_rawrpc_register(&CREATE_RECT, create_RECT);
  
  dsm_set_default_protocol(LI_HUDAK);
  pm2_set_dsm_page_distribution(DSM_CENTRALIZED, 0);
  dsm_mutex_init(&ball_lock, NULL);

  pm2_init(&argc, argv);
  
  if(pm2_self() == 0){
    
    fprintf(stderr, "There are  %d machines in this config...\n", pm2_config_size());
    init_X(&my_display_ptr,&my_root_window,&my_first_window);

    dsm_mutex_lock(&ball_lock);
    ball_x_dir =  (float) MAXBALLAXIALSPEED - (((float)(MAXBALLAXIALSPEED * 2 * rand()))/((float)(RAND_MAX)));
    ball_y_dir =(float) MAXBALLAXIALSPEED - (((float)(MAXBALLAXIALSPEED * 2 * rand()))/((float)(RAND_MAX)));
    dsm_mutex_unlock(&ball_lock);


    while(1){
      XWindowEvent(my_display_ptr,my_first_window,KeyPressMask,&my_event);
      fprintf(stderr,"Event\n");
      if (my_event.type == KeyPress)
	{
	  fprintf(stderr,"Key press event\n");
	  XLookupString((XKeyEvent *)&my_event, EventString, EVENTSTRSIZE, NULL, NULL);
	  fprintf(stderr,"%d\n",(int)EventString[0]);
	  switch ((int)EventString[0])
	    {
	    case 3:
	      pm2_halt();
	      exit(1);
	    case 113:
	      pm2_halt();
	      exit(1);
	    case 27:
	      pm2_halt();
	      exit(1);
	    case 99:
	      x = XRESOLUTION / 2;
	      y = YRESOLUTION / 2;
	      n = next_proc();
	      pm2_rawrpc_begin(n, CREATE_RECT, NULL);
	      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &nb_threads_created, 1);
	      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &x, 1);
	      pm2_pack_int(SEND_CHEAPER, RECV_CHEAPER, &y, 1);
	      pm2_rawrpc_end();
	      nb_threads_created++;
	      break;
	    case 98:
	      if(nb_threads_created == 0)
		break;
	      dsm_mutex_lock(&ball_lock);
	      ball_owner = (ball_owner + 1) % nb_threads_created;
	      dsm_mutex_unlock(&ball_lock);
	      break;
	    case 114:
	      dsm_mutex_lock(&ball_lock);
	      ball_x_dir =  (float) MAXBALLAXIALSPEED - (((float)(MAXBALLAXIALSPEED * 2 * rand()))/((float)(RAND_MAX)));
	      ball_y_dir = (float) MAXBALLAXIALSPEED - (((float)(MAXBALLAXIALSPEED * 2 * rand()))/((float)(RAND_MAX)));
	      dsm_mutex_unlock(&ball_lock);
	      break;
	    default:
	      break;
	    }
	}
    }
    fprintf(stderr,"clear?\n");
    scanf("%s",the_end);
    XClearWindow(my_display_ptr,my_first_window);
    XFlush(my_display_ptr);
    
    fprintf(stderr,"exit?\n");
    scanf("%s",the_end);
    XUnmapWindow(my_display_ptr,my_first_window);
    XDestroyWindow(my_display_ptr,my_first_window);
    XCloseDisplay(my_display_ptr);
    pm2_halt();
  }
  pm2_exit();
  return 0;
}


void init_X(Display **my_display_ptr_ptr, Window *the_root_window_ptr,Window *the_top_window_ptr)
{
  int my_screen;
  XColor foo;
  Colormap the_colormap;

  *my_display_ptr_ptr=XOpenDisplay(DISPLAY);
  if ((*my_display_ptr_ptr)==NULL) 
     {
       fprintf(stderr,"Could not open connection with XServer\n"); 
       pm2_halt(); 
     }
  *the_root_window_ptr=DefaultRootWindow(*my_display_ptr_ptr);
  my_screen=XDefaultScreen(*my_display_ptr_ptr);
  
 
  CreateNonSizableBlackIOWindow(200, 100, XRESOLUTION, YRESOLUTION, *my_display_ptr_ptr, *the_root_window_ptr, the_top_window_ptr);
  XMapRaised(*my_display_ptr_ptr,*the_top_window_ptr);
  fprintf(stderr,"Raised\n");
  XFlush(*my_display_ptr_ptr);

  my_gc_values.function = GXcopy;
  my_gc_values.line_width = LINEWIDTH;
  the_colormap = DefaultColormap(my_display_ptr,my_screen);
  XAllocNamedColor(my_display_ptr, the_colormap,"blue",&blue,&foo);
  XAllocNamedColor(my_display_ptr, the_colormap,"red",&red,&foo);
  XAllocNamedColor(my_display_ptr, the_colormap,"yellow",&yellow,&foo);
  XAllocNamedColor(my_display_ptr, the_colormap,"black",&black,&foo);
  XAllocNamedColor(my_display_ptr, the_colormap,"green",&green,&foo);
  my_GC=XCreateGC(my_display_ptr,my_first_window,(GCFunction|GCLineWidth),&my_gc_values);
}


void CreateNonSizableBlackIOWindow(int x, int y,int width,int height,Display *the_display_ptr,Window the_parent_window,Window *the_window_ptr) 
{
  XSetWindowAttributes my_attributes;
  XSizeHints my_sizehints;
  
  
  
  my_attributes.background_pixmap=None;
  my_attributes.background_pixel=BlackPixel(the_display_ptr,DefaultScreen(the_display_ptr));
  my_attributes.border_pixmap=None;
  my_attributes.border_pixel=WhitePixel(the_display_ptr,DefaultScreen(the_display_ptr));
  my_attributes.bit_gravity=ForgetGravity;
  my_attributes.win_gravity=NorthWestGravity;
  my_attributes.backing_store=Always;
  my_attributes.backing_planes=XAllPlanes();
  my_attributes.backing_pixel=0;
  my_attributes.save_under=False;
  my_attributes.event_mask=KeyPressMask;
  my_attributes.do_not_propagate_mask=KeyPressMask;
  my_attributes.override_redirect=False;
  my_attributes.colormap = 0;
  my_attributes.cursor=None;

  *the_window_ptr=XCreateWindow(the_display_ptr,the_parent_window,x,y,width,height,2,CopyFromParent,InputOutput,CopyFromParent,XAllPlanes(),&my_attributes);
 
  my_sizehints.width=width;
  my_sizehints.height=height;
  my_sizehints.min_width=width;
  my_sizehints.min_height=height;
  my_sizehints.max_width=width;
  my_sizehints.max_height=height;
  my_sizehints.flags=PSize|PMinSize|PMaxSize;
  
  XSetNormalHints(the_display_ptr,*the_window_ptr,&my_sizehints);
  XSetStandardProperties( the_display_ptr, *the_window_ptr, "DSMGrEvAnRt", 0, (Pixmap) 0, 0, 0, &my_sizehints);
}









