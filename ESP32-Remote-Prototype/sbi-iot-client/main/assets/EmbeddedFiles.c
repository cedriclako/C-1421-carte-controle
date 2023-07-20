#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_PNG_CLOCK_64X64_PNG                 = { .s32Width = 64, .s32Height = 64};
const EF_SImage EF_g_sIMAGES_PNG_FAN_512X512_PNG                 = { .s32Width = 512, .s32Height = 512};
const EF_SImage EF_g_sIMAGES_PNG_FAN_72X72_PNG                   = { .s32Width = 72, .s32Height = 72};
const EF_SImage EF_g_sIMAGES_PNG_HOME_72X72_PNG                  = { .s32Width = 72, .s32Height = 72};
const EF_SImage EF_g_sIMAGES_ICON_3BARS_80X80_JPG                = { .s32Width = 80, .s32Height = 80};
const EF_SImage EF_g_sIMAGES_ICON_ARROW_BACK_EN_160X160_JPG      = { .s32Width = 160, .s32Height = 160};
const EF_SImage EF_g_sIMAGES_ICON_ARROW_DOWN_DISA_120X60_JPG     = { .s32Width = 120, .s32Height = 60};
const EF_SImage EF_g_sIMAGES_ICON_ARROW_DOWN_EN_120X60_JPG       = { .s32Width = 120, .s32Height = 60};
const EF_SImage EF_g_sIMAGES_ICON_ARROW_UP_DISA_120X60_JPG       = { .s32Width = 120, .s32Height = 60};
const EF_SImage EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG         = { .s32Width = 120, .s32Height = 60};
const EF_SImage EF_g_sIMAGES_ICON_DISTRIBUTION_72X72_JPG         = { .s32Width = 72, .s32Height = 72};
const EF_SImage EF_g_sIMAGES_ICON_FAN_72X72_JPG                  = { .s32Width = 72, .s32Height = 72};
const EF_SImage EF_g_sIMAGES_ICON_FIREBOOST_72X72_JPG            = { .s32Width = 72, .s32Height = 72};
const EF_SImage EF_g_sIMAGES_ICON_FLAME_224X288_JPG              = { .s32Width = 224, .s32Height = 288};
const EF_SImage EF_g_sIMAGES_ICON_FLAME_56X72_JPG                = { .s32Width = 56, .s32Height = 72};
const EF_SImage EF_g_sIMAGES_ICON_FLAME_56X72_PNG                = { .s32Width = 56, .s32Height = 72};
const EF_SImage EF_g_sIMAGES_ICON_FLAME_80X80_JPG                = { .s32Width = 80, .s32Height = 80};
const EF_SImage EF_g_sIMAGES_ICON_SBI_LOGO_152X112_JPG           = { .s32Width = 152, .s32Height = 112};
const EF_SImage EF_g_sIMAGES_ICON_SETTING_160X160_JPG            = { .s32Width = 160, .s32Height = 160};


