#include <3ds.h>
#include <citro2d.h>
#include <stdio.h>
#include <math.h> 
#include "scene_credits.h"
#include "scene_manager.h"
#include "main.h"
#include "sprites.h"
#include "cwav_shit.h"
#include "text.h" 
#include "scene_mainmenu.h"




#define STATE_FLASH      0
#define STATE_LOGO_FADE  1
#define STATE_WAIT       2
#define STATE_SCROLL     3


static int sceneState = 0;
static float flashAlpha = 255.0f; 
static float logoAlpha = 0.0f;    
static float waveAlpha = 0.0f;    
static float scrollY = 480.0f;    
static u64 stateTimer = 0;        
static u64 sceneStartTime = 0;    
static bool musicStarted = false; 


static C2D_TextBuf creditBuf;
static C2D_Text creditText[1000];   
static C2D_Text exitText;           

static const char* creditLines[] = {
    "InPost3DS",
    "(Codename: ShroomPost)",
    "",
    "",
    "PROGRAMOWANIE",
    "TehFridge",
    "",
    "",
    "",
    "GRAFIKA",
    "",
    "UI Design",
    "TehFridge",
    "",
    "InPost Chan/Kun Design",
    "Sansy (@fedora_maniac)",
    "",
    "Archon Mii Design",
    "Arkadiusz \"Dark Archon\" Kamiński",
    "",
    "Fridge Art (w Intro)",
    "Darksona",
    "Sansy (@fedora_maniac)",
    "",
    "",
    "",
    "UDŹWIĘKOWIENIE",
    "",
    "InPost Chan VA",
    "Sansy (@fedora_maniac)",
    "",
    "InPost Kun VA",
    "Nishi Yuuma",
    "",
    "Archon VA",
    "Arkadiusz \"Dark Archon\" Kamiński",
    "",
    "Banner Jingle",
    "Chiyoko Music",
    "",
    "Main Menu Music",
    "John \"Joy\" Tay",
    "",
    "Credits Music",
    "Takenobu Mitsuyoshi",
    "Naofumi Hataya",
    "Tomoko Sasaki",
    "",
    "",
    "INPOST3DS BETATESTING TEAM",
    "",
    "MrowA Matsuri",
    "Vahn Blackwood",
    "igordes",
    "Chiyoko",
    "Szprink",
    "Wafelq",
    "mmaacio",
    "fifi_sc",
    "Reisu",
    "Plesio",
    "niishee",
    "",
    "Additional pozdrowienia dla:",
    "akirsonki",
    "Jesteś cool girl",
    "",
    "gencii",
    "Dzięki za ładowarke",
    "oraz fanarcika <3",
    "",
    "i BartkaGM",
    "Poprostu jesteś cool B)", 
    "",
    "Serdeczne podziękowania dla",
    "wszystkich którzy wspierali",
    "mnie podczas tworzenia tego",
    "projektu i w życiu prywatnym.",
    ":D",
    "",
    "Projekt korzysta z API Aplikacji",
    "InPost Mobile",
    "",
    "InPost nie jest w żaden sposób",
    "powiązany z tym projektem.",
    "",
    "Wszystkie znaki towarowe",
    "należą do ich właścicieli.",
    "",
    ""
};
#define CREDIT_LINE_COUNT (sizeof(creditLines)/sizeof(creditLines[0]))

void drawCaptureWaveform(float alphaVal) {
    if (!captureBuf) return;
    if (alphaVal <= 0.0f) return; 

    GSPGPU_InvalidateDataCache(captureBuf, CAPTURE_SIZE);

    s16* pcmData = (s16*)captureBuf;
    
    
    
    bool hasSound = false;
    for(int i=0; i<100; i++) {
        if(pcmData[i] > 100 || pcmData[i] < -100) { 
            hasSound = true;
            break;
        }
    }


    float prevX = 0.0f;
    float prevY = 120.0f;
    int step = 1;

    musicisplaying = false;
    
    u8 lineAlpha = (u8)((178.0f * alphaVal) / 255.0f);
    u8 shadowAlpha = (u8)alphaVal;

    u32 lineColor = C2D_Color32(192, 255, 0, lineAlpha); 
    u32 shadowColor = C2D_Color32(0, 0, 0, shadowAlpha);

    for (int x = 0; x < 400; x+=2) {
        if (x * step >= (CAPTURE_SIZE / 2)) break;

        s16 sample = pcmData[x * step];
        float normalized = (float)sample / 32768.0f;
        
        float currentY = 120.0f + (normalized * 80);
        float currentX = (float)x;

        if (x > 0) {
            C2D_DrawLine(
                prevX, prevY, shadowColor, 
                currentX, currentY + 3.0f, shadowColor,
                2.0f, 0.5f 
            );
            C2D_DrawLine(
                prevX, prevY, lineColor,   
                currentX, currentY, lineColor,
                2.0f, 0.5f 
            );
        }
        prevX = currentX;
        prevY = currentY;
    }
}

void sceneCreditsInit(void) {
    
    sceneState = STATE_FLASH;
    flashAlpha = 255.0f;
    logoAlpha = 0.0f;
    waveAlpha = 0.0f; 
    scrollY = 480.0f; 
    
    stateTimer = 0;
    sceneStartTime = osGetTime(); 
    musicStarted = false;

    
    creditBuf = C2D_TextBufNew(4096);
    for (int i = 0; i < CREDIT_LINE_COUNT; i++) {
        C2D_TextParse(&creditText[i], creditBuf, creditLines[i]);
        C2D_TextOptimize(&creditText[i]);
    }

    
    C2D_TextParse(&exitText, creditBuf, "Wciśnij (B) by wyjść");
    C2D_TextOptimize(&exitText);
}

