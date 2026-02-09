#include <3ds.h>
#include <citro2d.h>
#include <math.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <malloc.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <fcntl.h>
#include <time.h>

#include "scene_init.h"
#include "scene_manager.h"
#include "main.h"
#include "sprites.h"
#include "inpost_api.h" 

int VOICEACT = 0;
bool VA_YES = false;


#define SYSCLOCK_ARM11 268123480.0
#define MAKE_IPV4(a,b,c,d) ((a) << 24 | (b) << 16 | (c) << 8 | (d))
#define NTP_IP MAKE_IPV4(51, 137, 137, 111) 

typedef struct NtpPacket {
    u8 li_vn_mode; u8 stratum; u8 poll; u8 precision;
    u32 rootDelay; u32 rootDispersion; u32 refId;
    u32 refTm_s; u32 refTm_f;
    u32 origTm_s; u32 origTm_f;
    u32 rxTm_s; u32 rxTm_f;
    u32 txTm_s; u32 txTm_f;
} NtpPacket;

typedef enum {
    STATE_CHECK_WIFI,
    STATE_NO_WIFI,
    STATE_TEST_CONNECTION, 
    STATE_ATTEMPT_NTP,     
    STATE_WAIT_NTP,        
    STATE_CERT_ERROR,      
    STATE_ANIM_IN,
    STATE_WAIT_INPUT,
    STATE_ANIM_OUT,
    STATE_DONE
} InitState;

static InitState currentState;
static C2D_TextBuf staticTextBuf;
static C2D_Text txtNoWifi, txtCertError, txtNtpFixing, txtTitle, txtOptA, txtOptX, txtOptY, txtOptB;

static float animTimer = 0.0f;
static float animFactor = 0.0f; 
static const float ANIM_SPEED = 0.05f;
static bool testRequestSent = false;
static bool ntpAttempted = false; 


static int ntpSock = -1;
static u64 ntpStartTick = 0;
static u64 ntpTimeoutStart = 0;

static float easeOutBack(float t) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1.0f;
    float f = t - 1.0f;
    return 1.0f + c3 * (f * f * f) + c1 * (f * f);
}

static float easeInBack(float x) {
    const float c1 = 1.70158f;
    const float c3 = c1 + 1;
    return c3 * x * x * x - c1 * x * x;
}

static void onSelectionFinished(void) { }


#define NTP_TO_Y2K_SECONDS 3155673600ULL

// implemntation based on the one in luma3ds
static Result setSystemTime(u64 msSince1900) {
    Result res = 0;
    u64 msFrom1900to2000 = NTP_TO_Y2K_SECONDS * 1000ULL;

    if (msSince1900 < msFrom1900to2000) return -1; 
    s64 msY2k = (s64)(msSince1900 - msFrom1900to2000);

    
    if (R_FAILED(res = ptmSysmInit())) return res;
    if (R_FAILED(res = ptmSetsInit())) { ptmSysmExit(); return res; }
    if (R_FAILED(res = cfguInit())) { ptmSetsExit(); ptmSysmExit(); return res; }

    
    res = PTMSYSM_SetRtcTime(msY2k);
    
    
    if (R_SUCCEEDED(res)) {
        res = PTMSYSM_SetUserTime(msY2k);
    }

    
    if (R_SUCCEEDED(res)) {
        res = PTMSETS_SetSystemTime(msY2k);
    }

    cfguExit();
    ptmSetsExit();
    ptmSysmExit();
    return res;
}

static void drawRoundedBoxGradient(float x, float y, float z, float w, float h, u32 cTop, u32 cBot, float r) {
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    C2D_DrawCircleSolid(x + r, y + r, z, r, cTop);                 
    C2D_DrawCircleSolid(x + w - r, y + r, z, r, cTop);             
    C2D_DrawCircleSolid(x + r, y + h - r, z, r, cBot);             
    C2D_DrawCircleSolid(x + w - r, y + h - r, z, r, cBot);         
    C2D_DrawRectangle(x, y + r, z, r, h - 2 * r, cTop, cTop, cBot, cBot);
    C2D_DrawRectangle(x + w - r, y + r, z, r, h - 2 * r, cTop, cTop, cBot, cBot);
    C2D_DrawRectangle(x + r, y, z, w - 2 * r, h, cTop, cTop, cBot, cBot);
}