/*! @brief Total size: 60222, total (including trailing 0s): 60241 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_PNG_CLOCK_64X64_PNG] = { "png/clock_64x64.png", 2436, EF_EFLAGS_None, &EF_g_u8Blobs[0], &EF_g_sIMAGES_PNG_CLOCK_64X64_PNG },/* size: 2436 */
   [EF_EFILE_PNG_FAN_512X512_PNG] = { "png/fan_512x512.png", 7814, EF_EFLAGS_None, &EF_g_u8Blobs[2437], &EF_g_sIMAGES_PNG_FAN_512X512_PNG },/* size: 7814 */
   [EF_EFILE_PNG_FAN_72X72_PNG] = { "png/fan_72x72.png", 2338, EF_EFLAGS_None, &EF_g_u8Blobs[10252], &EF_g_sIMAGES_PNG_FAN_72X72_PNG },/* size: 2338 */
   [EF_EFILE_PNG_HOME_72X72_PNG] = { "png/home_72x72.png", 1896, EF_EFLAGS_None, &EF_g_u8Blobs[12591], &EF_g_sIMAGES_PNG_HOME_72X72_PNG },/* size: 1896 */
   [EF_EFILE_ICON_3BARS_80X80_JPG] = { "icon-3bars_80x80.jpg", 1498, EF_EFLAGS_None, &EF_g_u8Blobs[14488], &EF_g_sIMAGES_ICON_3BARS_80X80_JPG },/* size: 1498 */
   [EF_EFILE_ICON_ARROW_BACK_EN_160X160_JPG] = { "icon-arrow-back_en_160x160.jpg", 6015, EF_EFLAGS_None, &EF_g_u8Blobs[15987], &EF_g_sIMAGES_ICON_ARROW_BACK_EN_160X160_JPG },/* size: 6015 */
   [EF_EFILE_ICON_ARROW_DOWN_DISA_120X60_JPG] = { "icon-arrow-down_disa_120x60.jpg", 893, EF_EFLAGS_None, &EF_g_u8Blobs[22003], &EF_g_sIMAGES_ICON_ARROW_DOWN_DISA_120X60_JPG },/* size: 893 */
   [EF_EFILE_ICON_ARROW_DOWN_EN_120X60_JPG] = { "icon-arrow-down_en_120x60.jpg", 2442, EF_EFLAGS_None, &EF_g_u8Blobs[22897], &EF_g_sIMAGES_ICON_ARROW_DOWN_EN_120X60_JPG },/* size: 2442 */
   [EF_EFILE_ICON_ARROW_UP_DISA_120X60_JPG] = { "icon-arrow-up_disa_120x60.jpg", 893, EF_EFLAGS_None, &EF_g_u8Blobs[25340], &EF_g_sIMAGES_ICON_ARROW_UP_DISA_120X60_JPG },/* size: 893 */
   [EF_EFILE_ICON_ARROW_UP_EN_120X60_JPG] = { "icon-arrow-up_en_120x60.jpg", 2347, EF_EFLAGS_None, &EF_g_u8Blobs[26234], &EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG },/* size: 2347 */
   [EF_EFILE_ICON_DISTRIBUTION_72X72_JPG] = { "icon-distribution_72x72.jpg", 2313, EF_EFLAGS_None, &EF_g_u8Blobs[28582], &EF_g_sIMAGES_ICON_DISTRIBUTION_72X72_JPG },/* size: 2313 */
   [EF_EFILE_ICON_FAN_72X72_JPG] = { "icon-fan_72x72.jpg", 1905, EF_EFLAGS_None, &EF_g_u8Blobs[30896], &EF_g_sIMAGES_ICON_FAN_72X72_JPG },/* size: 1905 */
   [EF_EFILE_ICON_FIREBOOST_72X72_JPG] = { "icon-fireboost_72x72.jpg", 2036, EF_EFLAGS_None, &EF_g_u8Blobs[32802], &EF_g_sIMAGES_ICON_FIREBOOST_72X72_JPG },/* size: 2036 */
   [EF_EFILE_ICON_FLAME_224X288_JPG] = { "icon-flame_224x288.jpg", 4751, EF_EFLAGS_None, &EF_g_u8Blobs[34839], &EF_g_sIMAGES_ICON_FLAME_224X288_JPG },/* size: 4751 */
   [EF_EFILE_ICON_FLAME_56X72_JPG] = { "icon-flame_56x72.jpg", 2295, EF_EFLAGS_None, &EF_g_u8Blobs[39591], &EF_g_sIMAGES_ICON_FLAME_56X72_JPG },/* size: 2295 */
   [EF_EFILE_ICON_FLAME_56X72_PNG] = { "icon-flame_56x72.png", 1711, EF_EFLAGS_None, &EF_g_u8Blobs[41887], &EF_g_sIMAGES_ICON_FLAME_56X72_PNG },/* size: 1711 */
   [EF_EFILE_ICON_FLAME_80X80_JPG] = { "icon-flame_80x80.jpg", 1606, EF_EFLAGS_None, &EF_g_u8Blobs[43599], &EF_g_sIMAGES_ICON_FLAME_80X80_JPG },/* size: 1606 */
   [EF_EFILE_ICON_SBI_LOGO_152X112_JPG] = { "icon-sbi-logo_152x112.jpg", 6923, EF_EFLAGS_None, &EF_g_u8Blobs[45206], &EF_g_sIMAGES_ICON_SBI_LOGO_152X112_JPG },/* size: 6923 */
   [EF_EFILE_ICON_SETTING_160X160_JPG] = { "icon-setting_160x160.jpg", 8110, EF_EFLAGS_None, &EF_g_u8Blobs[52130], &EF_g_sIMAGES_ICON_SETTING_160X160_JPG }/* size: 8110 */
};
