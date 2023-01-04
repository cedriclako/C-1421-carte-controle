#include "CommonUI.h"

const EF_SFile* COMMONUI_GetBtnArrowUp(bool bIsEnabled)
{
    return bIsEnabled ? &EF_g_sFiles[EF_EFILE_ICON_ARROW_UP_EN_120X60_JPG] : &EF_g_sFiles[EF_EFILE_ICON_ARROW_UP_DISA_120X60_JPG];
}

const EF_SFile* COMMONUI_GetBtnArrowDown(bool bIsEnabled)
{
    return bIsEnabled ? &EF_g_sFiles[EF_EFILE_ICON_ARROW_DOWN_EN_120X60_JPG] : &EF_g_sFiles[EF_EFILE_ICON_ARROW_DOWN_DISA_120X60_JPG];
}    