static void drawRoundedBoxSolid(float x, float y, float z, float w, float h, u32 color, float r) {
    if (r * 2 > w) r = w / 2;
    if (r * 2 > h) r = h / 2;
    C2D_DrawRectSolid(x + r, y, z, w - 2 * r, h, color);
    C2D_DrawRectSolid(x, y + r, z, r, h - 2 * r, color);
    C2D_DrawRectSolid(x + w - r, y + r, z, r, h - 2 * r, color);
    C2D_DrawCircleSolid(x + r, y + r, z, r, color);
    C2D_DrawCircleSolid(x + w - r, y + r, z, r, color);
    C2D_DrawCircleSolid(x + r, y + h - r, z, r, color);
    C2D_DrawCircleSolid(x + w - r, y + h - r, z, r, color);
}

int random_val; 

void sceneInitInit(void) {
    VA_YES = true;
    currentState = STATE_CHECK_WIFI;
    animTimer = 0.0f;
    animFactor = 0.0f;
    testRequestSent = false;
    ntpAttempted = false; 
    srand(time(NULL));

    random_val = rand() % 500;  
    staticTextBuf = C2D_TextBufNew(4096);

    C2D_TextParse(&txtNoWifi, staticTextBuf, "Brak Internetu, podłącz aplikacje do internetu");
    C2D_TextParse(&txtCertError, staticTextBuf, "CERT ERROR!\nCzas w 3DS'ie jest niepoprawny, zmień go na aktualny\nWciśnij (START) by wyjść.");
    C2D_TextParse(&txtNtpFixing, staticTextBuf, "Błąd SSL. Próba naprawy czasu (NTP)...");
    
    C2D_TextParse(&txtTitle, staticTextBuf, "Wybierz głos nawigatora głosowego");
    
    C2D_TextParse(&txtOptA, staticTextBuf, "(A) Mężczyzna");
    C2D_TextParse(&txtOptX, staticTextBuf, "(X) Kobieta");
    C2D_TextParse(&txtOptY, staticTextBuf, "(Y) Niebinarny");
    C2D_TextParse(&txtOptB, staticTextBuf, "(B) Brak");

    C2D_TextOptimize(&txtNoWifi);
    C2D_TextOptimize(&txtCertError);
    C2D_TextOptimize(&txtNtpFixing);
    C2D_TextOptimize(&txtTitle);
    C2D_TextOptimize(&txtOptA);
    C2D_TextOptimize(&txtOptX);
    C2D_TextOptimize(&txtOptY);
    C2D_TextOptimize(&txtOptB);
    
    if (access("/3ds/InPost3DS/opcje.json", F_OK) == 0) {
        json_t *jsonfl = json_load_file("/3ds/InPost3DS/opcje.json", 0, NULL);
        json_t *czyjest = json_object_get(jsonfl, "VA");

        VOICEACT = (int)json_integer_value(czyjest);
        
        if (VOICEACT < 0) VOICEACT = 0;
        if (VOICEACT > 3) VOICEACT = 3;

        json_decref(jsonfl);
    } else {
        VA_YES = false;
    }
}

