#include "EmbeddedFiles.h"


/*! @brief Total size: 114282, total (including trailing 0s): 114286 */
const EF_SFile EF_g_sFiles[EF_EFILE_COUNT] = 
{
   [EF_EFILE_INDEX_HTML] = { "index.html", 2378, EF_EFLAGS_None, &EF_g_u8Blobs[0] },/* size: 2378 */
   [EF_EFILE_CSS_CONTENT_CSS] = { "css/content.css", 0, EF_EFLAGS_None, &EF_g_u8Blobs[2379] },/* size: 0 */
   [EF_EFILE_JS_APP_JS] = { "js/app.js", 4739, EF_EFLAGS_None, &EF_g_u8Blobs[2380] },/* size: 4739 */
   [EF_EFILE_JS_VUE_MIN_JS] = { "js/vue.min.js", 107165, EF_EFLAGS_None, &EF_g_u8Blobs[7120] }/* size: 107165 */
};
