#include "CommonUI.h"

const EF_SFile* COMMONUI_GetBtnArrowUp(bool bIsEnabled)
{
    return bIsEnabled ? &EF_g_sFiles[EF_EFILE_ICON_ARROW_UP_EN_120X60_JPG] : &EF_g_sFiles[EF_EFILE_ICON_ARROW_UP_DISA_120X60_JPG];
}

const EF_SFile* COMMONUI_GetBtnArrowDown(bool bIsEnabled)
{
    return bIsEnabled ? &EF_g_sFiles[EF_EFILE_ICON_ARROW_DOWN_EN_120X60_JPG] : &EF_g_sFiles[EF_EFILE_ICON_ARROW_DOWN_DISA_120X60_JPG];
}    

bool COMMONUI_IsInCoordinate(int32_t s32X, int32_t s32Y, int32_t s32Width, int32_t s32Height, int32_t s32TouchX, int32_t s32TouchY)
{
    return s32TouchX >= s32X && s32TouchX <= (s32X + s32Width) && 
           s32TouchY >= s32Y && s32TouchY <= (s32Y + s32Height);
}
