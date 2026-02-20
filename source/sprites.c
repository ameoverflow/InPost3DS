#include "sprites.h"
#include <3ds.h>
#include <stdlib.h> 
#include "main.h"


extern size_t linear_bytes_used;



GFX_IMAGE* test;
GFX_IMAGE* fridge_image;
GFX_IMAGE* bg_top;
GFX_IMAGE* bg_bottom;
GFX_IMAGE* logo3ds;



GFX_IMAGE* paczka_closed;
GFX_IMAGE* znaczek;
GFX_IMAGE* paczka_sel;
GFX_IMAGE* otworz_zdalnie_button;



GFX_IMAGE* chan_placeholder;
GFX_IMAGE* kun_placeholder;


GFX_IMAGE* arhn_siema;
GFX_IMAGE* arhn_explain;
GFX_IMAGE* arhn_diss;


GFX_IMAGE* chan_excited;
GFX_IMAGE* chan_idle_speak;
GFX_IMAGE* chan_idle;
GFX_IMAGE* chan_diss_speak;
GFX_IMAGE* chan_diss;


GFX_IMAGE* kun_excited;
GFX_IMAGE* kun_idle_speak;
GFX_IMAGE* kun_idle;
GFX_IMAGE* kun_diss_speak;
GFX_IMAGE* kun_diss;


GFX_IMAGE* sie_do_wiezienia;


GFX_IMAGE* tut0_0;
GFX_IMAGE* tut0_1;
GFX_IMAGE* tut0_2;
GFX_IMAGE* tut0_3;
GFX_IMAGE* tut0_bot0;
GFX_IMAGE* tut0_bot1;


GFX_IMAGE* tut1_0;
GFX_IMAGE* tut1_bot0;
GFX_IMAGE* tut1_bot1;


GFX_IMAGE* tut2_0;


GFX_IMAGE* tut3_0;

u64 lastFrameTime = 0;
int currentFrame = 0;

void ResetAnimState(SpriteAnimState* anim) {
    anim->currentFrame = 0;
    anim->lastFrameTime = osGetTime();
    anim->done = false;
    anim->loopedtimes = 0;
    anim->halting = false;
    anim->haltStartTime = 0;
}

void PlaySprite(float scale, C2D_SpriteSheet frames, int framerate, int framecount,
                float x, float y, SpriteAnimState* anim, int direction, float depth) {
    // chuj w tą funckje for now

    // if (!frames || framecount == 0 || !anim) return;

    // int totalFrames = C2D_SpriteSheetCount(frames);
    // if (framecount > totalFrames) framecount = totalFrames;
    // if (anim->done) return;

    // u64 now = osGetTime();

    
    // if (now - anim->lastFrameTime >= 1000 / framerate) {
    //     bool shouldAdvance = true;

    //     if (anim->halt_at_frame >= 0 && anim->currentFrame == anim->halt_at_frame) {
    //         if (!anim->halting) {
    //             anim->halting = true;
    //             anim->haltStartTime = now;
    //             shouldAdvance = false;
    //         } else {
                
    //             if (now - anim->haltStartTime >= anim->halt_for_howlong) {
    //                 anim->halting = false;
    //                 shouldAdvance = true;
    //             } else {
    //                 shouldAdvance = false;
    //             }
    //         }
    //     }

    //     if (shouldAdvance) {
    //         anim->currentFrame++;
    //         if (anim->currentFrame >= framecount) {
    //             if (anim->loops == -1) {
    //                 anim->currentFrame = 0;  
    //             } else {
    //                 anim->loopedtimes++;
    //                 if (anim->loopedtimes >= anim->loops) {
    //                     anim->done = true;
    //                     anim->currentFrame = framecount - 1; 
    //                     return;
    //                 } else {
    //                     anim->currentFrame = 0; 
    //                 }
    //             }
    //         }


    //         anim->lastFrameTime = now;
    //     }
    // }
    
    // if (anim->currentFrame < 0 || anim->currentFrame >= totalFrames) return;

    // GFX_IMAGE* frameImage = C2D_SpriteSheetGetImage(frames, anim->currentFrame);
    // if (!frameImage.subtex) return;
    // GFX_DrawImageAt(frameImage, x, y, depth, NULL, direction * scale, scale);
}

