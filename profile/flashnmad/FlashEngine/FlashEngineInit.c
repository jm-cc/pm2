#include <stdio.h>
#include "FlashEngineInit.h"

static void initialize_machine(nmad_state_t *nmad_status)
{
  int sep= WIDTH/2;

  addTimeControl(&(nmad_status->movie),0,0,600,200);

  SWFShape separator=drawRect(10,HEIGHT,0xaa,0xaa,0xaa,0xff);

  SWFDisplayItem item;

  item=SWFMovie_add(nmad_status->movie,(SWFBlock)separator);

  SWFDisplayItem_moveTo(item,sep,500);
  
  initialise_machine_newlist(nmad_status, 0, 1200);

  initialise_gates(nmad_status,800,300);

  initialise_drivers(nmad_status,sep+100,300);

  

}

void add_text(nmad_state_t *nmad_status, int x, int y, int size, char *str) 
{
	SWFFont font;

  	font = newSWFFont_fromFile("./FlashEngine/Fonts/test.ttf");
  	if (font == NULL) {
		perror("could not open font :");
		exit(-1);
	}

        SWFText text = newSWFText();
        SWFText_setFont(text, font);
        SWFText_setColor(text, 0, 0, 0, 0xff);
        SWFText_setHeight(text, size);
        SWFText_moveTo(text, x, y);
        SWFText_addString(text, str, NULL);

	SWFMovie_add(nmad_status->movie, text);
}

void flash_engine_init(nmad_state_t *nmad_status)
{
  Ming_init();
	
  Ming_setScale(1.0);

  nmad_status->movie = newSWFMovie();

  SWFMovie_setBackground(nmad_status->movie, 0xaa, 0xff, 0xff); 
  SWFMovie_setDimension(nmad_status->movie, WIDTH, HEIGHT);

  initialize_machine(nmad_status);

}

/* 
 * ok this is not the init ..... i know ;)
 */
void flash_engine_generate_output(nmad_state_t *nmad_status, char *path)
{
  SWFMovie_save(nmad_status->movie, path);
}


/*Add time control commands*/
void addTimeControl(SWFMovie* movie,int x,int y,int width,int height)
{

  int size=width/5;

  SWFDisplayItem item=SWFMovie_add(*movie,(SWFBlock)createButton(drawArrow(size,height,1),newSWFAction("if(!stopped){stop();box.stop();stopped=1;}nextFrame();"),SWFBUTTON_DOWN));
  SWFDisplayItem_moveTo(item,x+2*size+size/2,y);
  
  item=SWFMovie_add(*movie,(SWFBlock)createButton(drawPause(size/2,height),newSWFAction("if(stopped){play();box.play();stopped=0;}else{stop();stopped=1;box.stop();}"), SWFBUTTON_DOWN));
  SWFDisplayItem_moveTo(item,x+size+size/2,y);
  
  item=SWFMovie_add(*movie,(SWFBlock)createButton(drawArrow(size,height,0),newSWFAction("if(!stopped){stop();box.stop();stopped=1;}prevFrame();"),SWFBUTTON_DOWN));
  
  SWFDisplayItem_moveTo(item,x,y);
  		
  item = SWFMovie_add(*movie, (SWFBlock)drawTimer(size));
  SWFDisplayItem_moveTo(item, x+4*size+size/2,height/2);
  SWFDisplayItem_setName(item,"box");
}


void initialise_machine_newlist(nmad_state_t *nmad_status, int xpos, int ypos)
{
  nmad_status->newpackets_repr.obj.shape=drawPacketList(3*PACKET_WIDTH,PACKET_HEIGHT);
  nmad_status->newpackets_repr.vector.x = 2*PACKET_WIDTH;

  nmad_status->newpackets_repr.obj.item = 
    SWFMovie_add(nmad_status->movie, (SWFBlock)nmad_status->newpackets_repr.obj.shape);

  SWFDisplayItem_moveTo(nmad_status->newpackets_repr.obj.item, xpos, ypos);

  nmad_status->newpackets_repr.obj.pos.x = xpos;
  nmad_status->newpackets_repr.obj.pos.y = ypos;
}


