#include "EmbeddedFiles.h"


/*! @brief Total size: 130388, total (including trailing 0s): 130395 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_INDEX_HTML] = { "index.html", 2453, EF_EFLAGS_None, &EF_g_u8Blobs[0] },/* size: 2453 */
   [EF_EFILE_OTA_HTML] = { "ota.html", 769, EF_EFLAGS_None, &EF_g_u8Blobs[2454] },/* size: 769 */
   [EF_EFILE_CSS_CONTENT_CSS] = { "css/content.css", 0, EF_EFLAGS_None, &EF_g_u8Blobs[3224] },/* size: 0 */
   [EF_EFILE_JS_APP_JS] = { "js/app.js", 4739, EF_EFLAGS_None, &EF_g_u8Blobs[3225] },/* size: 4739 */
   [EF_EFILE_JS_OTA_JS] = { "js/ota.js", 1026, EF_EFLAGS_None, &EF_g_u8Blobs[7965] },/* size: 1026 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107165, EF_EFLAGS_None, &EF_g_u8Blobs[8992] },/* size: 107165 */
   [EF_EFILE_IMG_LOGO_SBI_350X256_PNG] = { "img/logo-sbi-350x256.png", 14236, EF_EFLAGS_None, &EF_g_u8Blobs[116158] }/* size: 14236 */
};
