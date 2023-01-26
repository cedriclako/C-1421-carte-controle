#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG            = { .s32Width = 350, .s32Height = 256};


/*! @brief Total size: 135306, total (including trailing 0s): 135314 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_INDEX_HTML] = { "index.html", 3094, EF_EFLAGS_None, &EF_g_u8Blobs[0], NULL },/* size: 3094 */
   [EF_EFILE_OTA_HTML] = { "ota.html", 769, EF_EFLAGS_None, &EF_g_u8Blobs[3095], NULL },/* size: 769 */
   [EF_EFILE_CSS_CONTENT_CSS] = { "css/content.css", 772, EF_EFLAGS_None, &EF_g_u8Blobs[3865], NULL },/* size: 772 */
   [EF_EFILE_JS_APP_JS] = { "js/app.js", 6571, EF_EFLAGS_None, &EF_g_u8Blobs[4638], NULL },/* size: 6571 */
   [EF_EFILE_JS_OTA_JS] = { "js/ota.js", 1026, EF_EFLAGS_None, &EF_g_u8Blobs[11210], NULL },/* size: 1026 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107165, EF_EFLAGS_None, &EF_g_u8Blobs[12237], NULL },/* size: 107165 */
   [EF_EFILE_FAVICON_ICO] = { "favicon.ico", 1673, EF_EFLAGS_None, &EF_g_u8Blobs[119403], NULL },/* size: 1673 */
   [EF_EFILE_IMG_LOGO_SBI_350X256_PNG] = { "img/logo-sbi-350x256.png", 14236, EF_EFLAGS_None, &EF_g_u8Blobs[121077], &EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG }/* size: 14236 */
};
