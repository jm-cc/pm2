/*
** Anim.c
** 
** Made by ( Dubois Jonathan Julien)
** Login   <jdubois@napoljtano>
** 
** Started on  Wed Nov 21 10:41:33 2007  Dubois Jonathan Julien
** Last update Wed Nov 28 11:27:21 2007  Dubois Jonathan Julien
*/

#include "Anim.h"

#define RATE 24



void movePacket(SWFpacket *p, int x, int y)
{
  SWFDisplayItem_moveTo(p->obj.item,x,y);
  
  p->obj.pos.x=x;
  p->obj.pos.y=y;
}

/*
 * collectively move an array of packets, specifying their respective 
 * destinations, and the number of frames devoted to that move 
 */
void movePacketsTo(SWFMovie movie, SWFpacket **packets, SWFpos *dests, unsigned int npackets, unsigned int nframes)
{
  int frame, packet;
  int remainingframes = nframes;

  if (npackets == 0) return;
  
  for (frame = 0 ; frame < nframes ; frame++, remainingframes--) 
  {
  	for (packet = 0; packet < npackets ; packet++) 
	{
		int x = packets[packet]->obj.pos.x;
		int y = packets[packet]->obj.pos.y;
		int dx = (dests[packet].x - x)/remainingframes;
		int dy = (dests[packet].y - y)/remainingframes;
		movePacket(packets[packet], x+dx, y+dy);
	}
	SWFMovie_nextFrame(movie);
  }
}