void spritesInit() {
    srand(osGetTime());

    
    
    s64 memBefore = osGetMemRegionUsed(MEMREGION_APPLICATION);

    // logo = C2D_SpriteSheetLoad("romfs:/gfx/logo.t3x");
    // placeholder_chan = C2D_SpriteSheetLoad("romfs:/gfx/placeholder_chan.t3x");
    // arhn = C2D_SpriteSheetLoad("romfs:/gfx/arhn.t3x");
    // chan = C2D_SpriteSheetLoad("romfs:/gfx/chan.t3x");
    // kun = C2D_SpriteSheetLoad("romfs:/gfx/kun.t3x");
    // pakuj = C2D_SpriteSheetLoad("romfs:/gfx/pakuj.t3x");
    // backgrounds = C2D_SpriteSheetLoad("romfs:/gfx/bg.t3x");
    // logo_inpost = C2D_SpriteSheetLoad("romfs:/gfx/logo_inpost.t3x");
    // paczka = C2D_SpriteSheetLoad("romfs:/gfx/paczka.t3x");
    
    if (rand() % 2 == 0) {
        fridge_image = GFX_LoadTexture("romfs:/gfx/tehfridge2.t3x", 0);
    } else {
        fridge_image = GFX_LoadTexture("romfs:/gfx/tehfridge.t3x", 0);
    }
    
    // tutorial0 = C2D_SpriteSheetLoad("romfs:/gfx/tutorial_0.t3x");
    // tutorial1 = C2D_SpriteSheetLoad("romfs:/gfx/tutorial_1.t3x");
    // tutorial2 = C2D_SpriteSheetLoad("romfs:/gfx/tutorial_2.t3x");
    // tutorial3 = C2D_SpriteSheetLoad("romfs:/gfx/tutorial_3.t3x");
    // tutorial0_bot = C2D_SpriteSheetLoad("romfs:/gfx/tutorial_0_bottom.t3x");
    // tutorial1_bot = C2D_SpriteSheetLoad("romfs:/gfx/tutorial_1_bottom.t3x");

    
    
    s64 memAfter = osGetMemRegionUsed(MEMREGION_APPLICATION);
    
    
    if (memAfter > memBefore) {
        linear_bytes_used += (size_t)(memAfter - memBefore);
    }
    
    sie_do_wiezienia = GFX_LoadTexture("romfs:/gfx/pakuj.t3x", 0);
    arhn_siema = GFX_LoadTexture("romfs:/gfx/arhn.t3x", 0);
    arhn_explain = GFX_LoadTexture("romfs:/gfx/arhn.t3x", 1);
    arhn_diss = GFX_LoadTexture("romfs:/gfx/arhn.t3x", 2);
    
    chan_excited = GFX_LoadTexture("romfs:/gfx/chan.t3x", 0);
    chan_idle_speak = GFX_LoadTexture("romfs:/gfx/chan.t3x", 1);
    chan_idle = GFX_LoadTexture("romfs:/gfx/chan.t3x", 2);
    chan_diss_speak = GFX_LoadTexture("romfs:/gfx/chan.t3x", 3);
    chan_diss = GFX_LoadTexture("romfs:/gfx/chan.t3x", 4);

    kun_excited = GFX_LoadTexture("romfs:/gfx/kun.t3x", 0);
    kun_idle_speak = GFX_LoadTexture("romfs:/gfx/kun.t3x", 1);
    kun_idle = GFX_LoadTexture("romfs:/gfx/kun.t3x", 2);
    kun_diss_speak = GFX_LoadTexture("romfs:/gfx/kun.t3x", 3);
    kun_diss = GFX_LoadTexture("romfs:/gfx/kun.t3x", 4);

    tut0_0 = GFX_LoadTexture("romfs:/gfx/tutorial_0.t3x", 0);
    tut0_1 = GFX_LoadTexture("romfs:/gfx/tutorial_0.t3x", 1);
    tut0_2 = GFX_LoadTexture("romfs:/gfx/tutorial_0.t3x", 2);
    tut0_3 = GFX_LoadTexture("romfs:/gfx/tutorial_0.t3x", 3);
    tut0_bot0 = GFX_LoadTexture("romfs:/gfx/tutorial_0_bottom.t3x", 0);
    tut0_bot1 = GFX_LoadTexture("romfs:/gfx/tutorial_0_bottom.t3x", 1);
    tut1_0 = GFX_LoadTexture("romfs:/gfx/tutorial_1.t3x", 0);
    tut1_bot0 = GFX_LoadTexture("romfs:/gfx/tutorial_1_bottom.t3x", 0);
    tut1_bot1 = GFX_LoadTexture("romfs:/gfx/tutorial_1_bottom.t3x", 1);
    tut2_0 = GFX_LoadTexture("romfs:/gfx/tutorial_2.t3x", 0);
    tut3_0 = GFX_LoadTexture("romfs:/gfx/tutorial_3.t3x", 0);
    chan_placeholder = GFX_LoadTexture("romfs:/gfx/placeholder_chan.t3x", 0);
    kun_placeholder = GFX_LoadTexture("romfs:/gfx/placeholder_chan.t3x", 1);
    bg_top = GFX_LoadTexture("romfs:/gfx/bg.t3x", 0);
    bg_bottom = GFX_LoadTexture("romfs:/gfx/bg.t3x", 1);
    // fridge_image = C2D_SpriteSheetGetImage(fridge, 0);
    logo3ds = GFX_LoadTexture("romfs:/gfx/logo_inpost.t3x", 0);
    paczka_closed = GFX_LoadTexture("romfs:/gfx/paczka.t3x", 0);
    znaczek = GFX_LoadTexture("romfs:/gfx/paczka.t3x", 2);
    paczka_sel = GFX_LoadTexture("romfs:/gfx/paczka.t3x", 1);
    otworz_zdalnie_button = GFX_LoadTexture("romfs:/gfx/paczka.t3x", 3);
}