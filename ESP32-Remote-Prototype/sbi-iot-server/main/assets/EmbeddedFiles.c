#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG            = { .s32Width = 350, .s32Height = 256};


/*! @brief Total size: 487288, total (including trailing 0s): 487302 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_MNT_INDEX_HTML] = { "mnt-index.html", 5892, EF_EFLAGS_None, &EF_g_u8Blobs[0], NULL },/* size: 5892 */
   [EF_EFILE_MNT_OTA_HTML] = { "mnt-ota.html", 1247, EF_EFLAGS_None, &EF_g_u8Blobs[5893], NULL },/* size: 1247 */
   [EF_EFILE_USER_INDEX_HTML] = { "user-index.html", 2913, EF_EFLAGS_None, &EF_g_u8Blobs[7141], NULL },/* size: 2913 */
   [EF_EFILE_CSS_MNT_CONTENT_CSS] = { "css/mnt-content.css", 1181, EF_EFLAGS_None, &EF_g_u8Blobs[10055], NULL },/* size: 1181 */
   [EF_EFILE_CSS_USER_CONTENT_CSS] = { "css/user-content.css", 1495, EF_EFLAGS_None, &EF_g_u8Blobs[11237], NULL },/* size: 1495 */
   [EF_EFILE_JS_API_DEF_JS] = { "js/api-def.js", 707, EF_EFLAGS_None, &EF_g_u8Blobs[12733], NULL },/* size: 707 */
   [EF_EFILE_JS_MNT_APP_JS] = { "js/mnt-app.js", 7482, EF_EFLAGS_None, &EF_g_u8Blobs[13441], NULL },/* size: 7482 */
   [EF_EFILE_JS_MNT_OTA_JS] = { "js/mnt-ota.js", 2158, EF_EFLAGS_None, &EF_g_u8Blobs[20924], NULL },/* size: 2158 */
   [EF_EFILE_JS_USER_APP_JS] = { "js/user-app.js", 5543, EF_EFLAGS_None, &EF_g_u8Blobs[23083], NULL },/* size: 5543 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107165, EF_EFLAGS_None, &EF_g_u8Blobs[28627], NULL },/* size: 107165 */
   [EF_EFILE_FAVICON_ICO] = { "favicon.ico", 1673, EF_EFLAGS_None, &EF_g_u8Blobs[135793], NULL },/* size: 1673 */
   [EF_EFILE_IMG_LOGO_SBI_350X256_PNG] = { "img/logo-sbi-350x256.png", 14236, EF_EFLAGS_None, &EF_g_u8Blobs[137467], &EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG },/* size: 14236 */
   [EF_EFILE_FONT_ROBOTO_BOLD_TTF] = { "font/Roboto-Bold.ttf", 167336, EF_EFLAGS_None, &EF_g_u8Blobs[151704], NULL },/* size: 167336 */
   [EF_EFILE_FONT_ROBOTO_REGULAR_TTF] = { "font/Roboto-Regular.ttf", 168260, EF_EFLAGS_None, &EF_g_u8Blobs[319041], NULL }/* size: 168260 */
};

const uint32_t EF_g_u32BlobSize = 487302;
const uint32_t EF_g_u32BlobCRC32 = 1192847235;
