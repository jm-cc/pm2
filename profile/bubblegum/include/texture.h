/**********************************************************************
 * File  : texture.h
 * Author: Blino
 *         revised by Lightness1024!
 * Date  : 11/11/2006
 *********************************************************************/

#ifndef TEXTURE_H
#define TEXTURE_H

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
