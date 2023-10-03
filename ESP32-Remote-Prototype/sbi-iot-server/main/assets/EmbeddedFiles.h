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
  EF_EFILE_MNT_INDEX_HTML = 0,                                  /*!< @brief file: mnt-index.html, size: 5167 */
  EF_EFILE_MNT_OTA_HTML = 1,                                    /*!< @brief file: mnt-ota.html, size: 781 */
  EF_EFILE_USER_INDEX_HTML = 2,                                 /*!< @brief file: user-index.html, size: 991 */
  EF_EFILE_CSS_MNT_CONTENT_CSS = 3,                             /*!< @brief file: css/mnt-content.css, size: 941 */
  EF_EFILE_CSS_USER_CONTENT_CSS = 4,                            /*!< @brief file: css/user-content.css, size: 780 */
  EF_EFILE_JS_MNT_APP_JS = 5,                                   /*!< @brief file: js/mnt-app.js, size: 7813 */
  EF_EFILE_JS_MNT_OTA_JS = 6,                                   /*!< @brief file: js/mnt-ota.js, size: 1032 */
  EF_EFILE_JS_USER_APP_JS = 7,                                  /*!< @brief file: js/user-app.js, size: 141 */
  EF_EFILE_JS_VUE_MIN_JS = 8,                                   /*!< @brief file: js/vue.min.js, size: 107165 */
  EF_EFILE_FAVICON_ICO = 9,                                     /*!< @brief file: favicon.ico, size: 1673 */
  EF_EFILE_IMG_LOGO_SBI_350X256_PNG = 10,                       /*!< @brief file: img/logo-sbi-350x256.png, size: 14236 */
  EF_EFILE_FONT_ROBOTO_BOLD_TTF = 11,                           /*!< @brief file: font/Roboto-Bold.ttf, size: 167336 */
  EF_EFILE_FONT_ROBOTO_REGULAR_TTF = 12,                        /*!< @brief file: font/Roboto-Regular.ttf, size: 168260 */
  EF_EFILE_COUNT = 13
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
