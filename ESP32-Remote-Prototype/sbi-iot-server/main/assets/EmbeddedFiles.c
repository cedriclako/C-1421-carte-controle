#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG            = { .s32Width = 350, .s32Height = 256};


/*! @brief Total size: 131675, total (including trailing 0s): 131682 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_INDEX_HTML] = { "index.html", 2566, EF_EFLAGS_None, &EF_g_u8Blobs[0], NULL },/* size: 2566 */
   [EF_EFILE_OTA_HTML] = { "ota.html", 769, EF_EFLAGS_None, &EF_g_u8Blobs[2567], NULL },/* size: 769 */
   [EF_EFILE_CSS_CONTENT_CSS] = { "css/content.css", 0, EF_EFLAGS_None, &EF_g_u8Blobs[3337], NULL },/* size: 0 */
   [EF_EFILE_JS_APP_JS] = { "js/app.js", 5913, EF_EFLAGS_None, &EF_g_u8Blobs[3338], NULL },/* size: 5913 */
   [EF_EFILE_JS_OTA_JS] = { "js/ota.js", 1026, EF_EFLAGS_None, &EF_g_u8Blobs[9252], NULL },/* size: 1026 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107165, EF_EFLAGS_None, &EF_g_u8Blobs[10279], NULL },/* size: 107165 */
   [EF_EFILE_IMG_LOGO_SBI_350X256_PNG] = { "img/logo-sbi-350x256.png", 14236, EF_EFLAGS_None, &EF_g_u8Blobs[117445], &EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG }/* size: 14236 */
};
