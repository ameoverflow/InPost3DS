
#ifndef SPRITES_H
#define SPRITES_H
#include <citro2d.h>
#include <stdlib.h>
typedef struct {
    int currentFrame;
    u64 lastFrameTime;
    int loopedtimes;
    int loops;
    bool done;
    
    int halt_at_frame;
    int halt_for_howlong; 
    u64 haltStartTime;    
    bool halting;         
} SpriteAnimState;


extern C2D_SpriteSheet testsheet, logo, fridge, backgrounds, logo;
extern C2D_Image test, fridge_image, bg_top, bg_bottom, logo3ds;

extern C2D_SpriteSheet paczka;
extern C2D_Image paczka_closed, znaczek, paczka_sel, otworz_zdalnie_button;

extern C2D_SpriteSheet placeholder_chan;
extern C2D_Image chan_placeholder, kun_placeholder;

extern C2D_SpriteSheet chan;
extern C2D_Image chan_excited, chan_idle_speak, chan_idle, chan_diss_speak, chan_diss;

extern C2D_SpriteSheet kun;
extern C2D_Image kun_excited, kun_idle_speak, kun_idle, kun_diss_speak, kun_diss;

extern C2D_SpriteSheet arhn;
extern C2D_Image arhn_siema, arhn_explain, arhn_diss;

extern C2D_SpriteSheet pakuj;
extern C2D_Image sie_do_wiezienia;

extern C2D_SpriteSheet tutorial0, tutorial0_bot;
extern C2D_Image tut0_0, tut0_1, tut0_2, tut0_3, tut0_bot0, tut0_bot1;

extern C2D_SpriteSheet tutorial1, tutorial1_bot;
extern C2D_Image tut1_0, tut1_bot0, tut1_bot1;
extern C2D_SpriteSheet tutorial2;
extern C2D_Image tut2_0;
extern C2D_SpriteSheet tutorial3;
extern C2D_Image tut3_0;

void ResetAnimState(SpriteAnimState* anim);
void PlaySprite(float scale, C2D_SpriteSheet frames, int framerate, int framecount, float x, float y, SpriteAnimState* anim, int direction, float depth);
void spritesInit();
#endif