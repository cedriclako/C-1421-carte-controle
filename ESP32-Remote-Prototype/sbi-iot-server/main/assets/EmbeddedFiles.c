#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG            = { .s32Width = 350, .s32Height = 256};


/*! @brief Total size: 495073, total (including trailing 0s): 495088 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_MNT_INDEX_HTML] = { "mnt-index.html", 5976, EF_EFLAGS_None, &EF_g_u8Blobs[0], NULL },/* size: 5976 */
   [EF_EFILE_MNT_OTA_HTML] = { "mnt-ota.html", 1247, EF_EFLAGS_None, &EF_g_u8Blobs[5977], NULL },/* size: 1247 */
   [EF_EFILE_USER_INDEX_HTML] = { "user-index.html", 6834, EF_EFLAGS_None, &EF_g_u8Blobs[7225], NULL },/* size: 6834 */
   [EF_EFILE_CSS_COMMON_CONTENT_CSS] = { "css/common-content.css", 1572, EF_EFLAGS_None, &EF_g_u8Blobs[14060], NULL },/* size: 1572 */
   [EF_EFILE_CSS_MNT_CONTENT_CSS] = { "css/mnt-content.css", 644, EF_EFLAGS_None, &EF_g_u8Blobs[15633], NULL },/* size: 644 */
   [EF_EFILE_CSS_USER_CONTENT_CSS] = { "css/user-content.css", 715, EF_EFLAGS_None, &EF_g_u8Blobs[16278], NULL },/* size: 715 */
   [EF_EFILE_JS_API_DEF_JS] = { "js/api-def.js", 877, EF_EFLAGS_None, &EF_g_u8Blobs[16994], NULL },/* size: 877 */
   [EF_EFILE_JS_MNT_APP_JS] = { "js/mnt-app.js", 7482, EF_EFLAGS_None, &EF_g_u8Blobs[17872], NULL },/* size: 7482 */
   [EF_EFILE_JS_MNT_OTA_JS] = { "js/mnt-ota.js", 2158, EF_EFLAGS_None, &EF_g_u8Blobs[25355], NULL },/* size: 2158 */
   [EF_EFILE_JS_USER_APP_JS] = { "js/user-app.js", 8898, EF_EFLAGS_None, &EF_g_u8Blobs[27514], NULL },/* size: 8898 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107165, EF_EFLAGS_None, &EF_g_u8Blobs[36413], NULL },/* size: 107165 */
   [EF_EFILE_FAVICON_ICO] = { "favicon.ico", 1673, EF_EFLAGS_None, &EF_g_u8Blobs[143579], NULL },/* size: 1673 */
   [EF_EFILE_IMG_LOGO_SBI_350X256_PNG] = { "img/logo-sbi-350x256.png", 14236, EF_EFLAGS_None, &EF_g_u8Blobs[145253], &EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG },/* size: 14236 */
   [EF_EFILE_FONT_ROBOTO_BOLD_TTF] = { "font/Roboto-Bold.ttf", 167336, EF_EFLAGS_None, &EF_g_u8Blobs[159490], NULL },/* size: 167336 */
   [EF_EFILE_FONT_ROBOTO_REGULAR_TTF] = { "font/Roboto-Regular.ttf", 168260, EF_EFLAGS_None, &EF_g_u8Blobs[326827], NULL }/* size: 168260 */
};

const uint32_t EF_g_u32BlobSize = 495088;
const uint32_t EF_g_u32BlobCRC32 = 392989670;
