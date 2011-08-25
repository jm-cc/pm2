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
 * File  : texture.c
 * Author: Blino
 *         revised by Lightness1024!
 *********************************************************************/

#include "texture.h"

GLuint SDL_GL_LoadTexture(SDL_Surface* surface, GLfloat* texcoord);

Texture* LoadTextureFromSurface(SDL_Surface* image)
{
   Texture* tex = (Texture*) malloc(sizeof(Texture));
   if (tex == NULL)
      fwprintf(stderr, L"LoadTexture : Impossible d'allouer de la mémoire\n");
   else
   {
      GLfloat texcoord[4];
      GLuint texID = SDL_GL_LoadTexture(image, texcoord);
      tex->texID = texID;
      tex->texMinX = texcoord[0];
      tex->texMinY = texcoord[1];
      tex->texMaxX = texcoord[2];
      tex->texMaxY = texcoord[3];
      tex->largeur = image->w;
      tex->hauteur = image->h;
      tex->width_c = NextPow2(image->w);
      tex->height_c = NextPow2(image->h);
   }
   return tex;
}

Texture* LoadTextureFromFile(const char* imagePath)
{
   SDL_Surface* image = IMG_Load(imagePath);
   if (image == NULL)
   {
      fprintf(stderr,
              "Impossible de charger l'image \"%s\": %s\n",
              imagePath,
              SDL_GetError());
      return NULL;
   }
   else
   {
      /* activer la transparence pour cette image */
      SDL_SetAlpha(image, SDL_RLEACCEL, SDL_ALPHA_TRANSPARENT);
      Texture* tex = (Texture*) LoadTextureFromSurface(image);
      SDL_FreeSurface(image);
      return tex;
   }
}

void ActivateTexture(Texture* tex)
{
   glBindTexture(GL_TEXTURE_2D, tex->texID);
   glEnable(GL_TEXTURE_2D);
}

void DisableTextures(void)
{
   glBindTexture(GL_TEXTURE_2D, 0);
   glDisable(GL_TEXTURE_2D);
}

void DeleteTexture(Texture* tex)
{
   glDeleteTextures(1, &(tex->texID));
   free(tex);
}

/*
  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

  Code from Sam Lantinga
  slouken@libsdl.org
*/

GLuint SDL_GL_LoadTexture(SDL_Surface* surface, GLfloat* texcoord)
{
   GLuint tex;
   int w, h;
   SDL_Surface* image;
   SDL_Rect area;
   Uint32 saved_flags;
   Uint8  saved_alpha;

   /* Use the surface width and height expanded to powers of 2 */

   w = NextPow2(surface->w);
   h = NextPow2(surface->h);


   texcoord[0] = 0.0f;			/* Min X */
   texcoord[1] = 0.0f;			/* Min Y */
   texcoord[2] = (GLfloat)surface->w / w;	/* Max X */
   texcoord[3] = (GLfloat)surface->h / h;	/* Max Y */

   image = SDL_CreateRGBSurface(SDL_SWSURFACE,
                                w, h,
                                32,
#if SDL_BYTEORDER == SDL_LIL_ENDIAN /* OpenGL RGBA masks */
                                0x000000FF,
                                0x0000FF00,
                                0x00FF0000,
                                0xFF000000
#else
                                0xFF000000,
                                0x00FF0000,
                                0x0000FF00,
                                0x000000FF
#endif
                                );

   if (image == NULL)
   {
      return 0;
   }

   /* Save the alpha blending attributes */
   saved_flags = surface->flags&(SDL_SRCALPHA | SDL_RLEACCELOK);
   saved_alpha = surface->format->alpha;
   if ((saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA)
   {
      SDL_SetAlpha(surface, 0, 0);
   }

   /* Copy the surface into the GL Texture image */
   area.x = 0;
   area.y = 0;
   area.w = surface->w;
   area.h = surface->h;

   SDL_BlitSurface(surface, &area, image, &area);

   /* Restore the alpha blending attributes */
   if ((saved_flags & SDL_SRCALPHA) == SDL_SRCALPHA)
   {
      SDL_SetAlpha(surface, saved_flags, saved_alpha);
   }

   /* Create an OpenGL Texture for the image */
   glGenTextures(1, &tex);
   glBindTexture(GL_TEXTURE_2D, tex);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

   glTexImage2D(GL_TEXTURE_2D,
                0,
                GL_RGBA,
                w, h,
                0,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                image->pixels);

   SDL_FreeSurface(image); /* No longer needed */

   return tex;
}
