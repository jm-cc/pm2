#include<stdlib.h>
#include"DrawShape.h"



SWFButton createButton(SWFShape shape,SWFAction action,int flags)
{
  SWFButton button = newSWFButton();
  SWFButton_addCharacter(button, (void*)shape, SWFBUTTON_UP|SWFBUTTON_DOWN|SWFBUTTON_OVER|SWFBUTTON_HIT);
  SWFButton_addAction(button,action,flags);

  return button;
}



SWFMovieClip drawTimer(int size)
{
  SWFMovieClip clip=newSWFMovieClip();
  SWFShape barre=newSWFShape();
 
  SWFShape_setLine(barre,PEN_WIDTH,0,0,0,255);

  SWFShape_movePenTo(barre,0,0);
  SWFShape_drawLineTo(barre,size,0);
 
  SWFDisplayItem i=SWFMovieClip_add(clip,(SWFBlock)barre);

  SWFMovieClip_nextFrame(clip);
		
  int n;
  for(n=0; n<25; ++n)
    {
      SWFDisplayItem_rotate(i, -14);
      SWFMovieClip_nextFrame(clip);
    }
		
  return clip;
}



SWFShape drawPause(float width, float height)
{
  SWFShape shape = newSWFShape();
  SWFShape_setLine(shape,PEN_WIDTH,0,0,0,255);  

  SWFFillStyle style = SWFShape_addSolidFillStyle(shape,0xaa,0xff,0xff,0xff);
  SWFShape_setRightFillStyle(shape,style);

  SWFShape_movePenTo(shape,0,0);
  SWFShape_drawLineTo(shape,0,height);
  SWFShape_movePenTo(shape,width,0);
  SWFShape_drawLineTo(shape,width,height);

  return shape;
}	

/* Draw an arrow */
SWFShape drawArrow(float width, float height, int right) 
{
  SWFShape shape = newSWFShape();
  SWFFillStyle style = SWFShape_addSolidFillStyle(shape,0,0,0,255);
  SWFShape_setRightFillStyle(shape,style);
  if (right) {
    SWFShape_movePenTo(shape,0,0);
    SWFShape_drawLineTo(shape,width,height/2);
    SWFShape_drawLineTo(shape,0,height);
    SWFShape_drawLineTo(shape,0,0);
  } else {
    SWFShape_movePenTo(shape,width,0);
    SWFShape_drawLineTo(shape,width,height);
    SWFShape_drawLineTo(shape,0,height/2);
    SWFShape_drawLineTo(shape,width,0);
  }
  return shape;
}

void colordistrib(int *rgb, int size)
{
	int small[3];
		small[0] = 0x00;
		small[1] = 0xFF;
		small[2] = 0x00;

	int medium[3];
		medium[0] = 0xFF;
		medium[1] = 0xFF;
		medium[2] = 0x00;

	int large[3];
		large[0] = 0xFF;
		large[1] = 0x00;
		large[2] = 0x00;

	float factor = logf((float)size)/20;
	if (factor > 1.0) factor = 1.0;

	printf("size = %d factor %f\n", size, factor);

	if (factor < 0.5) {
		/* [0;0.5] -> [0;1] */
		factor *= 2.0;
		rgb[0] = (int) ((1-factor) * small[0] + factor*medium[0]);
		rgb[1] = (int) ((1-factor) * small[1] + factor*medium[1]);
		rgb[2] = (int) ((1-factor) * small[2] + factor*medium[2]);
	}
	else {
		/* [0.5;1] -> [0;1] */
		factor = 2*(factor - 0.5);
		rgb[0] = (int) ((1-factor) * medium[0] + factor*large[0]);
		rgb[1] = (int) ((1-factor) * medium[1] + factor*large[1]);
		rgb[2] = (int) ((1-factor) * medium[2] + factor*large[2]);
	}
}
	
SWFShape drawPacket(int size)
{		
  SWFShape shape = newSWFShape();
		
  SWFShape_setLine(shape,PEN_WIDTH,0,0,0,255);
		
  /*La couleur du paquet depend du logarithme de sa taille */
  int color[3];
  colordistrib(color, size);
  SWFFillStyle style= SWFShape_addSolidFillStyle(shape,color[0], color[1], color[2],255);
  SWFShape_setRightFillStyle(shape,style);
		
  SWFShape_movePenTo(shape,0,0);
  SWFShape_drawLine(shape,PACKET_WIDTH,0);
  SWFShape_drawLine(shape,0,PACKET_HEIGHT);
  SWFShape_drawLine(shape,-PACKET_WIDTH,0);
  SWFShape_drawLine(shape,0,-PACKET_HEIGHT);
	
  return shape;
}
	
