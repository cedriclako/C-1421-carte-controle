#include "EmbeddedFiles.h"
#include <stddef.h>

const EF_SImage EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG            = { .s32Width = 350, .s32Height = 256};


/*! @brief Total size: 476046, total (including trailing 0s): 476058 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_MNT_INDEX_HTML] = { "mnt-index.html", 5167, EF_EFLAGS_None, &EF_g_u8Blobs[0], NULL },/* size: 5167 */
   [EF_EFILE_MNT_OTA_HTML] = { "mnt-ota.html", 781, EF_EFLAGS_None, &EF_g_u8Blobs[5168], NULL },/* size: 781 */
   [EF_EFILE_USER_INDEX_HTML] = { "user-index.html", 864, EF_EFLAGS_None, &EF_g_u8Blobs[5950], NULL },/* size: 864 */
   [EF_EFILE_CSS_MNT_CONTENT_CSS] = { "css/mnt-content.css", 941, EF_EFLAGS_None, &EF_g_u8Blobs[6815], NULL },/* size: 941 */
   [EF_EFILE_CSS_USER_CONTENT_CSS] = { "css/user-content.css", 778, EF_EFLAGS_None, &EF_g_u8Blobs[7757], NULL },/* size: 778 */
   [EF_EFILE_JS_MNT_APP_JS] = { "js/mnt-app.js", 7813, EF_EFLAGS_None, &EF_g_u8Blobs[8536], NULL },/* size: 7813 */
   [EF_EFILE_JS_MNT_OTA_JS] = { "js/mnt-ota.js", 1032, EF_EFLAGS_None, &EF_g_u8Blobs[16350], NULL },/* size: 1032 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107165, EF_EFLAGS_None, &EF_g_u8Blobs[17383], NULL },/* size: 107165 */
   [EF_EFILE_FAVICON_ICO] = { "favicon.ico", 1673, EF_EFLAGS_None, &EF_g_u8Blobs[124549], NULL },/* size: 1673 */
   [EF_EFILE_IMG_LOGO_SBI_350X256_PNG] = { "img/logo-sbi-350x256.png", 14236, EF_EFLAGS_None, &EF_g_u8Blobs[126223], &EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG },/* size: 14236 */
   [EF_EFILE_FONT_ROBOTO_BOLD_TTF] = { "font/Roboto-Bold.ttf", 167336, EF_EFLAGS_None, &EF_g_u8Blobs[140460], NULL },/* size: 167336 */
   [EF_EFILE_FONT_ROBOTO_REGULAR_TTF] = { "font/Roboto-Regular.ttf", 168260, EF_EFLAGS_None, &EF_g_u8Blobs[307797], NULL }/* size: 168260 */
};