void sceneInitUpdate(uint32_t kDown, uint32_t kHeld) {
    if (random_val != 213) {
        switch (currentState) {
            case STATE_CHECK_WIFI:
                if (osGetWifiStrength() > 0) {
                    currentState = STATE_TEST_CONNECTION;
                    testRequestSent = false;
                    testreq.done = false;
                    testreq.size = 0;
                    ntpAttempted = false; 
                } else {
                    currentState = STATE_NO_WIFI;
                }
                break;

            case STATE_NO_WIFI:
                if (osGetWifiStrength() > 0) {
                    currentState = STATE_TEST_CONNECTION;
                    testRequestSent = false;
                    testreq.done = false; 
                    testreq.size = 0;
                    
                    ntpAttempted = false; 
                }
                break;

            case STATE_TEST_CONNECTION:
                if (!testRequestSent) {
                    testrequest();
                    testRequestSent = true;
                }

                if (testreq.done) {
                    if (testreq.size > 5) {
                        
                        if (!VA_YES){
                            currentState = STATE_ANIM_IN;
                        } else {
                            currentState = STATE_DONE;
                            onSelectionFinished();
                        }
                        animTimer = 0.0f;
                    } else {
                        
                        if (!ntpAttempted) {
                            currentState = STATE_ATTEMPT_NTP;
                        } else {
                            currentState = STATE_CERT_ERROR;
                        }
                    }
                }
                break;

            
            case STATE_ATTEMPT_NTP:
                ntpAttempted = true; 
                
                
                ntpSock = socket(AF_INET, SOCK_DGRAM, 0);
                
                
                if (ntpSock < 0) {
                    
                    
                    currentState = STATE_CHECK_WIFI; 
                    ntpAttempted = false; 
                    break;
                }
                

                struct sockaddr_in servAddr = {0};
                servAddr.sin_family = AF_INET;
                servAddr.sin_addr.s_addr = htonl(NTP_IP);
                servAddr.sin_port = htons(123);

                int flags = fcntl(ntpSock, F_GETFL, 0);
                fcntl(ntpSock, F_SETFL, flags | O_NONBLOCK);

                NtpPacket packet = {0};
                packet.li_vn_mode = 0x1b; 

                sendto(ntpSock, &packet, sizeof(NtpPacket), 0, (struct sockaddr *)&servAddr, sizeof(servAddr));
                
                ntpStartTick = svcGetSystemTick();
                ntpTimeoutStart = osGetTime();
                currentState = STATE_WAIT_NTP;
                break;

            case STATE_WAIT_NTP:
                {
                    NtpPacket packet = {0};
                    struct sockaddr_in fromAddr;
                    socklen_t fromLen = sizeof(fromAddr);

                    int ret = recvfrom(ntpSock, &packet, sizeof(NtpPacket), 0, (struct sockaddr *)&fromAddr, &fromLen);
                    
                    if (ret > 0) {
                        u64 endTick = svcGetSystemTick();
                        close(ntpSock); 
                        ntpSock = -1;

                        u32 txSeconds = ntohl(packet.txTm_s);
                        u32 txFraction = ntohl(packet.txTm_f);

                        u64 roundTripTicks = endTick - ntpStartTick;
                        u64 dt = (u64)((1000.0 * roundTripTicks) / (SYSCLOCK_ARM11)); 

                        u64 msFraction = ((u64)txFraction * 1000ULL) >> 32;
                        u64 msSince1900 = ((u64)txSeconds * 1000ULL) + msFraction + dt;

                        Result timeRes = setSystemTime(msSince1900);
                        if (R_SUCCEEDED(timeRes)) {
                            currentState = STATE_TEST_CONNECTION; 
                        } else {
                            currentState = STATE_CERT_ERROR;
                        }

                        testRequestSent = false;
                        testreq.done = false;
                    }
                    else {
                        if (osGetTime() - ntpTimeoutStart > 5000) {
                            close(ntpSock);
                            ntpSock = -1;
                            currentState = STATE_CERT_ERROR; 
                        }
                    }
                }
                break;
            

            case STATE_CERT_ERROR:
                if (kDown & KEY_START) {
                    
                }
                break;

            case STATE_ANIM_IN:
                animTimer += ANIM_SPEED;
                if (animTimer >= 1.0f) {
                    animTimer = 1.0f;
                    currentState = STATE_WAIT_INPUT;
                }
                animFactor = easeOutBack(animTimer);
                break;

            case STATE_WAIT_INPUT:
                animFactor = 1.0f; 
                if (kDown & (KEY_A | KEY_X | KEY_Y | KEY_B)) {
                    if (kDown & KEY_A) VOICEACT = 1;
                    else if (kDown & KEY_X) VOICEACT = 2;
                    else if (kDown & KEY_Y) VOICEACT = 3;
                    else if (kDown & KEY_B) VOICEACT = 0;
                    
                    json_t *oproot = json_object();
                    json_object_set_new(oproot, "VA", json_integer(VOICEACT));
                    json_dump_file(oproot, "/3ds/InPost3DS/opcje.json", JSON_COMPACT);
                    json_decref(oproot);
                    
                    currentState = STATE_ANIM_OUT;
                    animTimer = 0.0f;
                }
                break;

            case STATE_ANIM_OUT:
                animTimer += ANIM_SPEED;
                if (animTimer >= 1.0f) {
                    animTimer = 1.0f;
                    currentState = STATE_DONE;
                    onSelectionFinished();
                }
                float exitScale = easeInBack(animTimer);
                animFactor = 1.0f - exitScale;
                if (animFactor < 0.0f) animFactor = 0.0f;
                break;

            case STATE_DONE:
                sceneManagerSwitchTo(SCENE_INTRO);
                break;
        }
    } 
}

