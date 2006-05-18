#include "rearangement.h"

void PrintZone(zone* zonePrincipale);
void printTab(int* tab, int taille);

/* fonction de test */
/* int main() */
/* { */
/*    zone* zone1 = CreerZone(0,0,2,1); */

/*    zone* zone2 = CreerZone(0,0,50,50); */
/*    zone* zone3 = CreerZone(0,0,50,50); */
/*    zone* zone4 = CreerZone(0,0,50,50); */
/*    zone* zone5 = CreerZone(0,0,50,50); */
/*    zone* zone6 = CreerZone(0,0,50,50); */
/*    zone* zone7 = CreerZone(0,0,50,50); */
/*    zone* zone8 = CreerZone(0,0,50,50); */

/*    AjouterSousZones(zone1, zone2); */
/*    AjouterSousZones(zone1, zone3); */
/*    AjouterSousZones(zone1, zone4); */
/*    AjouterSousZones(zone1, zone5); */
/*    AjouterSousZones(zone3, zone6); */
/*    AjouterSousZones(zone4, zone7); */
/*    AjouterSousZones(zone5, zone8); */

/*    Rearanger(zone1); */

/*    PrintZone(zone1); */

/*    return 0; */
/* } */


int Rearanger(zone * zone1)
{
   int nElement;
   int j,k;
   int nPlateau;
   int surface = 0;
   int largeur = 0, largeurMax = 0;
   int* tabX;
   int* tabY;
   int minY;
   int minY2;
   int nMin;

   zone *zone2;

   nElement = 1;
   zone2 = LireSousZones(zone1,1);

   if (LireZoneHauteur(zone1)>50)
      ChangerZoneHauteur(zone1, 50);

   if (LireZoneLargeur(zone1)>50)
      ChangerZoneLargeur(zone1, 50);

   /* la zone à réaranger n'est pas vide */
   if(zone2 != NULL)
   {
      /* pour chaque élément de la zone */
      while(zone2 != NULL)
      {
         ChangerZoneX(zone2,0);
         ChangerZoneY(zone2,0);

         if (LireZoneLargeur(zone2) != LARGEUR_T)
            Rearanger(zone2);


         /* on détermine approximativement la surface que la zone
          * couvrira */
         surface += (LireZoneHauteur(zone2) ) *
            (LireZoneLargeur(zone2));

         /* on vérifie la largeur minimale que la surface aura
          * ( = largeur de la plus large sous zone )*/
         if(largeur < (LireZoneLargeur(zone2) + 2 * MARGE))
            largeur = (LireZoneLargeur(zone2) + 2 * MARGE);

         nElement++;
         zone2 = LireSousZones(zone1,nElement);
      }

      /* on prend comme largeur le maximum entre la largeur minimale
       * et la racine carre de la surface */
      if (largeur < ceil(sqrt(surface)) + 2 * MARGE)
         largeur = ceil(sqrt(surface)) + 2 * MARGE;

      ChangerZoneLargeur(zone1, largeur);

      /* on cree un tableau de plateau */
      tabX = malloc((1 + nElement) * sizeof(int));
      tabY = malloc((1 + nElement) * sizeof(int));

      /* qu'on initialise : */
      for (j = 0; j < 1 + nElement; j++)
      {
         tabX[j] = 0;
         tabY[j] = 0;
      }

      /* initialement il n'y a qu'un plateau */
      nPlateau = 1;
      tabX[0] = MARGE;
      tabY[0] = MARGE;


      /* pour chaque élément, on essaye de l'insérer dans le tableau
       * de plateau */
      for(j = 1; j < nElement; j++)
      {
         /* on fixe le minimum à l'infini; */
         minY = 2147483647;

         /* nMin : numéro du plateau ou "ça passe le mieu" */
         nMin = -1;

         /* on teste pour chaque plateau si il "passe" à cet
          * endroit */
         for(k = 0; k < nPlateau; k++)
         {
            minY2 = TesterPositionnerZone(tabX, tabY,
                                          largeur,
                                          nPlateau, k,
                                          LireZoneLargeur(LireSousZones(zone1,j)));

            /* si on touve un meilleur endroit ou placer la zone,
             * alors on sauvegarde ses coordonnees */
            if (minY2 < minY)
            {
               minY = minY2;
               nMin = k;
            }
         }

         Translater(LireSousZones(zone1,j),
                    tabX[nMin] - LireZoneX(LireSousZones(zone1,j)),
                    minY - LireZoneY(LireSousZones(zone1,j)));

         if (LireZoneX(LireSousZones(zone1,j)) +
             LireZoneLargeur(LireSousZones(zone1,j)) >
             largeurMax)
            largeurMax = LireZoneX(LireSousZones(zone1,j)) +
               LireZoneLargeur(LireSousZones(zone1,j));

         /* quand on a trouvé la meilleur position pour mettre la
            nouvelle zone, on met à jour le tableau de plateau */
         nPlateau = MAJTabPlateau(tabX, tabY, nPlateau, nMin,
                                  LireSousZones(zone1, j), zone1, minY);

         /* debug */
/*          printTab(tabX,nPlateau); */
/*          printf("_"); */
/*          printTab(tabY,nPlateau); */
/*          printf("\n"); */

         ChangerZoneLargeur(zone1, largeurMax + MARGE);
         
         if (LireZoneHauteur(zone1) < tabY[nMin] + MARGE)
            ChangerZoneHauteur(zone1, tabY[nMin] + MARGE);
      }

      free(tabX);
      free(tabY);

      return 1;

   }

   return 1;
}

