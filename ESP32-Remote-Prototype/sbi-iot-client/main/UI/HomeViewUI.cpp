#include <string.h>
#include "HomeViewUI.hpp"

#define TAG "HomeViewUI"

static void Init(COMMONUI_SContext* pContext);
static void Enter(COMMONUI_SContext* pContext);
static void Exit(COMMONUI_SContext* pContext);

static void Process(COMMONUI_SContext* pContext);
static void OnTouch(COMMONUI_SContext* pContext, int32_t s32TouchX, int32_t s32TouchY);
static void DataReceived(COMMONUI_SContext* pContext);

const COMMONUI_SConfig HOMEVIEWUI_g_sConfig = 
{ 
    .ptrInit = Init, 
    .ptrEnter = Enter, 
    .ptrExit = Exit, 

    .ptrProcess = Process,
    .ptrOnTouch = OnTouch,
    .ptrOnDataReceived = DataReceived
};

static void Init(COMMONUI_SContext* pContext)
{

}

static void Enter(COMMONUI_SContext* pContext)
{
    ESP_LOGI(TAG, "Powered Off");
    G_g_CanvasResult.setTextSize(2);
    G_g_CanvasResult.setFreeFont(FF23);

    const float fTemperatureC = M5.SHT30.GetTemperature();

    char str[20];
    snprintf(str, sizeof(str), "ROOM  %.1f°C", fTemperatureC);
    G_g_CanvasResult.drawCentreString(str, SCREEN_WIDTH/2, 90, GFXFF);

    // Flame symbols
    {
        const bool bIsBigFlame = true;

        if (bIsBigFlame)
        {
            const EF_SFile* pSFileBigFlame = &EF_g_sFiles[EF_EFILE_ICON_FLAME_224X288_JPG];
            const EF_SImage* pSFileBigFlameMeta = (EF_SImage*)pSFileBigFlame->pMetaData;
            const int32_t s32Left = SCREEN_WIDTH / 2 - pSFileBigFlameMeta->s32Width / 2;
            const int32_t s32Top = SCREEN_HEIGHT / 2 - pSFileBigFlameMeta->s32Height;
            G_g_CanvasResult.drawJpg(pSFileBigFlame->pu8StartAddr, pSFileBigFlame->u32Length, s32Left, s32Top);
        }
        else
        {
            const EF_SFile* pSFileSmallFlame = &EF_g_sFiles[EF_EFILE_ICON_FLAME_56X72_JPG];
            const EF_SImage* pSFileSmallFlameMeta = (EF_SImage*)pSFileSmallFlame->pMetaData;
            const int32_t s32Left = SCREEN_WIDTH / 2 - ((EF_SImage*)pSFileSmallFlame->pMetaData)->s32Width / 2;
            const int32_t s32Top = SCREEN_HEIGHT / 2 - pSFileSmallFlameMeta->s32Height;
            G_g_CanvasResult.drawJpg(pSFileSmallFlame->pu8StartAddr, pSFileSmallFlame->u32Length, s32Left, s32Top);
        }
    }

    {
        // Message box
        const int32_t s32Width = SCREEN_WIDTH - (20 * 2);
        const int32_t s32Height = 310;
        const int32_t s32Left = (SCREEN_WIDTH / 2) - (s32Width / 2);
        const int32_t s32Top = (SCREEN_HEIGHT - s32Height - 20);
        G_g_CanvasResult.setTextSize(1);
        G_g_CanvasResult.setFreeFont(FF23);
        G_g_CanvasResult.drawRoundRect(s32Left, s32Top, s32Width, s32Height, 20, TFT_WHITE);
        
        G_g_CanvasResult.drawCentreString("MESSAGE", SCREEN_WIDTH/2, s32Top + 20, GFXFF);
        
        // 21 characters maximum
        G_g_CanvasResult.setFreeFont(FF19);
        const int32_t s32LineHeight = 40;
        const char* szLines[] = 
        {
            "ANTICONSTITUTIONNE#01",
            "ANTICONSTITUTIONNE#02",
            "ANTICONSTITUTIONNE#03"
        };
        for(int i = 0; i < sizeof(szLines)/sizeof(szLines[0]); i++)
        {
            G_g_CanvasResult.drawCentreString(szLines[i], SCREEN_WIDTH/2, s32Top + 80 + s32LineHeight*i, GFXFF);
        }
    }

    // Push canvas
    G_g_CanvasResult.pushCanvas(0, 0, UPDATE_MODE_A2);
    G_g_CanvasResult.fillCanvas(0);
}

static void Exit(COMMONUI_SContext* pContext)
{

}

static void Process(COMMONUI_SContext* pContext)
{

}

static void OnTouch(COMMONUI_SContext* pContext, int32_t s32TouchX, int32_t s32TouchY)
{
    
}

static void DataReceived(COMMONUI_SContext* pContext)
{
    
}