#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include "pm2_common.h"


#define SUB_CHANNELS 4
#define NUM_MESSAGES 16
#define MSG_LENGTH   11
static int master = 0;
static int slave  = 1;
static marcel_t thread[SUB_CHANNELS];
static char buffer1[MSG_LENGTH] = "Hello foo!";
static char buffer2[MSG_LENGTH] = "Hello goo!";

static marcel_cond_t condition;        
static marcel_mutex_t barrier;         
volatile int nbthreads = 0;

#define BARRIER() { marcel_mutex_lock(&barrier); \
                    if(++nbthreads < SUB_CHANNELS) \
                       marcel_cond_wait(&condition,&barrier); \
                    else \
                       marcel_cond_broadcast(&condition); \
                    marcel_mutex_unlock(&barrier);}


void send_on_channel(p_mad_channel_t channel)
{
  p_mad_connection_t  out    = NULL;
  char               *buf    = NULL; 
  int                 number = 0;
  int                 index  = 0;
  
  marcel_detach(marcel_self());
  BARRIER();
  
  while(index++ < SUB_CHANNELS)
    {
      if (thread[index] == marcel_self())
	{
	  number = index;
	  break;
	}
    }
  
  if((number%2) == 0)
    buf = buffer1; 
  else
    buf = buffer2;
  
  out = mad_begin_packing(channel, master);  
  for(index = 0 ; index < NUM_MESSAGES ; index ++)
    {
      mad_pack(out, buf, MSG_LENGTH, mad_send_CHEAPER, mad_receive_CHEAPER);
      marcel_printf("Thread number %i %p sent message\n",number,  marcel_self());
      marcel_yield();
    }
  mad_end_packing(out);  
  
}

void receive_on_channel(p_mad_channel_t channel)
{
  p_mad_connection_t  in     = NULL;
  char                buf[MSG_LENGTH] ;
  int                 index  = 0;

  marcel_detach(marcel_self());
  BARRIER();

  in = mad_begin_unpacking(channel);  
  for(index = 0 ; index < NUM_MESSAGES ; index ++)
    {
      mad_unpack(in, buf, MSG_LENGTH, mad_send_CHEAPER, mad_receive_CHEAPER);
      marcel_printf("Received message : %s\n",buf);
      marcel_yield();
    }
  mad_end_unpacking(in);  
}

/* Warning: this function is automatically renamed to 
   marcel_main when appropriate */
int main(int argc, char **argv)
{
  p_mad_madeleine_t   madeleine      = NULL;
  p_mad_session_t     session        = NULL;
  p_tbx_slist_t       slist          = NULL;
  p_mad_channel_t     orig_channel   = NULL;
  p_mad_channel_t     sub_channel[SUB_CHANNELS];
  char                *name          = NULL;
  int                 process_rank;
  int                 index          = 0;  
  

  common_pre_init(&argc, argv, NULL);
  common_post_init(&argc, argv, NULL);
  TRACE("Returned from common_init");

  marcel_mutex_init(&barrier, NULL); 
  marcel_cond_init(&condition, NULL); 
  
  madeleine    = mad_get_madeleine();
  session      = madeleine->session;
  process_rank = session->process_rank;
  DISP_VAL("My global rank is",process_rank);
  DISP_VAL("The configuration size is",
	   tbx_slist_get_length(madeleine->dir->process_slist));
  
  DISP("----------");


  slist        = madeleine->public_channel_slist;
  tbx_slist_ref_to_head(slist);
  name         = tbx_slist_ref_get(slist); 
  orig_channel = tbx_htable_get(madeleine->channel_htable, name);
  
  if (!orig_channel)
    {
      DISP("I don't belong to this channel");
      return 0;
    }
  
  for(index=0 ; index < SUB_CHANNELS ; index ++)
    sub_channel[index] = mad_get_sub_channel(orig_channel, index);
  
  if(process_rank == master)
    for(index=0 ; index < SUB_CHANNELS ; index ++)
      marcel_create (&thread[index], NULL, 
		     (marcel_func_t )receive_on_channel,sub_channel[index]);
  else
    for(index=0 ; index < SUB_CHANNELS ; index ++)
      marcel_create (&thread[index], NULL, 
		     (marcel_func_t) send_on_channel,sub_channel[index]);
  
  /* for(index=0 ; index < SUB_CHANNELS ; index ++) */
  /*     marcel_join(thread[index],NULL); */
  
  mad_leonie_barrier();
  DISP("----------");
  DISP("Exiting");
  
  common_exit(NULL);
  
  return 0;
}











