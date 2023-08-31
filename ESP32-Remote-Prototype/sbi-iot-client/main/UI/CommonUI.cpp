#include "CommonUI.hpp"

static bool IsInCoordinate(int32_t s32X, int32_t s32Y, int32_t s32Width, int32_t s32Height, int32_t s32TouchX, int32_t s32TouchY);

const EF_SFile* COMMONUI_GetBtnArrowUp(bool bIsEnabled)
{
    return bIsEnabled ? &EF_g_sFiles[EF_EFILE_ICON_ARROW_UP_EN_120X60_JPG] : &EF_g_sFiles[EF_EFILE_ICON_ARROW_UP_DISA_120X60_JPG];
}

const EF_SFile* COMMONUI_GetBtnArrowDown(bool bIsEnabled)
{
    return bIsEnabled ? &EF_g_sFiles[EF_EFILE_ICON_ARROW_DOWN_EN_120X60_JPG] : &EF_g_sFiles[EF_EFILE_ICON_ARROW_DOWN_DISA_120X60_JPG];
}

bool COMMONUI_IsInCoordinate(const COMMONUI_SButton* psButton, int32_t s32TouchX, int32_t s32TouchY)
{
    return IsInCoordinate(psButton->sRect.s32X, psButton->sRect.s32Y, psButton->sRect.s32Width, psButton->sRect.s32Height, s32TouchX, s32TouchY);
}

void COMMONUI_Button_Init(COMMONUI_SButton* psButton, const EF_SFile* pSFile, int32_t s32X, int32_t s32Y)
{
    const EF_SImage* psMeta = (EF_SImage*)pSFile->pMetaData;
    psButton->pSFile = pSFile;
    psButton->sRect.s32X = s32X;
    psButton->sRect.s32Width = psMeta->s32Width;
    psButton->sRect.s32Y = s32Y;
    psButton->sRect.s32Height = psMeta->s32Height;
}

void COMMONUI_Button_Draw(const COMMONUI_SButton* psButton)
{
    G_g_CanvasResult.drawJpg(psButton->pSFile->pu8StartAddr, psButton->pSFile->u32Length, psButton->sRect.s32X, psButton->sRect.s32Y);
}

static bool IsInCoordinate(int32_t s32X, int32_t s32Y, int32_t s32Width, int32_t s32Height, int32_t s32TouchX, int32_t s32TouchY)
{
    return s32TouchX >= s32X && s32TouchX <= (s32X + s32Width) &&
           s32TouchY >= s32Y && s32TouchY <= (s32Y + s32Height);
}
