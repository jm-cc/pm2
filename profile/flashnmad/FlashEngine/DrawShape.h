/*
** DrawShape.h
** 
** Made by  Dubois Jonathan Julien
** Login   <jdubois@camelat.emi.u-bordeaux1.fr>
** 
** Started on  Thu Nov  8 11:32:34 2007  Dubois Jonathan Julien
** Last update Mon Dec  3 15:59:54 2007  Dubois Jonathan Julien
*/

#ifndef   	DRAWSHAPE_H_
# define   	DRAWSHAPE_H_
#include<ming.h>
#include<math.h>
	
/*Define pricipaly used by the SWFMovie*/
#define WIDTH 6000
#define HEIGHT 4000

	
#define PEN_WIDTH 1
#define PACKET_WIDTH 200
#define PACKET_HEIGHT 100

#define MAX(x,y) (((x) > (y)) ? (x) : (y))

SWFButton createButton(SWFShape shape,SWFAction action,int flags);

SWFMovieClip drawTimer(int size);

SWFShape drawPause(float width,float heigth);

SWFShape drawArrow(float width, float height, int right);

SWFShape drawPacket(int size);

SWFShape drawNetworkCard(int nb_pretracks,int nb_posttracks,int nb_packet);

SWFShape drawRect(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a);

SWFShape drawPacketList(int width,int height);

SWFShape drawMachine(int width,int height);

SWFShape drawControlPacket();

SWFShape drawWaitAckPacket();


#endif 	    /* !DRAWSHAPE_H_ */
