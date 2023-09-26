#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG            = { .s32Width = 350, .s32Height = 256};


/*! @brief Total size: 138787, total (including trailing 0s): 138795 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_INDEX_HTML] = { "index.html", 5226, EF_EFLAGS_None, &EF_g_u8Blobs[0], NULL },/* size: 5226 */
   [EF_EFILE_OTA_HTML] = { "ota.html", 769, EF_EFLAGS_None, &EF_g_u8Blobs[5227], NULL },/* size: 769 */
   [EF_EFILE_CSS_CONTENT_CSS] = { "css/content.css", 941, EF_EFLAGS_None, &EF_g_u8Blobs[5997], NULL },/* size: 941 */
   [EF_EFILE_JS_APP_JS] = { "js/app.js", 7745, EF_EFLAGS_None, &EF_g_u8Blobs[6939], NULL },/* size: 7745 */
   [EF_EFILE_JS_OTA_JS] = { "js/ota.js", 1032, EF_EFLAGS_None, &EF_g_u8Blobs[14685], NULL },/* size: 1032 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107165, EF_EFLAGS_None, &EF_g_u8Blobs[15718], NULL },/* size: 107165 */
   [EF_EFILE_FAVICON_ICO] = { "favicon.ico", 1673, EF_EFLAGS_None, &EF_g_u8Blobs[122884], NULL },/* size: 1673 */
   [EF_EFILE_IMG_LOGO_SBI_350X256_PNG] = { "img/logo-sbi-350x256.png", 14236, EF_EFLAGS_None, &EF_g_u8Blobs[124558], &EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG }/* size: 14236 */
};
