#include <citro2d.h>
#include <3ds.h>
#include <3ds/synchronization.h>
#include <cwav.h>
#include <ncsnd.h>
#include <jansson.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <malloc.h>
#include "logs.h"
#include "buttons.h"
#include "cwav_shit.h"
#include "scene_manager.h"
#include "scene_debug.h"
#include "main.h"
#include "sprites.h"
#include "request.h"


#define FRAME_MS (1000.0f / 60.0f)
#define MAX_SPRITES   1
#define SCREEN_WIDTH  400
#define SCREEN_HEIGHT 240
#define SOC_ALIGN       0x1000
#define SOC_BUFFERSIZE  0x300000


#define fmin(a, b) ((a) < (b) ? (a) : (b))
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))

struct memory {
    char *response;
    size_t size;
};

bool cpu_debug = false;
extern bool logplz, readingoferta, json_done, citra_machen;
extern float text_w, text_h, max_scroll;

extern int cwavCount;
extern Button buttonsy[100];
size_t linear_bytes_used = 0;
bool pausedForSleep = false;
int Scene;
int selectioncodelol;
bool przycskmachen = true;
bool generatingQR = false;
char combinedText[128];
static u32 *SOC_buffer = NULL;

C2D_TextBuf totpBuf;
C2D_TextBuf g_staticBuf, kupon_text_Buf, themeBuf;
C2D_Text g_staticText[100];
C2D_Text themeText[100];
C2D_Text g_totpText[5];

C3D_RenderTarget* left;
C3D_RenderTarget* right;
C3D_RenderTarget* bottom;

u8* captureBuf = NULL;
ndspWaveBuf captureWaveBuf; 
const u32 CAPTURE_SIZE = 0x340; 

