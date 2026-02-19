#include "drawing.h"

void GFX_DrawImageAt(GFX_IMAGE* texture, float x, float y, float depth, C2D_ImageTint *tint, float scalex, float scaley) {
    C2D_DrawImageAt(texture->image, x, y, depth, tint, scalex, scaley);  
}