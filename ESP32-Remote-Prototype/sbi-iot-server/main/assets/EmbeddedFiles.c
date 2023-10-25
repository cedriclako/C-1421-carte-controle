#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG            = { .s32Width = 350, .s32Height = 256};


/*! @brief Total size: 496085, total (including trailing 0s): 496104 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_INDEX_HTML] = { "index.html", 5159, EF_EFLAGS_None, &EF_g_u8Blobs[0], NULL },/* size: 5159 */
   [EF_EFILE_MNT_INDEX_HTML] = { "mnt-index.html", 5350, EF_EFLAGS_None, &EF_g_u8Blobs[5160], NULL },/* size: 5350 */
   [EF_EFILE_MNT_OTA_HTML] = { "mnt-ota.html", 781, EF_EFLAGS_None, &EF_g_u8Blobs[10511], NULL },/* size: 781 */
   [EF_EFILE_OTA_HTML] = { "ota.html", 769, EF_EFLAGS_None, &EF_g_u8Blobs[11293], NULL },/* size: 769 */
   [EF_EFILE_USER_INDEX_HTML] = { "user-index.html", 1339, EF_EFLAGS_None, &EF_g_u8Blobs[12063], NULL },/* size: 1339 */
   [EF_EFILE_CSS_CONTENT_CSS] = { "css/content.css", 941, EF_EFLAGS_None, &EF_g_u8Blobs[13403], NULL },/* size: 941 */
   [EF_EFILE_CSS_MNT_CONTENT_CSS] = { "css/mnt-content.css", 1181, EF_EFLAGS_None, &EF_g_u8Blobs[14345], NULL },/* size: 1181 */
   [EF_EFILE_CSS_USER_CONTENT_CSS] = { "css/user-content.css", 778, EF_EFLAGS_None, &EF_g_u8Blobs[15527], NULL },/* size: 778 */
   [EF_EFILE_JS_API_DEF_JS] = { "js/api-def.js", 542, EF_EFLAGS_None, &EF_g_u8Blobs[16306], NULL },/* size: 542 */
   [EF_EFILE_JS_APP_JS] = { "js/app.js", 7813, EF_EFLAGS_None, &EF_g_u8Blobs[16849], NULL },/* size: 7813 */
   [EF_EFILE_JS_MNT_APP_JS] = { "js/mnt-app.js", 7386, EF_EFLAGS_None, &EF_g_u8Blobs[24663], NULL },/* size: 7386 */
   [EF_EFILE_JS_MNT_OTA_JS] = { "js/mnt-ota.js", 1032, EF_EFLAGS_None, &EF_g_u8Blobs[32050], NULL },/* size: 1032 */
   [EF_EFILE_JS_OTA_JS] = { "js/ota.js", 1032, EF_EFLAGS_None, &EF_g_u8Blobs[33083], NULL },/* size: 1032 */
   [EF_EFILE_JS_USER_APP_JS] = { "js/user-app.js", 3312, EF_EFLAGS_None, &EF_g_u8Blobs[34116], NULL },/* size: 3312 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107165, EF_EFLAGS_None, &EF_g_u8Blobs[37429], NULL },/* size: 107165 */
   [EF_EFILE_FAVICON_ICO] = { "favicon.ico", 1673, EF_EFLAGS_None, &EF_g_u8Blobs[144595], NULL },/* size: 1673 */
   [EF_EFILE_IMG_LOGO_SBI_350X256_PNG] = { "img/logo-sbi-350x256.png", 14236, EF_EFLAGS_None, &EF_g_u8Blobs[146269], &EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG },/* size: 14236 */
   [EF_EFILE_FONT_ROBOTO_BOLD_TTF] = { "font/Roboto-Bold.ttf", 167336, EF_EFLAGS_None, &EF_g_u8Blobs[160506], NULL },/* size: 167336 */
   [EF_EFILE_FONT_ROBOTO_REGULAR_TTF] = { "font/Roboto-Regular.ttf", 168260, EF_EFLAGS_None, &EF_g_u8Blobs[327843], NULL }/* size: 168260 */
};
