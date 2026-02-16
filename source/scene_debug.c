#include <3ds.h>
#include <citro2d.h>
#include "scene_debug.h"
#include "scene_manager.h"
#include "main.h"
#include "sprites.h"
#include "inpost_api.h"
#include "text.h"

#define DEBUG_MENU_ITEMS 6

bool debugmode = false;
int debugMenuSelected = 0;

const float menuStartY = 100.0f;
const float menuLineHeight = 28.0f;

bool paczkashow = false;
bool fake_paczka_mode = false;

C2D_TextBuf DebugMenu;
C2D_Text debugmenu_Text[50];


void RebuildDebugMenuStaticText(void) {
    C2D_TextParse(&debugmenu_Text[0], DebugMenu, "INPOST3DS debug menu");
    C2D_TextParse(&debugmenu_Text[1], DebugMenu, "Zbierz dane do testów (logi)");
    C2D_TextParse(&debugmenu_Text[2], DebugMenu, "Odśwież token (/v1/authenticate)");
    C2D_TextParse(&debugmenu_Text[3], DebugMenu, "Pokaż Fake'owe paczki (romfs://test_data.json)");
    C2D_TextParse(&debugmenu_Text[4], DebugMenu, "Pokaż prawdziwe paczki (/parcels/tracked)");
    C2D_TextParse(&debugmenu_Text[5], DebugMenu, "Tutorial Menu");
    C2D_TextParse(&debugmenu_Text[10], DebugMenu, ">");
    
    C2D_TextOptimize(&debugmenu_Text[10]);
    for (int i = 0; i < 6; i++) 
        C2D_TextOptimize(&debugmenu_Text[i]);
}

void sceneDebugInit(void) {
    DebugMenu = C2D_TextBufNew(4096); 
    C2D_TextBufClear(DebugMenu);
    RebuildDebugMenuStaticText();
}

void sceneDebugUpdate(uint32_t kDown, uint32_t kHeld) {
    if (kDown & KEY_DOWN) {
        debugMenuSelected++;
        if (debugMenuSelected >= DEBUG_MENU_ITEMS)
            debugMenuSelected = 0;
    }

    if (kDown & KEY_UP) {
        debugMenuSelected--;
        if (debugMenuSelected < 0)
            debugMenuSelected = DEBUG_MENU_ITEMS - 1;
    }

    if (kDown & KEY_A) {
        switch (debugMenuSelected) {
            case 0:
                getEverything();
                break;

            case 1:
                refresh_the_Token(refreshToken);
                break;

            case 2: 
                paczkashow = false;
                
                
                C2D_TextBufClear(DebugMenu);
                RebuildDebugMenuStaticText(); 
                
                
                parseFakePaczkas();
                
                if (paczka_count > 0) {
                    C2D_TextParse(&debugmenu_Text[11], DebugMenu, paczka_list[0].pickupPointName);
                    C2D_TextParse(&debugmenu_Text[12], DebugMenu, paczka_list[0].city);
                    C2D_TextParse(&debugmenu_Text[13], DebugMenu, paczka_list[0].status);
                    C2D_TextParse(&debugmenu_Text[14], DebugMenu, paczka_list[0].shipmentNumber);
                }
                if (paczka_count > 1) {
                    C2D_TextParse(&debugmenu_Text[15], DebugMenu, paczka_list[1].pickupPointName);
                    C2D_TextParse(&debugmenu_Text[16], DebugMenu, paczka_list[1].city);
                    C2D_TextParse(&debugmenu_Text[17], DebugMenu, paczka_list[1].status);
                    C2D_TextParse(&debugmenu_Text[18], DebugMenu, paczka_list[1].shipmentNumber);
                }
                paczkashow = true;
                break;

            case 3: 
                paczkashow = false;

                
                C2D_TextBufClear(DebugMenu);
                RebuildDebugMenuStaticText();
                

                parsePaczkas((const char*)paczkas.data);

                if (paczka_count > 0) {
                    C2D_TextParse(&debugmenu_Text[11], DebugMenu, paczka_list[0].pickupPointName);
                    C2D_TextParse(&debugmenu_Text[12], DebugMenu, paczka_list[0].city);
                    C2D_TextParse(&debugmenu_Text[13], DebugMenu, paczka_list[0].status);
                    C2D_TextParse(&debugmenu_Text[14], DebugMenu, paczka_list[0].shipmentNumber);
                }
                if (paczka_count > 1) {
                    C2D_TextParse(&debugmenu_Text[15], DebugMenu, paczka_list[1].pickupPointName);
                    C2D_TextParse(&debugmenu_Text[16], DebugMenu, paczka_list[1].city);
                    C2D_TextParse(&debugmenu_Text[17], DebugMenu, paczka_list[1].status);
                    C2D_TextParse(&debugmenu_Text[18], DebugMenu, paczka_list[1].shipmentNumber);
                }
                paczkashow = true;
                break;

            case 4:
                sceneManagerSwitchTo(SCENE_TUTORIAL);
                break;
        }
    }
}

void sceneDebugRender(void) {
    C2D_SceneBegin(left);
    C2D_TargetClear(left, C2D_Color32(0, 0, 0, 255));

    
    drawShadowedText(
        &debugmenu_Text[0],
        200.0f, 20.0f, 1.0f, 1.0f, 1.0f,
        C2D_Color32(0xB1, 0xA2, 0x2F, 0xff),
        C2D_Color32(0xff, 0xff, 0xff, 0xff)
    );

    
    for (int i = 0; i < DEBUG_MENU_ITEMS; i++) {
        float y = menuStartY + (i * menuLineHeight);

        
        if (i == debugMenuSelected) {
            C2D_DrawText(
                &debugmenu_Text[10],
                C2D_AtBaseline | C2D_WithColor,
                15.0f, y,
                0.5f, 0.5f, 0.5f,
                C2D_Color32(255, 255, 255, 255)
            );
        }

        u32 color = (i == debugMenuSelected)
            ? C2D_Color32(255, 255, 0, 255)
            : C2D_Color32(200, 200, 200, 255);

        C2D_DrawText(
            &debugmenu_Text[i + 1],
            C2D_AtBaseline | C2D_WithColor,
            30.0f, y,
            0.5f, 0.5f, 0.5f,
            color
        );
    }

    C2D_SceneBegin(bottom);
    C2D_TargetClear(bottom, C2D_Color32(0, 0, 0, 255));
    
    if (paczkashow) {
        
        if (paczka_count > 0) {
            float y = 15.0f;
            for(int k=11; k<=14; k++) {
                C2D_DrawText(&debugmenu_Text[k], C2D_AtBaseline | C2D_WithColor, 15.0f, y, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
                y += 15.0f;
            }
        }
        if (paczka_count > 1) {
            float y = 90.0f;
            for(int k=15; k<=18; k++) {
                C2D_DrawText(&debugmenu_Text[k], C2D_AtBaseline | C2D_WithColor, 15.0f, y, 0.5f, 0.5f, 0.5f, C2D_Color32(255, 255, 255, 255));
                y += 15.0f;
            }
        }
    }
}

void sceneDebugExit(void) {
    C2D_TextBufDelete(DebugMenu);
}