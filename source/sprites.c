#include "sprites.h"
#include <3ds.h>
#include <stdlib.h> 
#include "main.h"


extern size_t linear_bytes_used;


C2D_SpriteSheet testsheet, logo, fridge, backgrounds, logo_inpost;
C2D_Image test, fridge_image, bg_top, bg_bottom, logo3ds;


C2D_SpriteSheet paczka;
C2D_Image paczka_closed, znaczek, paczka_sel, otworz_zdalnie_button;


C2D_SpriteSheet placeholder_chan;
C2D_Image chan_placeholder, kun_placeholder;

C2D_SpriteSheet arhn;
C2D_Image arhn_siema, arhn_explain, arhn_diss;

C2D_SpriteSheet chan;
C2D_Image chan_excited, chan_idle_speak, chan_idle, chan_diss_speak, chan_diss;

C2D_SpriteSheet kun;
C2D_Image kun_excited, kun_idle_speak, kun_idle, kun_diss_speak, kun_diss;

C2D_SpriteSheet pakuj;
C2D_Image sie_do_wiezienia;

C2D_SpriteSheet tutorial0, tutorial0_bot;
C2D_Image tut0_0, tut0_1, tut0_2, tut0_3, tut0_bot0, tut0_bot1;

C2D_SpriteSheet tutorial1, tutorial1_bot;
C2D_Image tut1_0, tut1_bot0, tut1_bot1;

C2D_SpriteSheet tutorial2;
C2D_Image tut2_0;

C2D_SpriteSheet tutorial3;
C2D_Image tut3_0;

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
    if (!frames || framecount == 0 || !anim) return;

    int totalFrames = C2D_SpriteSheetCount(frames);
    if (framecount > totalFrames) framecount = totalFrames;
    if (anim->done) return;

    u64 now = osGetTime();

    
    if (now - anim->lastFrameTime >= 1000 / framerate) {
        bool shouldAdvance = true;

        if (anim->halt_at_frame >= 0 && anim->currentFrame == anim->halt_at_frame) {
            if (!anim->halting) {
                anim->halting = true;
                anim->haltStartTime = now;
                shouldAdvance = false;
            } else {
                
                if (now - anim->haltStartTime >= anim->halt_for_howlong) {
                    anim->halting = false;
                    shouldAdvance = true;
                } else {
                    shouldAdvance = false;
                }
            }
        }

        if (shouldAdvance) {
            anim->currentFrame++;
            if (anim->currentFrame >= framecount) {
                if (anim->loops == -1) {
                    anim->currentFrame = 0;  
                } else {
                    anim->loopedtimes++;
                    if (anim->loopedtimes >= anim->loops) {
                        anim->done = true;
                        anim->currentFrame = framecount - 1; 
                        return;
                    } else {
                        anim->currentFrame = 0; 
                    }
                }
            }


            anim->lastFrameTime = now;
        }
    }
    
    if (anim->currentFrame < 0 || anim->currentFrame >= totalFrames) return;

    C2D_Image frameImage = C2D_SpriteSheetGetImage(frames, anim->currentFrame);
    if (!frameImage.subtex) return;
    C2D_DrawImageAt(frameImage, x, y, depth, NULL, direction * scale, scale);
}

