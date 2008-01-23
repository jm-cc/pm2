#ifndef   	ANIM_H_
# define   	ANIM_H_

#include <ming.h>
#include "DrawShape.h"
#include<stdlib.h>

typedef struct _pos{
  int x;
  int y;
} SWFpos;

typedef struct _object{
  SWFDisplayItem item;
  SWFShape shape;
  SWFpos pos;
}SWFobject;

typedef struct _packet{
  SWFobject obj;
  int size;
}SWFpacket;

typedef struct _packetlist{
  SWFobject obj;
  SWFpos vector; /* vector + obj.pos = position of end */
  int size; /* length of the list representation */
  int npackets; /* number of packets actually in the list */
}SWFpacketlist;

typedef struct _nic{
  SWFDisplayItem item;
  SWFShape shape;
  int x;
  int y;
  char *url;
}SWFnic;

typedef struct _gate{
  SWFDisplayItem item;
  SWFShape shape;
  int x;
  int y;
}SWFgate;

typedef struct _networkcard{
  SWFDisplayItem item;
  SWFShape shape;
  int x;
  int y;
  int buffer;
}SWFNetworkCard;

void movePacket(SWFpacket *p,int x,int y);

void movePacketTo(SWFMovie movie, SWFpacket *p,int x,int y);

void movePacketsTo(SWFMovie movie, SWFpacket **packets, SWFpos *dests, unsigned int npackets, unsigned int nframes);

#endif 	    /* !ANIM_H_ */
