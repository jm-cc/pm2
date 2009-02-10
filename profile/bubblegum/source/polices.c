/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2009 "the PM2 team" (see AUTHORS file)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at
 * your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 */

/**********************************************************************
 * File  : polices.c
 * Author: Lightness1024
 *         mailto:oddouv@enseirb.fr
 * Date  : 14/14/2006
 *
 *             basé sur la 'lesson 43' des tutoriels de nehe
 *
 *********************************************************************/

// #define VERBOSE_GPF   // sorties de debug

#include "polices.h"

// fonctions locales au module (ne vont pas dans les .o)
static void PreparePixMap(GPFont* ft, int ch, FT_Face face);
static void PrintCmn     (GPFont* ft, const char* fmt, va_list vl);

int NextPow2(int a)  // retourne puissance de deux directement supérieure à a.
{
   int rval = 1;
   while (rval < a)
      rval <<= 1;
   return rval;
}

// initialisation / chargement
GPFont* InitGPFont(const char* fontName, int ptSize)
{
   FT_Library ftlib;
   FT_Face face;

   if (FT_Init_FreeType(&ftlib))
      return NULL;
   if (FT_New_Face(ftlib, fontName, 0, &face))
      return NULL;

   GPFont* gpfont = malloc(sizeof(GPFont));  // allocation dans le tas pour être retournée valide
   gpfont->bfsize = 1;
   gpfont->buf = malloc(1);
   
   FT_Set_Char_Size(face, 0, ptSize << 6, 96, 96); // résolution courante des écrans: 96 dpi

   gpfont->listBase = glGenLists(256);  // on réserve des display lists
   glGenTextures(256, gpfont->texNames);  // autant de textures
   int ch;
   for (ch = 0; ch < 256; ++ch)
      PreparePixMap(gpfont, ch, face);    // pour chaque char son sprite

#ifdef VERBOSE_GPF
   wprintf(L"police chargée. %ld glyphes.\n", face->num_glyphs);
#endif
   
   FT_Done_Face(face);
   FT_Done_FreeType(ftlib);

   return gpfont;
}

static void PreparePixMap(GPFont* ft, int ch, FT_Face face)
{
   // on fait la conversion du caractère en indice de glyphe
   // puis en bitmap antialiasé en une commande:
   FT_Load_Char(face, ch, FT_LOAD_RENDER);
   FT_Bitmap* pgpix = &face->glyph->bitmap;  // raccourci
   int w, h;
   w = NextPow2(pgpix->width);   // on fait une texture alignée
   h = NextPow2(pgpix->rows);   
   // on alloue le pixmap (bitmap monochrome-alpha)
   GLubyte* pixmap = malloc(sizeof(GLubyte) * w * h * 2);
   // il faut remplir cet espace et copier la glyphe.
   // attention au padding
   int x, y;
   for (x = 0; x < w; ++x)
   {
      for (y = 0; y < h; ++y)
      {
         if (x < pgpix->width && y < pgpix->rows)
            pixmap[2 * (y * w + x)] = pixmap[2 * (y * w + x) + 1] = pgpix->buffer[y * pgpix->width + x];
         else
            pixmap[2 * (y * w + x)] = pixmap[2 * (y * w + x) + 1] = 0;  // chrome noir et alpha 100%
      }
   }
   glBindTexture(GL_TEXTURE_2D, ft->texNames[ch]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  // pas pigé pkoi ca devait etre là ca
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_LUMINANCE_ALPHA, GL_UNSIGNED_BYTE, pixmap);

   glNewList(ft->listBase + ch, GL_COMPILE);
   {
      // activer la bonne texture:
      glBindTexture(GL_TEXTURE_2D, ft->texNames[ch]);
      glPushMatrix();
      // déplacer un peu a droite (voir la métrique des glyphes):
      glTranslatef(face->glyph->bitmap_left, 0, 0);
      // déplacer en bas en cas de glyphe dépassant la base-line:
      glTranslatef(0, face->glyph->bitmap_top - pgpix->rows, 0);

      // trouver les coordonnées de texture où s'arreter
      // c'est un contre effet du padding:
      float	u, v;
      u = (float)pgpix->width / w;
      v = (float)pgpix->rows / h;

      // on dessine le sprite:

      glBegin(GL_QUADS);
      {
         glTexCoord2d(0, 0); glVertex3f(0, pgpix->rows, 0);
         glTexCoord2d(0, v); glVertex3f(0, 0, 0);
         glTexCoord2d(u, v); glVertex3f(pgpix->width, 0, 0);
         glTexCoord2d(u, 0); glVertex3f(pgpix->width, pgpix->rows, 0);
      }
      glEnd();
      glPopMatrix();
      // on stocke les avances pour faire la fonction qui donne la largeur d'une chaine
      ft->glyph_advances[ch] = face->glyph->advance.x >> 6;
      // on avance le 'stylo' de ce qu'il faut dans la métrique:
      glTranslatef(face->glyph->advance.x >> 6, 0, 0);
#ifdef VERBOSE_GPF
      printf("%d:%c\n", ch, ch);
#endif
      
   }
   glEndList();

   free(pixmap);
}

