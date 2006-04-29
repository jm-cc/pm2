#ifndef REARANGEMENT_H
#define REARANGEMENT_H
#include <stdio.h>
#include "zone.h"
#include "math.h"

int Rearanger(zone *zone);

int Translater(zone * zone1, int x, int y);

int TesterPositionnerZone(int* tabX,int * tabY,
                          int largeurZoneP,
                          int Nplateau, int plateau,
                          int largeurSousZone);

int MAJTabPlateau(int* tabX, int *tabY,
                  int nPlateau, int plateau,
                  zone* SousZone,
                  zone* ZonePrincipale,
                  int Y);


#endif
