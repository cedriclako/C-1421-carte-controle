#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG            = { .s32Width = 350, .s32Height = 256};


/*! @brief Total size: 486183, total (including trailing 0s): 486197 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_MNT_INDEX_HTML] = { "mnt-index.html", 5252, EF_EFLAGS_None, &EF_g_u8Blobs[0], NULL },/* size: 5252 */
   [EF_EFILE_MNT_OTA_HTML] = { "mnt-ota.html", 1247, EF_EFLAGS_None, &EF_g_u8Blobs[5253], NULL },/* size: 1247 */
   [EF_EFILE_USER_INDEX_HTML] = { "user-index.html", 2913, EF_EFLAGS_None, &EF_g_u8Blobs[6501], NULL },/* size: 2913 */
   [EF_EFILE_CSS_MNT_CONTENT_CSS] = { "css/mnt-content.css", 1181, EF_EFLAGS_None, &EF_g_u8Blobs[9415], NULL },/* size: 1181 */
   [EF_EFILE_CSS_USER_CONTENT_CSS] = { "css/user-content.css", 1495, EF_EFLAGS_None, &EF_g_u8Blobs[10597], NULL },/* size: 1495 */
   [EF_EFILE_JS_API_DEF_JS] = { "js/api-def.js", 707, EF_EFLAGS_None, &EF_g_u8Blobs[12093], NULL },/* size: 707 */
   [EF_EFILE_JS_MNT_APP_JS] = { "js/mnt-app.js", 7017, EF_EFLAGS_None, &EF_g_u8Blobs[12801], NULL },/* size: 7017 */
   [EF_EFILE_JS_MNT_OTA_JS] = { "js/mnt-ota.js", 2158, EF_EFLAGS_None, &EF_g_u8Blobs[19819], NULL },/* size: 2158 */
   [EF_EFILE_JS_USER_APP_JS] = { "js/user-app.js", 5543, EF_EFLAGS_None, &EF_g_u8Blobs[21978], NULL },/* size: 5543 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107165, EF_EFLAGS_None, &EF_g_u8Blobs[27522], NULL },/* size: 107165 */
   [EF_EFILE_FAVICON_ICO] = { "favicon.ico", 1673, EF_EFLAGS_None, &EF_g_u8Blobs[134688], NULL },/* size: 1673 */
   [EF_EFILE_IMG_LOGO_SBI_350X256_PNG] = { "img/logo-sbi-350x256.png", 14236, EF_EFLAGS_None, &EF_g_u8Blobs[136362], &EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG },/* size: 14236 */
   [EF_EFILE_FONT_ROBOTO_BOLD_TTF] = { "font/Roboto-Bold.ttf", 167336, EF_EFLAGS_None, &EF_g_u8Blobs[150599], NULL },/* size: 167336 */
   [EF_EFILE_FONT_ROBOTO_REGULAR_TTF] = { "font/Roboto-Regular.ttf", 168260, EF_EFLAGS_None, &EF_g_u8Blobs[317936], NULL }/* size: 168260 */
};

const uint32_t EF_g_u32BlobSize = 486197;
const uint32_t EF_g_u32BlobCRC32 = 1751083882;
