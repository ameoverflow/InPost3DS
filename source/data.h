#ifndef DATA_H
#define DATA_H

#include <citro2d.h>

// kolorki
typedef struct {
    unsigned char r, g, b, a;
} GFX_COLOR;

typedef struct {
    C2D_SpriteSheet sheet; 
    C2D_Image image;       
    float width;
    float height;
} GFX_IMAGE;

GFX_IMAGE* GFX_LoadTexture(const char* path, int pos_in_sheet);
void GFX_FreeTexture(GFX_IMAGE* tex);

#endif