/**********************************************************************
 * File  : polices.h
 * Author: Lightness1024
 *         mailto:oddouv@enseirb.fr
 * Date  : 14/14/2006
 *********************************************************************/

#ifndef GPROG_FONTS_H
#define GPROG_FONTS_H 1

#include <ft2build.h>   // GNU freetype font
#include FT_FREETYPE_H

#include <GL/gl.h>
#include <GL/glu.h>
#include <stdarg.h>

typedef struct GPFont_tag
{
      GLuint listBase;
      GLuint texNames[256];
      int glyph_advances[256];
      char* buf;
      size_t bfsize;
} GPFont;

// appellez cette fonction pour créer une nouvelle interface
// d'affichage de texte en opengl
GPFont* InitGPFont(const char* fontName, int ptSize);

// appellez cette fonction pour afficher du texte
// la chaine de formattage et les arguments iront dans une fonction sprintf
void Print2D(GPFont* ft, const char* fmt, ...);

// pour connaître la largeur d'un texte:
int GetStringPixelWidth(GPFont* ft, char* message);

// nettoyeur:
void DeleteGPFont(GPFont* ft);

// fonctions gl necessaires a l'affichage 2d:
void PushScreenCoordinateMatrix(void);
void PopProjectionMatrix(void);

// une ptite fonction generalement necessaire
int NextPow2(int a);

#endif
