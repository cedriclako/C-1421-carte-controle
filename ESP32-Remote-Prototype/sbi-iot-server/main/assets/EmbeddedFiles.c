#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG            = { .s32Width = 350, .s32Height = 256};


/*! @brief Total size: 494192, total (including trailing 0s): 494207 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_MNT_INDEX_HTML] = { "mnt-index.html", 5854, EF_EFLAGS_None, &EF_g_u8Blobs[0], NULL },/* size: 5854 */
   [EF_EFILE_MNT_OTA_HTML] = { "mnt-ota.html", 1215, EF_EFLAGS_None, &EF_g_u8Blobs[5855], NULL },/* size: 1215 */
   [EF_EFILE_USER_INDEX_HTML] = { "user-index.html", 6846, EF_EFLAGS_None, &EF_g_u8Blobs[7071], NULL },/* size: 6846 */
   [EF_EFILE_CSS_COMMON_CONTENT_CSS] = { "css/common-content.css", 1487, EF_EFLAGS_None, &EF_g_u8Blobs[13918], NULL },/* size: 1487 */
   [EF_EFILE_CSS_MNT_CONTENT_CSS] = { "css/mnt-content.css", 611, EF_EFLAGS_None, &EF_g_u8Blobs[15406], NULL },/* size: 611 */
   [EF_EFILE_CSS_USER_CONTENT_CSS] = { "css/user-content.css", 676, EF_EFLAGS_None, &EF_g_u8Blobs[16018], NULL },/* size: 676 */
   [EF_EFILE_JS_API_DEF_JS] = { "js/api-def.js", 851, EF_EFLAGS_None, &EF_g_u8Blobs[16695], NULL },/* size: 851 */
   [EF_EFILE_JS_MNT_APP_JS] = { "js/mnt-app.js", 7257, EF_EFLAGS_None, &EF_g_u8Blobs[17547], NULL },/* size: 7257 */
   [EF_EFILE_JS_MNT_OTA_JS] = { "js/mnt-ota.js", 2083, EF_EFLAGS_None, &EF_g_u8Blobs[24805], NULL },/* size: 2083 */
   [EF_EFILE_JS_USER_APP_JS] = { "js/user-app.js", 8652, EF_EFLAGS_None, &EF_g_u8Blobs[26889], NULL },/* size: 8652 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107155, EF_EFLAGS_None, &EF_g_u8Blobs[35542], NULL },/* size: 107155 */
   [EF_EFILE_FAVICON_ICO] = { "favicon.ico", 1673, EF_EFLAGS_None, &EF_g_u8Blobs[142698], NULL },/* size: 1673 */
   [EF_EFILE_IMG_LOGO_SBI_350X256_PNG] = { "img/logo-sbi-350x256.png", 14236, EF_EFLAGS_None, &EF_g_u8Blobs[144372], &EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG },/* size: 14236 */
   [EF_EFILE_FONT_ROBOTO_BOLD_TTF] = { "font/Roboto-Bold.ttf", 167336, EF_EFLAGS_None, &EF_g_u8Blobs[158609], NULL },/* size: 167336 */
   [EF_EFILE_FONT_ROBOTO_REGULAR_TTF] = { "font/Roboto-Regular.ttf", 168260, EF_EFLAGS_None, &EF_g_u8Blobs[325946], NULL }/* size: 168260 */
};

const uint32_t EF_g_u32BlobSize = 494207;
const uint32_t EF_g_u32BlobCRC32 = 4068982398;