float slider;
bool saving_va = false;
bool debug_stats = false;
void executeButtonFunction(int buttonIndex) {
    if (buttonIndex >= 0 && buttonIndex < 100 && buttonsy[buttonIndex].onClick) {
        buttonsy[buttonIndex].onClick();
    } else {
        log_to_file("Invalid button index or function not assigned!\n");
    }
}
Result czyFolderIstnieje(FS_Archive archive, FS_Path path)
{
    Handle dirHandle = 0;
    Result res;

    res = FSUSER_OpenDirectory(&dirHandle, archive, path);

    if (R_SUCCEEDED(res))
    {
        FSDIR_Close(dirHandle);
        return 0; 
    }

    res = FSUSER_CreateDirectory(archive, path, 0);

    return res;
}
int main(int argc, char* argv[]) {
    cwavUseEnvironment(CWAV_ENV_DSP);
    romfsInit();
    cfguInit();
    gfxInitDefault();
    PrintConsole topConsole;
    consoleInit(GFX_TOP, &topConsole);
    ndspInit();
    init_logger();
    doing_debug_logs = false;
    
    SOC_buffer = (u32*)memalign(SOC_ALIGN, SOC_BUFFERSIZE);
    if (!SOC_buffer || socInit(SOC_buffer, SOC_BUFFERSIZE)) {
        printf("SOC init failed\n");
    }

    totpBuf = C2D_TextBufNew(128);
    g_staticBuf = C2D_TextBufNew(256);
    kupon_text_Buf = C2D_TextBufNew(512);
    themeBuf = C2D_TextBufNew(512);
    
    
    C2D_TextBuf memBuf = C2D_TextBufNew(1024); 
    C2D_Text memtext[6]; 
    
    
    memset(memtext, 0, sizeof(memtext));

    consoleClear();
    start_request_thread();
    gfxExit();
    
    gfxInitDefault();
    gfxSet3D(false); 
    osSetSpeedupEnable(true);
    
    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();
    
    left = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    right = C2D_CreateScreenTarget(GFX_TOP, GFX_RIGHT);
    bottom = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    
    initCwavSystem();
    Scene = 0;
    captureBuf = linearAlloc(CAPTURE_SIZE);
    
    memset(&captureWaveBuf, 0, sizeof(ndspWaveBuf)); 
    captureWaveBuf.data_vaddr = captureBuf;         
    captureWaveBuf.nsamples = CAPTURE_SIZE / 4;      
    captureWaveBuf.looping = true;                   
    
    ndspSetCapture(&captureWaveBuf);   
    spritesInit();

    sceneManagerSwitchTo(SCENE_INIT);

 
    float cpu_avg = 0.0f;
    float gpu_avg = 0.0f;
    const float smoothing = 0.05f; 

    u64 lastTick = svcGetSystemTick();
    int frameCount = 0;
    float currentFps = 0.0f;
    
    
    int statsUpdateTimer = 16; 

    
    C2D_TextBufClear(memBuf);
    fsInit();
    FS_Archive sdmcArchive;
    FSUSER_OpenArchive(&sdmcArchive, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, ""));
    FS_Path folderPath = fsMakePath(PATH_ASCII, "/3ds/InPost3DS");
    Result res = czyFolderIstnieje(sdmcArchive, folderPath);

    FSUSER_CloseArchive(sdmcArchive);
    fsExit();

    while (aptMainLoop()) {
        hidScanInput();
        u32 kDown = hidKeysDown();
        u32 kHeld = hidKeysHeld();
        slider = osGet3DSliderState();

        if ((kDown & KEY_START) || saving_va)
            break;
        #ifdef DEBUG
            if ((kDown & KEY_Y) && debugmode){
                debug_stats = !debug_stats;
                logplz = !logplz;
            }
        #endif

        sceneManagerUpdate(kDown, kHeld);

        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        sceneManagerRender();

        if (debug_stats) {
            C2D_SceneBegin(left);

            
            float rawCpu = C3D_GetProcessingTime() * FRAME_MS;
            float rawGpu = C3D_GetDrawingTime() * FRAME_MS;

            
            cpu_avg = (rawCpu * smoothing) + (cpu_avg * (1.0f - smoothing));
            gpu_avg = (rawGpu * smoothing) + (gpu_avg * (1.0f - smoothing));

            
            frameCount++;
            u64 currentTick = svcGetSystemTick();
            
            
            if (currentTick - lastTick >= CPU_TICKS_PER_MSEC * 1000) {
                currentFps = (float)frameCount;
                frameCount = 0;
                lastTick = currentTick;
            }

            
            float startX = 20.0f;
            float startY = 20.0f;
            float barMaxWidth = 260.0f;
            float barHeight = 5.0f;
            
            statsUpdateTimer++;
            if (statsUpdateTimer > 15) {
                statsUpdateTimer = 0;
                C2D_TextBufClear(memBuf); 
                char buf[64];

                
                snprintf(buf, sizeof(buf), "CPU: %.2f ms", cpu_avg);
                C2D_TextParse(&memtext[0], memBuf, buf);
                C2D_TextOptimize(&memtext[0]);

                
                snprintf(buf, sizeof(buf), "GPU: %.2f ms", gpu_avg);
                C2D_TextParse(&memtext[1], memBuf, buf);
                C2D_TextOptimize(&memtext[1]);

                
                snprintf(buf, sizeof(buf), "FPS: %.1f", currentFps);
                C2D_TextParse(&memtext[2], memBuf, buf);
                C2D_TextOptimize(&memtext[2]);

                
                
                double sysUsed = osGetMemRegionUsed(MEMREGION_APPLICATION) / 1048576.0;
                double sysTotal = (osGetMemRegionUsed(MEMREGION_APPLICATION) + osGetMemRegionFree(MEMREGION_APPLICATION)) / 1048576.0;
                
                snprintf(buf, sizeof(buf), "SYS: %.1f / %.1f MB", sysUsed, sysTotal);
                C2D_TextParse(&memtext[3], memBuf, buf);
                C2D_TextOptimize(&memtext[3]);

                
                
                struct mallinfo mi = mallinfo();
                double heapUsed = mi.uordblks / 1048576.0;
                

                snprintf(buf, sizeof(buf), "HEAP: %.2f MB", heapUsed);
                C2D_TextParse(&memtext[4], memBuf, buf);
                C2D_TextOptimize(&memtext[4]);
                double linUsed = linear_bytes_used / 1048576.0;
                snprintf(buf, sizeof(buf), "LIN: %.2f MB", linUsed);
                C2D_TextParse(&memtext[5], memBuf, buf);
                C2D_TextOptimize(&memtext[5]);
            }

            
            
            
            
            float cpuRatio = cpu_avg / FRAME_MS;
            if (cpuRatio > 1.0f) cpuRatio = 1.0f;
            C2D_DrawRectSolid(startX, startY, 1.0f, startX + (cpuRatio * barMaxWidth), barHeight, C2D_Color32(255, 255, 0, 255));
            
            
            C2D_DrawRectSolid(18, 22, 1.0f, 100, 14, C2D_Color32(255, 255, 255, 255));
            C2D_DrawText(&memtext[0], C2D_AlignLeft | C2D_WithColor, 20, 25, 1.0f, 0.4f, 0.4f, C2D_Color32(0, 0, 0, 255));

            
            float gpuRatio = gpu_avg / FRAME_MS;
            if (gpuRatio > 1.0f) gpuRatio = 1.0f;
            C2D_DrawRectSolid(startX, startY + 30, 1.0f, startX + (gpuRatio * barMaxWidth), barHeight, C2D_Color32(255, 255, 0, 255));

            
            C2D_DrawRectSolid(18, 52, 1.0f, 100, 14, C2D_Color32(255, 255, 255, 255));
            C2D_DrawText(&memtext[1], C2D_AlignLeft | C2D_WithColor, 20, 55, 1.0f, 0.4f, 0.4f, C2D_Color32(0, 0, 0, 255));

            
            C2D_DrawRectSolid(330, 22, 1.0f, 60, 14, C2D_Color32(255, 255, 255, 255));
            C2D_DrawText(&memtext[2], C2D_AlignLeft | C2D_WithColor, 335, 25, 1.0f, 0.4f, 0.4f, C2D_Color32(0, 0, 0, 255));
            

            
            C2D_DrawRectSolid(280, 52, 1.0f, 110, 14, C2D_Color32(255, 255, 255, 255));
            C2D_DrawText(&memtext[3], C2D_AlignLeft | C2D_WithColor, 285, 55, 1.0f, 0.4f, 0.4f, C2D_Color32(0, 0, 0, 255));

            
            C2D_DrawRectSolid(280, 72, 1.0f, 110, 14, C2D_Color32(255, 255, 255, 255));
            C2D_DrawText(&memtext[4], C2D_AlignLeft | C2D_WithColor, 285, 75, 1.0f, 0.4f, 0.4f, C2D_Color32(0, 0, 0, 255));
            
            C2D_DrawRectSolid(280, 92, 1.0f, 110, 14, C2D_Color32(255, 255, 255, 255));
            C2D_DrawText(&memtext[5], C2D_AlignLeft | C2D_WithColor, 285, 95, 1.0f, 0.4f, 0.4f, C2D_Color32(0, 0, 0, 255));

        }

        C3D_FrameEnd(0);
    }

    close_logger();

    C2D_TextBufDelete(g_staticBuf);
    C2D_TextBufDelete(kupon_text_Buf);
    C2D_TextBufDelete(themeBuf);
    C2D_TextBufDelete(totpBuf);
    C2D_TextBufDelete(memBuf);
    socExit();
    if (SOC_buffer) free(SOC_buffer); 

    C2D_Fini();
    C3D_Fini();
    ndspExit();
    cfguExit();
    romfsExit();
    gfxExit();

    return 0;
}