SWFShape drawControlPacket()
{		
  SWFShape shape = newSWFShape();
		
  SWFShape_setLine(shape,PEN_WIDTH,0xFF,0,0,255);
  SWFShape_movePenTo(shape,0,0);
  SWFShape_drawLine(shape,PACKET_WIDTH,PACKET_HEIGHT);

  SWFShape_setLine(shape,PEN_WIDTH,0,0,0,255);
		
  SWFFillStyle style= SWFShape_addSolidFillStyle(shape,0xFF,0xFF,0xFF,255);
  SWFShape_setRightFillStyle(shape,style);

  SWFShape_movePenTo(shape,0,0);
  SWFShape_drawLine(shape,PACKET_WIDTH,0);
  SWFShape_drawLine(shape,0,PACKET_HEIGHT);
  SWFShape_drawLine(shape,-PACKET_WIDTH,0);
  SWFShape_drawLine(shape,0,-PACKET_HEIGHT);
		
  return shape;
}

SWFShape drawWaitAckPacket()
{		
  SWFShape shape = newSWFShape();
		  
  SWFShape_setLine(shape,PEN_WIDTH,0,0,0,255);
		
  SWFFillStyle style= SWFShape_addSolidFillStyle(shape,0xC9,0xC9,0xC9,255);
  SWFShape_setRightFillStyle(shape,style);

  SWFShape_movePenTo(shape,0,0);
  SWFShape_drawLine(shape,PACKET_WIDTH,0);
  SWFShape_drawLine(shape,0,PACKET_HEIGHT);
  SWFShape_drawLine(shape,-PACKET_WIDTH,0);
  SWFShape_drawLine(shape,0,-PACKET_HEIGHT);
		
  return shape;
}

SWFShape drawNetworkCard(int nb_pretracks,int nb_posttracks,int nb_packet)
{
  int size_card=100;

  int width=nb_packet*(PACKET_WIDTH+PACKET_WIDTH/2);
  SWFShape shape = newSWFShape();
	
  int height=MAX(nb_pretracks,nb_posttracks)*(PACKET_HEIGHT);
  
  SWFShape_setLine(shape,PEN_WIDTH,0,0,0,255);
  
  /*draw the card*/		
  
  SWFFillStyle style= SWFShape_addSolidFillStyle(shape,0,0x7F,0,255);
  SWFShape_setRightFillStyle(shape,style);
  
  SWFShape_movePenTo(shape,width,0);
  SWFShape_drawLine(shape,size_card,0);
  SWFShape_drawLine(shape,0,height);
  SWFShape_drawLine(shape,-size_card,0);
  SWFShape_drawLine(shape,0,-height);
		
  return shape;
}

	
SWFShape drawRect(int x, int y, unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
  SWFShape s = newSWFShape();
  SWFFill f =  SWFShape_addSolidFill(s, r, g, b, a);
  
  SWFShape_setRightFill(s, f);
  
  SWFShape_movePenTo(s, -x/2,-y/2);
  SWFShape_drawLineTo(s, x/2,-y/2);
  SWFShape_drawLineTo(s, x/2, y/2);
  SWFShape_drawLineTo(s, -x/2, y/2);
  SWFShape_drawLineTo(s, -x/2,-y/2);
    
  return s;
}	

SWFShape drawPacketList(int width,int height)
{
  SWFShape shape;
  shape=newSWFShape();

  SWFShape_setLine(shape,PEN_WIDTH,0,0,0,255);
  SWFFillStyle style= SWFShape_addSolidFillStyle(shape,0,0x7F,0,255);
  SWFShape_setRightFillStyle(shape,style);

  SWFShape_movePenTo(shape,0,0);
  SWFShape_drawLine(shape,width,0);
  SWFShape_movePenTo(shape,0,height);
  SWFShape_drawLine(shape,width,0);

  return shape;
}	

SWFShape drawMachine(int width,int height)
{
  SWFShape shape=newSWFShape();

  SWFShape_setLine(shape,PEN_WIDTH,0,0,0,255);

  SWFFillStyle style= SWFShape_addSolidFillStyle(shape,0xFF,0,0,255);
  SWFShape_setRightFillStyle(shape,style);

  SWFShape_movePenTo(shape,0,0);
  SWFShape_drawLine(shape,width,0);
  SWFShape_drawLine(shape,0,height);
  SWFShape_drawLine(shape,-width,0);
  SWFShape_drawLine(shape,0,-height);

  return shape;
}
