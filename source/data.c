#include <stdlib.h>
#include <citro2d.h>
#include "data.h"

GFX_IMAGE* GFX_LoadTexture(const char* path, int pos_in_sheet) {

    GFX_IMAGE* newTex = (GFX_IMAGE*)malloc(sizeof(GFX_IMAGE));
    if (newTex == NULL) return NULL;

    newTex->sheet = C2D_SpriteSheetLoad(path);
    if (!newTex->sheet) {
        free(newTex);
        return NULL;
    }

    newTex->image = C2D_SpriteSheetGetImage(newTex->sheet, pos_in_sheet);

    if (newTex->image.subtex) {
        newTex->width = (float)newTex->image.subtex->width;
        newTex->height = (float)newTex->image.subtex->height;
    }

    return newTex;
}

void GFX_FreeTexture(GFX_IMAGE* tex) {
    if (tex) {
        if (tex->sheet) {
            C2D_SpriteSheetFree(tex->sheet);
        }
        free(tex);
    }
}