/* fonction qui translate un zone et les sous-zones d'un vecteur x, y */
int Translater(zone * zonePrincipale, int x, int y)
{
   int i;

   zone* sousZone;

   /* on décale la zone de x et y */
   ChangerZoneX(zonePrincipale, LireZoneX(zonePrincipale) + x);
   ChangerZoneY(zonePrincipale, LireZoneY(zonePrincipale) + y);

   /* et on fait pareil recursivement sur les sous-zones */
   for(i = 1;
       (sousZone = LireSousZones(zonePrincipale, i)) != NULL;
       i++)
      Translater(sousZone, x, y);

   return 0;
}

/* fonction servant à tester si une zone peut s'insérer à un
 * endroit */
int TesterPositionnerZone(int* tabX, int *tabY,
                          int largeurZoneP,
                          int nPlateau, int plateau,
                          int largeurSousZone)
{
   int i;
   int y = tabY[plateau];

   /* si la zone à insérer est trop à droite, on retourne l'infini */
   if (largeurZoneP - MARGE < largeurSousZone + tabX[plateau])
   {
      return 2147483647;
   }

   /* sinon, on test pour chacun des plateaux qui sont au niveau de la
    * sous zone lequel gène le plus la zone (est le plus haut) pour
    * déterminer à quelle hauteur minimale peut on placer la sous
    * zone*/ 
   for(i = plateau + 1;
       (tabX[i] < largeurSousZone + tabX[plateau]) &&
          (i < nPlateau );
       i++)
   {
      if (tabY[i] > y)
         y = tabY[i];
   }
 
   return y;
}

/* fonction mettant à jour la liste des plateaux en rajoutant une autre
 * zone */
int MAJTabPlateau(int* tabX, int *tabY,
                  int nPlateau, int plateau,
                  zone* SousZone,
                  zone* ZonePrincipale,
                  int Y)
{
   int i,NbPlateauASupr,k,NbPlateauACreer;

   NbPlateauASupr = 0;
   NbPlateauACreer = 1;

   /* on parcourt le tableau de plateau pour trouver le nombre (NbPlateauASupr) de
    * plateau qui seront à supprimer par l'ajout du nouveau plateau */
   for (i = plateau + 1; i< nPlateau; i++, NbPlateauASupr++)
   {
      /* si on tombe "pile" sur un plateau, pas besoin d'en créer un */
      if(tabX[i] == tabX[plateau] + LireZoneLargeur(SousZone))
         NbPlateauACreer = 0;

      /* si on passe sur des tableau à droite de la zone on s'arrète */
      if (tabX[i] >= tabX[plateau] + LireZoneLargeur(SousZone))
         break;
   }

   /* si on tombe pile sur la fin de la zone principale on n'a pas
    * besoin de creer un nouveau plateau */
   if(LireZoneLargeur(ZonePrincipale) - MARGE ==
      tabX[plateau] + LireZoneLargeur(SousZone))
      NbPlateauACreer = 0;

   /* on décale les plateaux vers la gauche dans le tableau */
   for(k = plateau + 1 + NbPlateauACreer;
       k < nPlateau - NbPlateauACreer - NbPlateauASupr;
       k++)
   {
      tabX[k] = tabX[k + NbPlateauASupr];
      tabY[k] = tabY[k + NbPlateauASupr];
   }

   nPlateau -= NbPlateauASupr;

   /* si on doit creer un nouveau plateau */
   if (NbPlateauACreer == 1)
   {
      /* on décale les plateaux d'une case vers la droite dans le
       * tableau pour laisser de la place au nouveau plateau*/
      for(k = nPlateau;
          k > plateau + 1;
          k--)
      {
         tabX[k] = tabX[k - 1];
         tabY[k] = tabY[k - 1];
      }

      /* et on insere un nouveau plateau */
      tabY[plateau + 1] = Y;
      tabX[plateau + 1] = tabX[plateau] + LireZoneLargeur(SousZone);
   }


   tabY[plateau] = Y + LireZoneHauteur(SousZone);

   return nPlateau - NbPlateauASupr + NbPlateauACreer;
}

/* fonction de trace */
void printTab(int* tab, int taille)
{
   int i;

   for(i = 0; i < taille; i++)
      printf("_%d",tab[i]);

   printf("\n");
   return;
}

/* fontion de trace */
void PrintZone(zone* zonePrincipale)
{
   zone* sousZone;

   int i;

   printf("L:%d H:%d X:%d Y:%d\n",
          LireZoneLargeur(zonePrincipale),
          LireZoneHauteur(zonePrincipale),
          LireZoneX(zonePrincipale),
          LireZoneY(zonePrincipale));

   sousZone = LireSousZones(zonePrincipale, i);

   for(i = 1;
       (sousZone = LireSousZones(zonePrincipale, i)) != NULL;
       i++)
      PrintZone(sousZone);

   return;
}
