#include "Interface.h"

static unsigned listSize(nmad_packet_list_t list)
{
  nmad_packet_itor_t i;
  unsigned size = 0;

  /* scan the whole list for the (first) element that matches */
  for (i  = nmad_packet_list_begin(list);
       i != nmad_packet_list_end(list);
       i  = nmad_packet_list_next(i))
    {
      size++;
    }

  return size;
}

void updateList(SWFMovie movie, nmad_packet_list_t list,  SWFpacketlist *listrepr, 
		unsigned int nframes)
{
  int npackets;
  npackets = listSize(list);

  if (npackets == 0) return;

  /* allocate the corresponding arrays */
  SWFpacket *packets[npackets];
  SWFpos dests[npackets];

  /* now fill in the destination information */	
  nmad_packet_itor_t i;
  int packet = 0;
  for (i  = nmad_packet_list_begin(list);
       i != nmad_packet_list_end(list);
       i  = nmad_packet_list_next(i), packet++)
    {
      packets[packet] = &i->repr;	

      int dx = 0;
      int dy = 0;

      if (npackets > 1) {
	dx = (listrepr->vector.x * packet)/(npackets - 1);
	dy = (listrepr->vector.y * packet)/(npackets - 1);
      }

      dests[packet].x = listrepr->obj.pos.x + dx; 
      dests[packet].y = listrepr->obj.pos.y + dy; 
    }

  movePacketsTo(movie, packets, dests, npackets, nframes);
}

void initializePacket(SWFMovie movie, nmad_packet_t packet)
{
  SWFShape shape;

  /* first create the corresponding shape  */
  if(packet->is_a_control_packet==1)
    shape=drawControlPacket();
  else
    shape = drawPacket(packet->length);

   
  packet->repr.obj.shape = shape;

  packet->repr.obj.item = SWFMovie_add(movie, (SWFBlock)shape);

  /* place the packet somewhere : 
   * TODO this location might change if we are dealing with a 
   * 	    control packet or not ... */
  packet->repr.obj.pos.x = -200; /* we don't see it yet */
  packet->repr.obj.pos.y = HEIGHT/2;

  SWFDisplayItem_moveTo(packet->repr.obj.item, 
			packet->repr.obj.pos.x, packet->repr.obj.pos.y);
}

void destroyPacket(SWFMovie movie, nmad_packet_t packet)
{

  SWFpos dest[1];
  SWFpacket *packets[1];

  packets[0] = &packet->repr;

  dest[0].x = packet->repr.obj.pos.x + 1400;
  dest[0].y = packet->repr.obj.pos.y ;

  movePacketsTo(movie, packets, dest, 1, 2);
  SWFMovie_remove(movie,packet->repr.obj.item);

}

void splitPacket(SWFMovie movie, nmad_packet_t packet, nmad_packet_t new_packet, int nframes)
{

  /* we first create the new packet */

  SWFShape shape;

  /* first create the corresponding shape  */
  shape = drawPacket(new_packet->length);
  new_packet->repr.obj.shape = shape;


  SWFpacket *packets[2];
  SWFpos dests[2];

  /*Set new positions */
  packets[0] = &packet->repr;
  packets[1] = &new_packet->repr;

  dests[0].x = packet->repr.obj.pos.x; 
  dests[0].y = packet->repr.obj.pos.y - 300; 
  dests[1].y  = dests[0].y;

  /*Move packet*/
  movePacketsTo(movie, packets, dests, 1, nframes);
  new_packet->repr.obj.item = SWFMovie_add(movie, (SWFBlock)shape);

  /*Destroy old packet */
  SWFMovie_remove(movie,packet->repr.obj.item);

  packet->repr.obj.shape=drawPacket(packet->length);
  packet->repr.obj.item=SWFMovie_add(movie,(SWFBlock)packet->repr.obj.shape);
  SWFDisplayItem_moveTo(packet->repr.obj.item,packet->repr.obj.pos.x,packet->repr.obj.pos.y);

  /*Move new packets */
  new_packet->repr.obj.pos.x = dests[0].x;
  new_packet->repr.obj.pos.y = dests[0].y;

  dests[1].x  = dests[0].x - 250;
  dests[0].x += 250;

  movePacketsTo(movie, packets, dests, 2, nframes);




}

void agregPacket(SWFMovie movie, nmad_packet_t host_packet, nmad_packet_t packet, int nframes)
{

  SWFpacket *packets[2];
  SWFpos dests[2];

  int posy = host_packet->repr.obj.pos.y;

  packets[0] = &host_packet->repr;
  packets[1] = &packet->repr;

  dests[0].x = host_packet->repr.obj.pos.x; 
  dests[0].y = posy - 300; 
  dests[1].x = dests[0].x;
  dests[1].y = dests[0].y;
  
  movePacketsTo(movie, packets, dests, 2, nframes);

  /*Maybe we had to change the order of these 2 actions.*/

  /*Destroy the second packet*/ 
  SWFMovie_remove(movie,packet->repr.obj.item);

  /*Edit the first packet */
  host_packet->repr.obj.shape=drawPacket(host_packet->length+packet->length);

  SWFMovie_remove(movie,host_packet->repr.obj.item);

  host_packet->repr.obj.item=SWFMovie_add(movie,(SWFBlock)host_packet->repr.obj.shape);
  SWFDisplayItem_moveTo(host_packet->repr.obj.item,host_packet->repr.obj.pos.x,host_packet->repr.obj.pos.y);

  dests[0].y = posy; 
  movePacketsTo(movie, packets, dests, 1, nframes);

}

void ackPacket(SWFMovie movie, nmad_packet_t packet)
{
  int i,j;
  int nb_frames=5;
  int nb_flash=3;
 
  /*Flash the packet which is acked.It's ugly but it works...*/
  SWFMovie_remove(movie,packet->repr.obj.item);
  
  SWFDisplayItem old=SWFMovie_add(movie,(SWFBlock)drawWaitAckPacket());
  SWFDisplayItem_moveTo(old,packet->repr.obj.pos.x,packet->repr.obj.pos.y);
  SWFMovie_nextFrame(movie);
  

  packet->repr.obj.shape=drawPacket(packet->length);
  packet->repr.obj.item=SWFMovie_add(movie,(SWFBlock)packet->repr.obj.shape);
  
  SWFDisplayItem_moveTo(packet->repr.obj.item,+50000,packet->repr.obj.pos.y);
  SWFMovie_nextFrame(movie);

  
  for(j=0;j<nb_flash;++j){
    SWFDisplayItem_moveTo(packet->repr.obj.item,+50000,packet->repr.obj.pos.y);
    SWFMovie_nextFrame(movie);

    for(i=0;i<nb_frames;++i){      
      SWFMovie_nextFrame(movie);
    }
    
    SWFDisplayItem_moveTo(packet->repr.obj.item,packet->repr.obj.pos.x,packet->repr.obj.pos.y);
   
    for(i=0;i<nb_frames;++i){      
      SWFMovie_nextFrame(movie);
    } 
  }

  SWFMovie_remove(movie,old);
  SWFMovie_nextFrame(movie);
}


/*
 * This function is called in order to tell the flash engine to initialize the 
 * representation with the proper number of gates, of lists ... status is an
 * input and output value : we take piece of information such as 
 * status->ndrivers and preProcess will fill status->movie, or for instance 
 * status->drivers[0]->repr ...
 */
void preProcess(nmad_state_t *status)
{

}
