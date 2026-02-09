#include <3ds.h>
#include <citro2d.h>
#include "scene_game.h"
#include "scene_manager.h"
#include "main.h"
#include "sprites.h"
#include "cwav_shit.h"
#include <math.h>
#include <stdlib.h>

#define FLOOR_Y 160
#define CHAR_HEIGHT 31.0f
#define CHAR_WIDTH 19.0f



void sceneGameInit(void) {

}


void sceneGameUpdate(uint32_t kDown, uint32_t kHeld) {

}



void drawPlayer() {


}

void sceneGameRender(void) {
    C2D_SceneBegin(left);
    C2D_TargetClear(left, C2D_Color32(0, 0, 0, 255));
	C2D_SceneBegin(bottom);
    C2D_TargetClear(bottom, C2D_Color32(0, 0, 0, 255));

}



void sceneGameExit(void) {
    
}
