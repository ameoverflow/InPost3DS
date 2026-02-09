#include <3ds.h>
#include <citro2d.h>
#include "scene_intro.h"
#include "scene_manager.h"
#include "main.h"
#include "sprites.h"
#include "cwav_shit.h"
#include <math.h>
#include <stdlib.h>
#include "scene_debug.h"

float easeOutCubicIntro(float t) {
    return 1 - powf(1 - t, 3);
}
static float introTimer = 0.0f;
static float waitTimer = 0.0f;
static float secondTimer = 0.0f;

static bool animationDone = false;
static bool waitingDone = false;
static bool secondAnimDone = false;

C2D_TextBuf textBuf;
C2D_Text text;

static bool textAnimationStarted = false;

static float bounceTimer = 0.0f;
static int bounceStage = 0; 

#define BOUNCE_DURATION 0.41f 
static int bounceCount = 0;
static bool bounceFinished = false;
int lastBounceStage = -1;


static float flashTimer = 0.0f;
#define FLASH_DURATION 1.0f
static float fadeTimer = 0.0f;
#define FADE_DURATION 1.0f  
static bool fadeStarted = false;


void sceneIntroInit(void) {
    
    gfxSet3D(true); 
    getCwav(0); 
    getCwav(2); 
    getCwav(4); 
    
    playCwav(2, true); 

    textBuf = C2D_TextBufNew(128);
}

void sceneIntroUpdate(uint32_t kDown, uint32_t kHeld) {
    if (!animationDone) {
        introTimer += 0.023f;
        if (introTimer >= 1.0f) {
            introTimer = 1.0f;
            animationDone = true;
        }
    } else if (!waitingDone) {
        waitTimer += 1.0f / 60.0f; 
        if (waitTimer >= 0.35f) {
            waitingDone = true;
        }
    } else if (!secondAnimDone) {
        secondTimer += 0.06f;
        if (secondTimer >= 0.87f) {
            secondTimer = 1.0f;
            secondAnimDone = true;
        }
    }

    if (secondAnimDone) {
        textAnimationStarted = true;

        if (!bounceFinished) {
            bounceTimer += 1.0f / 60.0f; 

            if (bounceTimer >= BOUNCE_DURATION) {
                bounceTimer = 0.0f;
                bounceStage = (bounceStage + 1) % 3;  

                const char* stages[] = { "TehFridge", "Teh", "TehFri" };
                if (bounceStage != lastBounceStage) {
                    C2D_TextParse(&text, textBuf, stages[bounceStage]);
                    C2D_TextOptimize(&text);
                    lastBounceStage = bounceStage;
                }

                bounceCount++;
                if (bounceCount >= 3) {  
                    bounceFinished = true;
                    bounceStage = 0;
                    C2D_TextParse(&text, textBuf, stages[bounceStage]);
                    C2D_TextOptimize(&text);
                    lastBounceStage = bounceStage;

                    flashTimer = FLASH_DURATION;  
                }
            }
        }
    }

    if (flashTimer > 0.0f) {
        flashTimer -= 1.0f / 60.0f;
        if (flashTimer < 0.0f) flashTimer = 0.0f;
    } else if (bounceFinished && !fadeStarted) {

        fadeTimer += 1.0f / 60.0f;
        if (fadeTimer >= 1.0f) {
            fadeStarted = true;
            fadeTimer = 0.0f;  
        }
    }

    if (fadeStarted) {
        fadeTimer += 1.0f / 60.0f;
        if (fadeTimer >= FADE_DURATION) {
            fadeTimer = FADE_DURATION;
        
            sceneManagerSwitchTo(SCENE_MAIN_MENU);
        }
    }
    #ifdef DEBUG
        if ((kDown & KEY_R) && !fake_paczka_mode) {
            fake_paczka_mode = true; 
            playCwav(0, true);
        }
        if ((kDown & KEY_L) && !debugmode) {
            debugmode = true; 
            playCwav(0, true);
        }
    #endif
}