// récupère la largeur en pixel que prendra l'affichage d'une chaine
int GetStringPixelWidth(GPFont* ft, char* message)
{
   int i;
   int sum = 0;
   int len = strlen(message);
   for (i = 0; i < len; ++i)
   {
      sum += ft->glyph_advances[(int)((unsigned char)message[i])];
   }
   return sum;
}

// affiche dans l'espace 3D
static void PrintCmn(GPFont* ft, const char* fmt, va_list vl)
{
   glPushAttrib(GL_ALL_ATTRIB_BITS);
   glPushMatrix();
 
   glEnable(GL_TEXTURE_2D);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
   
   int len = strlen(fmt);
   if ((signed)ft->bfsize <= len)
   {
      free(ft->buf);
      ft->buf = malloc(len + 10);
      ft->bfsize = len + 10;
   }

   int n = 0;
   do
   {
      n = vsnprintf(ft->buf, ft->bfsize, fmt, vl);
      if (n < 0 || n >= (signed)ft->bfsize)  // pas assez d'espace dans la chaine
      {
         if (n < 0)  // glib <= 2.0
         {
            len += 30;  // on ne sait pas de combien il faut augmenter
         }
         else
         {
			  len = n + 1;
         }
			free(ft->buf);
			ft->buf = malloc(len);
			ft->bfsize = len;
         n = -1; // pour le while
      }
      else
         len = n;  // ca c'est bien passé
   } while (n < 0);

   glListBase(ft->listBase);
   glCallLists(len, GL_UNSIGNED_BYTE, ft->buf);  // poum tout d'un coup

   glPopMatrix();
   glPopAttrib();
}

// affiche dans l'espace non projeté
void Print2D(GPFont* ft, const char* fmt, ...)
{
// pas la peine de ralentir, dans cette application on est toujours en screen coordinate
//   PushScreenCoordinateMatrix();
   va_list vl;
   va_start(vl, fmt);
   PrintCmn(ft, fmt, vl);
   va_end(vl);
//   PopProjectionMatrix();
}

void DeleteGPFont(GPFont* ft)
{
   glDeleteLists(ft->listBase, 256);
   glDeleteTextures(256, ft->texNames);
}

void PushScreenCoordinateMatrix(void)  // projection orthogonale
{
   glPushAttrib(GL_TRANSFORM_BIT);
   GLint viewport[4];
   glGetIntegerv(GL_VIEWPORT, viewport);
   glMatrixMode(GL_PROJECTION);
   glPushMatrix();
   glLoadIdentity();
   glOrtho(viewport[0], viewport[2], viewport[1], viewport[3], -1000, 1000);
   glPopAttrib();
}

void PopProjectionMatrix(void)
{
   glPushAttrib(GL_TRANSFORM_BIT);
   glMatrixMode(GL_PROJECTION);
   glPopMatrix();
   glPopAttrib();
}
