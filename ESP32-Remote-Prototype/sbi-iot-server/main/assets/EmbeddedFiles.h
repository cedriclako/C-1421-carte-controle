#ifndef _EMBEDDEDFILES_
#define _EMBEDDEDFILES_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
   EF_EFLAGS_None = 0,
   EF_EFLAGS_GZip = 1,
} EF_EFLAGS;

typedef struct
{
   int32_t s32Width;
   int32_t s32Height;
} EF_SImage;

typedef struct
{
   const char* strFilename;
   uint32_t u32Length;
   EF_EFLAGS eFlags;
   const uint8_t* pu8StartAddr;
   const void* pMetaData;
} EF_SFile;

typedef enum
{
  EF_EFILE_INDEX_HTML = 0,                                      /*!< @brief file: index.html, size: 4528 */
  EF_EFILE_OTA_HTML = 1,                                        /*!< @brief file: ota.html, size: 769 */
  EF_EFILE_CSS_CONTENT_CSS = 2,                                 /*!< @brief file: css/content.css, size: 941 */
  EF_EFILE_JS_APP_JS = 3,                                       /*!< @brief file: js/app.js, size: 7163 */
  EF_EFILE_JS_OTA_JS = 4,                                       /*!< @brief file: js/ota.js, size: 1026 */
  EF_EFILE_JS_VUE_MIN_JS = 5,                                   /*!< @brief file: js/vue.min.js, size: 107165 */
  EF_EFILE_FAVICON_ICO = 6,                                     /*!< @brief file: favicon.ico, size: 1673 */
  EF_EFILE_IMG_LOGO_SBI_350X256_PNG = 7,                        /*!< @brief file: img/logo-sbi-350x256.png, size: 14236 */
  EF_EFILE_COUNT = 8
} EF_EFILE;

/*! @brief Check if compressed flag is active */
#define EF_ISFILECOMPRESSED(x) ((x & EF_EFLAGS_GZip) == EF_EFLAGS_GZip)


extern const EF_SFile EF_g_sFiles[EF_EFILE_COUNT];
extern const uint8_t EF_g_u8Blobs[];

extern const EF_SImage EF_g_sIMAGES_IMG_LOGO_SBI_350X256_PNG;

#ifdef __cplusplus
}
#endif

#endif
