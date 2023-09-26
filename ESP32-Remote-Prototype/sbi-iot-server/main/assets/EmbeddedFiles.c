#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG            = { .s32Width = 350, .s32Height = 256};


/*! @brief Total size: 138828, total (including trailing 0s): 138836 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_INDEX_HTML] = { "index.html", 5159, EF_EFLAGS_None, &EF_g_u8Blobs[0], NULL },/* size: 5159 */
   [EF_EFILE_OTA_HTML] = { "ota.html", 769, EF_EFLAGS_None, &EF_g_u8Blobs[5160], NULL },/* size: 769 */
   [EF_EFILE_CSS_CONTENT_CSS] = { "css/content.css", 941, EF_EFLAGS_None, &EF_g_u8Blobs[5930], NULL },/* size: 941 */
   [EF_EFILE_JS_APP_JS] = { "js/app.js", 7853, EF_EFLAGS_None, &EF_g_u8Blobs[6872], NULL },/* size: 7853 */
   [EF_EFILE_JS_OTA_JS] = { "js/ota.js", 1032, EF_EFLAGS_None, &EF_g_u8Blobs[14726], NULL },/* size: 1032 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107165, EF_EFLAGS_None, &EF_g_u8Blobs[15759], NULL },/* size: 107165 */
   [EF_EFILE_FAVICON_ICO] = { "favicon.ico", 1673, EF_EFLAGS_None, &EF_g_u8Blobs[122925], NULL },/* size: 1673 */
   [EF_EFILE_IMG_LOGO_SBI_350X256_PNG] = { "img/logo-sbi-350x256.png", 14236, EF_EFLAGS_None, &EF_g_u8Blobs[124599], &EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG }/* size: 14236 */
};