void sceneCreditsUpdate(uint32_t kDown, uint32_t kHeld) {
    
    if (kDown & (KEY_B | KEY_START)) {
        sceneManagerSwitchTo(SCENE_HOME_MENU); 
        return; 
    }

    
    
    if (!musicStarted) {
        if (osGetTime() - sceneStartTime >= 1500) {
            playCwav(83, true); 
            musicStarted = true;
        }
    }

    

    
    if (sceneState == STATE_FLASH) {
        if (flashAlpha > 0.0f) {
            flashAlpha -= 5.0f; 
            if (flashAlpha < 0.0f) flashAlpha = 0.0f;
        } else {
            sceneState = STATE_LOGO_FADE;
        }
    }

    
    if (sceneState == STATE_LOGO_FADE) {
        if (logoAlpha < 255.0f) {
            logoAlpha += 2.0f; 
            if (logoAlpha > 255.0f) logoAlpha = 255.0f;
        } else {
            sceneState = STATE_WAIT;
            stateTimer = osGetTime();
        }
    }

    
    if (sceneState == STATE_WAIT) {
        if (osGetTime() - stateTimer >= 1500) { 
            sceneState = STATE_SCROLL;
        }
    }

    
    if (sceneState == STATE_SCROLL) {
        scrollY -= 0.5f; 
        
        
        if (waveAlpha < 255.0f) {
            waveAlpha += 2.0f; 
            if (waveAlpha > 255.0f) waveAlpha = 255.0f;
        }

        
        if (logoAlpha > 50.0f) logoAlpha -= 0.5f; 
    }
}


void drawTopContent(float offset, float alphaFade) {
    
    if (logo3ds.subtex) {
        float logoX = (400 - logo3ds.subtex->width) / 2;
        float logoY = (240 - logo3ds.subtex->height) / 2 - 20;

        C2D_ImageTint tint;
        C2D_AlphaImageTint(&tint, alphaFade); 
        
        
        C2D_DrawImageAt(logo3ds, 0, 0, 0.5f, &tint, 1.0f, 1.0f);
    }
    drawCaptureWaveform(waveAlpha);
    
    if (sceneState == STATE_SCROLL) {
        float currentY = scrollY; 
        
        for (int i = 0; i < CREDIT_LINE_COUNT; i++) {
            float width, height;
            C2D_TextGetDimensions(&creditText[i], 0.7f, 0.7f, &width, &height);
            
            
            float x = (400 - width) / 2;
            
            
            if (currentY + height > 0 && currentY < 240) {
                C2D_DrawText(&creditText[i], C2D_WithColor, x + offset, currentY, 0.6f, 0.7f, 0.7f, C2D_Color32(255, 255, 255, 255));
            }
            currentY += height + 10.0f;
        }
    }
}


void drawBottomContent(void) {
    if (sceneState == STATE_SCROLL) {
        
        
        
        float currentY = scrollY - 240.0f; 
        
        for (int i = 0; i < CREDIT_LINE_COUNT; i++) {
            float width, height;
            C2D_TextGetDimensions(&creditText[i], 0.7f, 0.7f, &width, &height);
            
            
            float x = (320 - width) / 2;
            
            
            if (currentY + height > 0 && currentY < 240) {
                C2D_DrawText(&creditText[i], C2D_WithColor, x, currentY, 0.6f, 0.7f, 0.7f, C2D_Color32(255, 255, 255, 255));
            }
            currentY += height + 10.0f;
        }
    }

    
    
    if (sceneState != STATE_FLASH) {
        
        
        
        u8 pulseAlpha = (u8)(fabs(sin(osGetTime() / 300.0)) * 255.0f);

        
        if (pulseAlpha < 50) pulseAlpha = 50;

        u32 exitColor = C2D_Color32(255, 255, 255, pulseAlpha);
        u32 exitShadow = C2D_Color32(235, 163, 7, pulseAlpha);

        
        
        drawShadowedText(&exitText, 160.0f, 220.0f, 0.9f, 0.5f, 0.5f, exitColor, exitShadow);
    }
}

void sceneCreditsRender(void) {
    float logoAlphaVal = logoAlpha / 255.0f;

    
    C2D_SceneBegin(left); 
    C2D_TargetClear(left, C2D_Color32(0, 0, 0, 255));
    drawTopContent(0.0f, logoAlphaVal);
    
    if (flashAlpha > 0.0f) {
        C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, C2D_Color32(255, 255, 255, (u8)flashAlpha));
    }

    
    if (slider > 0.0f) {
        C2D_SceneBegin(right);
        C2D_TargetClear(right, C2D_Color32(0, 0, 0, 255));
        drawTopContent(slider * -5.0f, logoAlphaVal); 
        
        if (flashAlpha > 0.0f) {
            C2D_DrawRectSolid(0, 0, 1.0f, 400, 240, C2D_Color32(255, 255, 255, (u8)flashAlpha));
        }
    }

    
    C2D_SceneBegin(bottom); 
    C2D_TargetClear(bottom, C2D_Color32(0, 0, 0, 255)); 
    drawBottomContent();
    
    if (flashAlpha > 0.0f) {
        C2D_DrawRectSolid(0, 0, 0, 320, 240, C2D_Color32(255, 255, 255, (u8)flashAlpha));
    }
}

void sceneCreditsExit(void) {
    if (creditBuf) C2D_TextBufDelete(creditBuf);
    musicisplaying = false;
    stopCwav(83);
}