void initialise_gates(nmad_state_t *nmad_status,int xpos,int ypos)
{
  int i;
  
  /*Space for timer*/

  /*Var needed for compute height*/  
  int begingate=ypos;
  int height=ypos;

  int machineheight=0;
  int machinewidth=100;

  add_text(nmad_status, xpos + 3*PACKET_WIDTH,  ypos - 2*PACKET_HEIGHT, PACKET_HEIGHT, "Gate(s)");

  for(i=0;i<nmad_status->ngates;++i){
    int j;
    printf("input list %d\n",nmad_status->gates[i].ninputlists);
   

    /*If there is more than one gate we draw a separator. Need some
      correction.*/
    
    if(i!=0){
      SWFDisplayItem item=SWFMovie_add(nmad_status->movie,(SWFBlock)drawRect(2*(3*PACKET_WIDTH)+machinewidth,10,40,40,40,0xFF));
      SWFDisplayItem_moveTo(item,xpos,begingate);
      begingate+=20;
      height=begingate;
    }
    

    /*Draw input lists */
    for(j=0;j<nmad_status->gates[i].ninputlists;++j){

      nmad_status->gates[i].inputlists_repr[j].obj.shape=drawPacketList(3*PACKET_WIDTH,PACKET_HEIGHT);
      
      nmad_status->gates[i].inputlists_repr[j].vector.x = 2*PACKET_WIDTH;
      
      nmad_status->gates[i].inputlists_repr[j].obj.item=SWFMovie_add(nmad_status->movie,(SWFBlock)nmad_status->gates[i].inputlists_repr[j].obj.shape);
      
      SWFDisplayItem_moveTo(nmad_status->gates[i].inputlists_repr[j].obj.item,xpos,height);
      
      nmad_status->gates[i].inputlists_repr[j].obj.pos.x = xpos;
      
      nmad_status->gates[i].inputlists_repr[j].obj.pos.y = height;
      
      height+=PACKET_HEIGHT;
    }

    machineheight=height;

    height=begingate;

    /*Draw output lists */
    for(j=0;j<nmad_status->gates[i].noutputlists;++j){
      
      nmad_status->gates[i].outputlists_repr[j].obj.shape=drawPacketList(3*PACKET_WIDTH,PACKET_HEIGHT);
      
      nmad_status->gates[i].outputlists_repr[j].vector.x = 2*PACKET_WIDTH;
      
      nmad_status->gates[i].outputlists_repr[j].obj.item=SWFMovie_add(nmad_status->movie,(SWFBlock)nmad_status->gates[i].outputlists_repr[j].obj.shape);
      
      SWFDisplayItem_moveTo(nmad_status->gates[i].outputlists_repr[j].obj.item,xpos+3*PACKET_WIDTH+machinewidth,height);
    
      nmad_status->gates[i].outputlists_repr[j].obj.pos.x=xpos+3*PACKET_WIDTH+machinewidth,height;
      
      nmad_status->gates[i].outputlists_repr[j].obj.pos.y=height;
      
      height+=PACKET_HEIGHT;

    }

    /*Draw the separator */
    machineheight=MAX(machineheight,height);
    SWFDisplayItem item=SWFMovie_add(nmad_status->movie,(SWFBlock)drawMachine(machinewidth,height-begingate));
    SWFDisplayItem_moveTo(item,xpos+3*PACKET_WIDTH,begingate);

    begingate=height;    
  }
}

