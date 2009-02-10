/*
 * PM2: Parallel Multithreaded Machine
 * Copyright (C) 2008 "the PM2 team" (see AUTHORS file)
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
 * File  : texture.h
 * Author: Blino
 *         revised by Lightness1024!
 * Date  : 11/11/2006
 *********************************************************************/

#ifndef TEXTURE_H
#define TEXTURE_H

#include <wchar.h>
#include <stdlib.h>
#include <GL/gl.h>
#include <SDL/SDL.h>
#include <SDL/SDL_image.h>

#include "polices.h"  // pour 'next power of 2'

typedef struct Texture_tag
{
    GLuint texID;
    GLfloat texMinX, texMinY, texMaxX, texMaxY;
    GLuint largeur, hauteur;
    GLuint width_c, height_c;
} Texture;

Texture* LoadTextureFromSurface(SDL_Surface* image);
Texture* LoadTextureFromFile(const char* imagePath);
void ActivateTexture(Texture* tex);
void DisableTextures(void);
void DeleteTexture(Texture* tex);


#endif /* TEXTURE_H */
