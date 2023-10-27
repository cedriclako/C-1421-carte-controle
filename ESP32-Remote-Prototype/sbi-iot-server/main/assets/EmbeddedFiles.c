#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG            = { .s32Width = 350, .s32Height = 256};


/*! @brief Total size: 484420, total (including trailing 0s): 484434 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_MNT_INDEX_HTML] = { "mnt-index.html", 5185, EF_EFLAGS_None, &EF_g_u8Blobs[0], NULL },/* size: 5185 */
   [EF_EFILE_MNT_OTA_HTML] = { "mnt-ota.html", 781, EF_EFLAGS_None, &EF_g_u8Blobs[5186], NULL },/* size: 781 */
   [EF_EFILE_USER_INDEX_HTML] = { "user-index.html", 2913, EF_EFLAGS_None, &EF_g_u8Blobs[5968], NULL },/* size: 2913 */
   [EF_EFILE_CSS_MNT_CONTENT_CSS] = { "css/mnt-content.css", 1181, EF_EFLAGS_None, &EF_g_u8Blobs[8882], NULL },/* size: 1181 */
   [EF_EFILE_CSS_USER_CONTENT_CSS] = { "css/user-content.css", 1495, EF_EFLAGS_None, &EF_g_u8Blobs[10064], NULL },/* size: 1495 */
   [EF_EFILE_JS_API_DEF_JS] = { "js/api-def.js", 603, EF_EFLAGS_None, &EF_g_u8Blobs[11560], NULL },/* size: 603 */
   [EF_EFILE_JS_MNT_APP_JS] = { "js/mnt-app.js", 7017, EF_EFLAGS_None, &EF_g_u8Blobs[12164], NULL },/* size: 7017 */
   [EF_EFILE_JS_MNT_OTA_JS] = { "js/mnt-ota.js", 1032, EF_EFLAGS_None, &EF_g_u8Blobs[19182], NULL },/* size: 1032 */
   [EF_EFILE_JS_USER_APP_JS] = { "js/user-app.js", 5543, EF_EFLAGS_None, &EF_g_u8Blobs[20215], NULL },/* size: 5543 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107165, EF_EFLAGS_None, &EF_g_u8Blobs[25759], NULL },/* size: 107165 */
   [EF_EFILE_FAVICON_ICO] = { "favicon.ico", 1673, EF_EFLAGS_None, &EF_g_u8Blobs[132925], NULL },/* size: 1673 */
   [EF_EFILE_IMG_LOGO_SBI_350X256_PNG] = { "img/logo-sbi-350x256.png", 14236, EF_EFLAGS_None, &EF_g_u8Blobs[134599], &EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG },/* size: 14236 */
   [EF_EFILE_FONT_ROBOTO_BOLD_TTF] = { "font/Roboto-Bold.ttf", 167336, EF_EFLAGS_None, &EF_g_u8Blobs[148836], NULL },/* size: 167336 */
   [EF_EFILE_FONT_ROBOTO_REGULAR_TTF] = { "font/Roboto-Regular.ttf", 168260, EF_EFLAGS_None, &EF_g_u8Blobs[316173], NULL }/* size: 168260 */
};