void initialise_drivers(nmad_state_t *nmad_status,int xpos,int ypos)
{
  int i, j;
  
  /*Space for timer*/
  int height=ypos;
  int driverheight = height;
  int nicwidth=100;  

  add_text(nmad_status, xpos + 3*PACKET_WIDTH,  ypos - 2*PACKET_HEIGHT, PACKET_HEIGHT, "NIC(s)");

  for(i=0;i<nmad_status->ndrivers;++i){
    /*Set the positions */
    nmad_status->drivers[i].repr.x=xpos;
    nmad_status->drivers[i].repr.y=driverheight;

    height = driverheight;

    /*Draw pre-tracks*/
    for(j=0;j<nmad_status->drivers[i].ntracks_pre;++j){
      nmad_status->drivers[i].pre_tracks_repr[j].obj.shape=drawPacketList(3*PACKET_WIDTH,PACKET_HEIGHT);
      nmad_status->drivers[i].pre_tracks_repr[j].vector.x = 2*PACKET_WIDTH;
      nmad_status->drivers[i].pre_tracks_repr[j].obj.item=SWFMovie_add(nmad_status->movie,(SWFBlock)nmad_status->drivers[i].pre_tracks_repr[j].obj.shape);
      
      SWFDisplayItem_moveTo(nmad_status->drivers[i].pre_tracks_repr[j].obj.item,xpos,height);
      nmad_status->drivers[i].pre_tracks_repr[j].obj.pos.x = xpos;
      nmad_status->drivers[i].pre_tracks_repr[j].obj.pos.y = height;
      height+=PACKET_HEIGHT;
    }


    height=driverheight;

    /*Draw post-tracks*/
    for(j=0;j<nmad_status->drivers[i].ntracks_post;++j){
      
      nmad_status->drivers[i].post_tracks_repr[j].obj.shape=drawPacketList(3*PACKET_WIDTH,PACKET_HEIGHT);
      nmad_status->drivers[i].post_tracks_repr[j].vector.x = 2*PACKET_WIDTH;
      nmad_status->drivers[i].post_tracks_repr[j].obj.item=SWFMovie_add(nmad_status->movie,(SWFBlock)nmad_status->drivers[i].post_tracks_repr[j].obj.shape);
      
      SWFDisplayItem_moveTo(nmad_status->drivers[i].post_tracks_repr[j].obj.item,xpos+3*PACKET_WIDTH+nicwidth,height);
      nmad_status->drivers[i].post_tracks_repr[j].obj.pos.x=xpos+3*PACKET_WIDTH+nicwidth,height;
      nmad_status->drivers[i].post_tracks_repr[j].obj.pos.y=height;
      
      height+=PACKET_HEIGHT;

    } 

    /*Create the drawing objects*/
    SWFShape shape = newSWFShape();

    int nicheight = MAX(nmad_status->drivers[i].ntracks_pre,nmad_status->drivers[i].ntracks_post)*(PACKET_HEIGHT);

    SWFShape_setLine(shape,PEN_WIDTH,0,0,0,255);

    SWFFillStyle style= SWFShape_addSolidFillStyle(shape,0,0x7F,0,255);
    SWFShape_setRightFillStyle(shape,style);

    SWFShape_movePenTo(shape,3*PACKET_WIDTH,0);
    SWFShape_drawLine(shape,nicwidth,0);
    SWFShape_drawLine(shape,0,nicheight);
    SWFShape_drawLine(shape,-nicwidth,0);
    SWFShape_drawLine(shape,0,-nicheight);

    add_text(nmad_status,  nmad_status->drivers[i].repr.x + 7*PACKET_WIDTH + nicwidth,  nmad_status->drivers[i].repr.y + 3*PACKET_HEIGHT/2 , PACKET_HEIGHT, nmad_status->drivers[i].driverurl);

    nmad_status->drivers[i].repr.shape = shape; 
    nmad_status->drivers[i].repr.item=SWFMovie_add(nmad_status->movie,(SWFBlock)nmad_status->drivers[i].repr.shape);

    SWFDisplayItem_moveTo(nmad_status->drivers[i].repr.item, nmad_status->drivers[i].repr.x , nmad_status->drivers[i].repr.y);

    driverheight+=MAX(nmad_status->drivers[i].ntracks_pre,nmad_status->drivers[i].ntracks_post)*(PACKET_HEIGHT+PACKET_HEIGHT/2);
    
  }
}