void sceneIntroRender(void) {
    C2D_SceneBegin(left);
    C2D_TargetClear(left, C2D_Color32(0, 0, 0, 255));
    C2D_TargetClear(right, C2D_Color32(0, 0, 0, 255));
    float scale;

    if (!animationDone) {
        float t = easeOutCubicIntro(introTimer);
        scale = (1.0f - t) * 40.0f + t * 0.3f;  
    } else if (!waitingDone) {
        scale = 0.3f;  
    } else if (!secondAnimDone) {
        float t = easeOutCubicIntro(secondTimer);
        scale = (1.0f - t) * 0.3f + t * 0.22f;  
    } else {
        scale = 0.22f;
    }

    float imgW = fridge_image.subtex->width * scale;
    float imgH = fridge_image.subtex->height * scale;

    float centerX = 400.0f / 2.0f;
    float centerY = 240.0f / 2.0f;

    float drawX = centerX - imgW / 2.0f;
    float drawY = centerY - imgH / 2.0f;
    
    C2D_DrawImageAt(
        fridge_image,
        drawX - (3 * slider), drawY + 10.0f,
        0.1f,
        NULL,
        scale, scale
    );

    C2D_SceneBegin(right);
    if (slider != 0.0) {
        C2D_DrawImageAt(
            fridge_image,
            drawX + (3 * slider), drawY + 10.0f,
            0.1f,
            NULL,
            scale, scale
        );        
    }

    C2D_SceneBegin(left);
    if (textAnimationStarted) {
		float bounceScale = 1.0f;

		if (!bounceFinished) {
			float bounceProgress = bounceTimer / BOUNCE_DURATION;
			float eased = powf(sinf(bounceProgress * M_PI), 0.14f);  
			bounceScale = 1.2f - 0.2f * eased;  
		}

        float textX = 400.0f / 2.0f;

        float textWidth, textHeight;
        C2D_TextGetDimensions(&text, 1.0f, 1.0f, &textWidth, &textHeight);

        float drawX = textX - (textWidth * bounceScale) / 2.0f;

        C2D_DrawText(&text, C2D_WithColor, drawX - (3 * slider), 20.0f, 1.0f, bounceScale, bounceScale, C2D_Color32(235, 0, 146, 255));

        if (slider != 0.0) {
            C2D_SceneBegin(right);
            C2D_DrawText(&text, C2D_WithColor, drawX + (3 * slider), 20.0f, 1.0f, bounceScale, bounceScale, C2D_Color32(235, 0, 146, 255));
            C2D_SceneBegin(left);
        }
    }

    if (flashTimer > 0.0f) {
        float alpha = (flashTimer / FLASH_DURATION) * 255.0f;
        C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, C2D_Color32(255, 255, 255, (uint8_t)alpha));
        if (slider != 0.0f) {
            C2D_SceneBegin(right);
            C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, C2D_Color32(255, 255, 255, (uint8_t)alpha));
            C2D_SceneBegin(left);
        }
    }
    if (fadeStarted) {
        float alpha = (fadeTimer / FADE_DURATION) * 255.0f;
        C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, C2D_Color32(0, 0, 0, (uint8_t)alpha));
        if (slider != 0.0f) {
            C2D_SceneBegin(right);
            C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, C2D_Color32(0, 0, 0, (uint8_t)alpha));
            C2D_SceneBegin(left);
        }
    }

    C2D_SceneBegin(bottom);
    C2D_TargetClear(bottom, C2D_Color32(0, 0, 0, 255));
    if (flashTimer > 0.0f) {
        float alpha = (flashTimer / FLASH_DURATION) * 255.0f;
        C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, C2D_Color32(255, 255, 255, (uint8_t)alpha));
    }
}

void sceneIntroExit(void) {
    if (textBuf) {
        C2D_TextBufDelete(textBuf);
        textBuf = NULL;
    }
    
    unloadCwavIndex(2);
}