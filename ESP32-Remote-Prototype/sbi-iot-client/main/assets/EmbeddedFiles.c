#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_CLOCK_64X64_PNG                     = { .s32Width = 64, .s32Height = 64};
const EF_SImage EF_g_sIMAGES_FAN_512X512_PNG                     = { .s32Width = 512, .s32Height = 512};
const EF_SImage EF_g_sIMAGES_FAN_72X72_PNG                       = { .s32Width = 72, .s32Height = 72};
const EF_SImage EF_g_sIMAGES_FLAME_56X72_PNG                     = { .s32Width = 56, .s32Height = 72};
const EF_SImage EF_g_sIMAGES_HOME_72X72_PNG                      = { .s32Width = 72, .s32Height = 72};
const EF_SImage EF_g_sIMAGES_ICON_ARROW_BACK_EN_160X160_JPG      = { .s32Width = 160, .s32Height = 160};
const EF_SImage EF_g_sIMAGES_ICON_ARROW_DOWN_DISA_120X60_JPG     = { .s32Width = 120, .s32Height = 60};
const EF_SImage EF_g_sIMAGES_ICON_ARROW_DOWN_EN_120X60_JPG       = { .s32Width = 120, .s32Height = 60};
const EF_SImage EF_g_sIMAGES_ICON_ARROW_UP_DISA_120X60_JPG       = { .s32Width = 120, .s32Height = 60};
const EF_SImage EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG         = { .s32Width = 120, .s32Height = 60};
const EF_SImage EF_g_sIMAGES_ICON_SBI_LOGO_152X112_JPG           = { .s32Width = 152, .s32Height = 112};
const EF_SImage EF_g_sIMAGES_ICON_SETTING_160X160_JPG            = { .s32Width = 160, .s32Height = 160};


/*! @brief Total size: 43818, total (including trailing 0s): 43830 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_CLOCK_64X64_PNG] = { "clock_64x64.png", 2436, EF_EFLAGS_None, &EF_g_u8Blobs[0], &EF_g_sIMAGES_CLOCK_64X64_PNG },/* size: 2436 */
   [EF_EFILE_FAN_512X512_PNG] = { "fan_512x512.png", 7814, EF_EFLAGS_None, &EF_g_u8Blobs[2437], &EF_g_sIMAGES_FAN_512X512_PNG },/* size: 7814 */
   [EF_EFILE_FAN_72X72_PNG] = { "fan_72x72.png", 2338, EF_EFLAGS_None, &EF_g_u8Blobs[10252], &EF_g_sIMAGES_FAN_72X72_PNG },/* size: 2338 */
   [EF_EFILE_FLAME_56X72_PNG] = { "flame_56x72.png", 1711, EF_EFLAGS_None, &EF_g_u8Blobs[12591], &EF_g_sIMAGES_FLAME_56X72_PNG },/* size: 1711 */
   [EF_EFILE_HOME_72X72_PNG] = { "home_72x72.png", 1896, EF_EFLAGS_None, &EF_g_u8Blobs[14303], &EF_g_sIMAGES_HOME_72X72_PNG },/* size: 1896 */
   [EF_EFILE_ICON_ARROW_BACK_EN_160X160_JPG] = { "icon-arrow-back_en_160x160.jpg", 6015, EF_EFLAGS_None, &EF_g_u8Blobs[16200], &EF_g_sIMAGES_ICON_ARROW_BACK_EN_160X160_JPG },/* size: 6015 */
   [EF_EFILE_ICON_ARROW_DOWN_DISA_120X60_JPG] = { "icon-arrow-down_disa_120x60.jpg", 893, EF_EFLAGS_None, &EF_g_u8Blobs[22216], &EF_g_sIMAGES_ICON_ARROW_DOWN_DISA_120X60_JPG },/* size: 893 */
   [EF_EFILE_ICON_ARROW_DOWN_EN_120X60_JPG] = { "icon-arrow-down_en_120x60.jpg", 2442, EF_EFLAGS_None, &EF_g_u8Blobs[23110], &EF_g_sIMAGES_ICON_ARROW_DOWN_EN_120X60_JPG },/* size: 2442 */
   [EF_EFILE_ICON_ARROW_UP_DISA_120X60_JPG] = { "icon-arrow-up_disa_120x60.jpg", 893, EF_EFLAGS_None, &EF_g_u8Blobs[25553], &EF_g_sIMAGES_ICON_ARROW_UP_DISA_120X60_JPG },/* size: 893 */
   [EF_EFILE_ICON_ARROW_UP_EN_120X60_JPG] = { "icon-arrow-up_en_120x60.jpg", 2347, EF_EFLAGS_None, &EF_g_u8Blobs[26447], &EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG },/* size: 2347 */
   [EF_EFILE_ICON_SBI_LOGO_152X112_JPG] = { "icon-sbi-logo_152x112.jpg", 6923, EF_EFLAGS_None, &EF_g_u8Blobs[28795], &EF_g_sIMAGES_ICON_SBI_LOGO_152X112_JPG },/* size: 6923 */
   [EF_EFILE_ICON_SETTING_160X160_JPG] = { "icon-setting_160x160.jpg", 8110, EF_EFLAGS_None, &EF_g_u8Blobs[35719], &EF_g_sIMAGES_ICON_SETTING_160X160_JPG }/* size: 8110 */
};