void sceneInitRender(void) {
    float screenW = 400.0f;
    float screenH = 240.0f;
    C2D_SceneBegin(left);
    C2D_TargetClear(left, C2D_Color32(0, 0, 0, 255));
    
    if (currentState == STATE_NO_WIFI) {
        float textW = 0, textH = 0;
        C2D_TextGetDimensions(&txtNoWifi, 0.5f, 0.5f, &textW, &textH);
        C2D_DrawText(&txtNoWifi, C2D_WithColor, (screenW - textW)/2, (screenH - textH)/2, 0, 0.5f, 0.5f, C2D_Color32(255, 0, 0, 255));
        return; 
    }

    if (currentState == STATE_CERT_ERROR) {
        float textW = 0, textH = 0;
        C2D_TextGetDimensions(&txtCertError, 0.5f, 0.5f, &textW, &textH);
        C2D_DrawText(&txtCertError, C2D_WithColor, (screenW - textW)/2, (screenH - textH)/2, 0, 0.5f, 0.5f, C2D_Color32(255, 0, 0, 255));
        return;
    }

    
    if (currentState == STATE_ATTEMPT_NTP || currentState == STATE_WAIT_NTP) {
        float textW = 0, textH = 0;
        C2D_TextGetDimensions(&txtNtpFixing, 0.5f, 0.5f, &textW, &textH);
        C2D_DrawText(&txtNtpFixing, C2D_WithColor, (screenW - textW)/2, (screenH - textH)/2, 0, 0.5f, 0.5f, C2D_Color32(255, 255, 0, 255));
        return;
    }

    if (currentState >= STATE_ANIM_IN && currentState <= STATE_ANIM_OUT) {
        float alphaFactor = (currentState == STATE_ANIM_OUT) ? animFactor : (animTimer > 1.0f ? 1.0f : animTimer); 
        u8 dimAlpha = (u8)(150.0f * alphaFactor);
        C2D_DrawRectSolid(0, 0, 0.5f, screenW, screenH, C2D_Color32(0, 0, 0, dimAlpha));
        
        float finalBoxW = 320.0f;
        float finalBoxH = 180.0f;

        float curW = finalBoxW * animFactor;
        float curH = finalBoxH * animFactor;
        
        float curX = (screenW - curW) / 2.0f;
        float curY = (screenH - curH) / 2.0f;
        
        u32 clrBorder = C2D_Color32(255, 255, 255, 255); 
        u32 clrBgTop = C2D_Color32(45, 45, 20, 255); 
        u32 clrBgBot = C2D_Color32(5, 5, 5, 255);    
        
        float cornerRadius = 5.0f * animFactor; 
        float borderThickness = 3.0f;

        if (curW > cornerRadius * 2) {
            drawRoundedBoxSolid(curX, curY, 0.6f, curW, curH, clrBorder, cornerRadius);
            drawRoundedBoxGradient(
                curX + borderThickness, 
                curY + borderThickness, 
                0.6f, 
                curW - (borderThickness * 2), 
                curH - (borderThickness * 2), 
                clrBgTop,
                clrBgBot, 
                cornerRadius - 1.0f
            );
        }

        float textScaleBase = 0.5f; 
        float curTextScale = textScaleBase * animFactor;
        
        if (curTextScale > 0.01f) {
            float wT, hT;
            C2D_TextGetDimensions(&txtTitle, curTextScale, curTextScale, &wT, &hT);
            u32 clrTitle = C2D_Color32(255, 235, 59, 255);

            C2D_DrawText(&txtTitle, C2D_WithColor, 
                (screenW - wT)/2.0f, 
                curY + (15.0f * animFactor), 
                0.7f, curTextScale, curTextScale, clrTitle);

            float optScale = 0.6f * animFactor;
            float startY = curY + (60.0f * animFactor);
            float gapY = 28.0f * animFactor;
            u32 clrText = C2D_Color32(255, 255, 255, 255);

            float wX, hX;
            C2D_TextGetDimensions(&txtOptX, optScale, optScale, &wX, &hX);
            C2D_DrawText(&txtOptX, C2D_WithColor, (screenW - wX)/2, startY, 0.7f, optScale, optScale, clrText);

            float wA, hA;
            C2D_TextGetDimensions(&txtOptA, optScale, optScale, &wA, &hA);
            C2D_DrawText(&txtOptA, C2D_WithColor, (screenW - wA)/2, startY + gapY, 0.7f, optScale, optScale, clrText);

            float wY_dim, hY_dim;
            C2D_TextGetDimensions(&txtOptY, optScale, optScale, &wY_dim, &hY_dim);
            C2D_DrawText(&txtOptY, C2D_WithColor, (screenW - wY_dim)/2, startY + gapY*2, 0.7f, optScale, optScale, clrText);

            float wB, hB;
            C2D_TextGetDimensions(&txtOptB, optScale, optScale, &wB, &hB);
            C2D_DrawText(&txtOptB, C2D_WithColor, (screenW - wB)/2, startY + gapY*3, 0.7f, optScale, optScale, clrText);
        }
    }
    C2D_SceneBegin(bottom);
    C2D_TargetClear(bottom, C2D_Color32(0, 0, 0, 255));
    if (random_val == 213) {
        C2D_DrawImageAt(sie_do_wiezienia, 0.0f, 0.0f, 0.5f, NULL, 1.0f, 1.0f);
    }
}

void sceneInitExit(void) {
    if (ntpSock >= 0) {
        close(ntpSock);
    }
    C2D_TextBufDelete(staticTextBuf);
}