void spritesInit() {
    srand(osGetTime());

    
    
    s64 memBefore = osGetMemRegionUsed(MEMREGION_APPLICATION);

    logo = C2D_SpriteSheetLoad("romfs:/gfx/logo.t3x");
    placeholder_chan = C2D_SpriteSheetLoad("romfs:/gfx/placeholder_chan.t3x");
    arhn = C2D_SpriteSheetLoad("romfs:/gfx/arhn.t3x");
    chan = C2D_SpriteSheetLoad("romfs:/gfx/chan.t3x");
    kun = C2D_SpriteSheetLoad("romfs:/gfx/kun.t3x");
    pakuj = C2D_SpriteSheetLoad("romfs:/gfx/pakuj.t3x");
    backgrounds = C2D_SpriteSheetLoad("romfs:/gfx/bg.t3x");
    logo_inpost = C2D_SpriteSheetLoad("romfs:/gfx/logo_inpost.t3x");
    paczka = C2D_SpriteSheetLoad("romfs:/gfx/paczka.t3x");
    
    if (rand() % 2 == 0) {
        fridge = C2D_SpriteSheetLoad("romfs:/gfx/tehfridge2.t3x");
    } else {
        fridge = C2D_SpriteSheetLoad("romfs:/gfx/tehfridge.t3x");
    }
    
    tutorial0 = C2D_SpriteSheetLoad("romfs:/gfx/tutorial_0.t3x");
    tutorial1 = C2D_SpriteSheetLoad("romfs:/gfx/tutorial_1.t3x");
    tutorial2 = C2D_SpriteSheetLoad("romfs:/gfx/tutorial_2.t3x");
    tutorial3 = C2D_SpriteSheetLoad("romfs:/gfx/tutorial_3.t3x");
    tutorial0_bot = C2D_SpriteSheetLoad("romfs:/gfx/tutorial_0_bottom.t3x");
    tutorial1_bot = C2D_SpriteSheetLoad("romfs:/gfx/tutorial_1_bottom.t3x");

    
    
    s64 memAfter = osGetMemRegionUsed(MEMREGION_APPLICATION);
    
    
    if (memAfter > memBefore) {
        linear_bytes_used += (size_t)(memAfter - memBefore);
    }
    
    sie_do_wiezienia = C2D_SpriteSheetGetImage(pakuj, 0);
    arhn_siema = C2D_SpriteSheetGetImage(arhn, 0);
    arhn_explain = C2D_SpriteSheetGetImage(arhn, 1);
    arhn_diss = C2D_SpriteSheetGetImage(arhn, 2);
    
    chan_excited = C2D_SpriteSheetGetImage(chan, 0);
    chan_idle_speak = C2D_SpriteSheetGetImage(chan, 1);
    chan_idle = C2D_SpriteSheetGetImage(chan, 2);
    chan_diss_speak = C2D_SpriteSheetGetImage(chan, 3);
    chan_diss = C2D_SpriteSheetGetImage(chan, 4);

    kun_excited = C2D_SpriteSheetGetImage(kun, 0);
    kun_idle_speak = C2D_SpriteSheetGetImage(kun, 1);
    kun_idle = C2D_SpriteSheetGetImage(kun, 2);
    kun_diss_speak = C2D_SpriteSheetGetImage(kun, 3);
    kun_diss = C2D_SpriteSheetGetImage(kun, 4);

    tut0_0 = C2D_SpriteSheetGetImage(tutorial0, 0);
    tut0_1 = C2D_SpriteSheetGetImage(tutorial0, 1);
    tut0_2 = C2D_SpriteSheetGetImage(tutorial0, 2);
    tut0_3 = C2D_SpriteSheetGetImage(tutorial0, 3);
    tut0_bot0 = C2D_SpriteSheetGetImage(tutorial0_bot, 0);
    tut0_bot1 = C2D_SpriteSheetGetImage(tutorial0_bot, 1);
    tut1_0 = C2D_SpriteSheetGetImage(tutorial1, 0);
    tut1_bot0 = C2D_SpriteSheetGetImage(tutorial1_bot, 0);
    tut1_bot1 = C2D_SpriteSheetGetImage(tutorial1_bot, 1);
    tut2_0 = C2D_SpriteSheetGetImage(tutorial2, 0);
    tut3_0 = C2D_SpriteSheetGetImage(tutorial3, 0);
    chan_placeholder = C2D_SpriteSheetGetImage(placeholder_chan, 0);
    kun_placeholder = C2D_SpriteSheetGetImage(placeholder_chan, 1);
    bg_top = C2D_SpriteSheetGetImage(backgrounds, 0);
    bg_bottom = C2D_SpriteSheetGetImage(backgrounds, 1);
    fridge_image = C2D_SpriteSheetGetImage(fridge, 0);
    logo3ds = C2D_SpriteSheetGetImage(logo_inpost, 0);
    paczka_closed = C2D_SpriteSheetGetImage(paczka, 0);
    znaczek = C2D_SpriteSheetGetImage(paczka, 2);
    paczka_sel = C2D_SpriteSheetGetImage(paczka, 1);
    otworz_zdalnie_button = C2D_SpriteSheetGetImage(paczka, 3);
}