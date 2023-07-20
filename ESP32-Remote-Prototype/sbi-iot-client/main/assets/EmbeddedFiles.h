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
  EF_EFILE_PNG_CLOCK_64X64_PNG = 0,                             /*!< @brief file: png/clock_64x64.png, size: 2436 */
  EF_EFILE_PNG_FAN_512X512_PNG = 1,                             /*!< @brief file: png/fan_512x512.png, size: 7814 */
  EF_EFILE_PNG_FAN_72X72_PNG = 2,                               /*!< @brief file: png/fan_72x72.png, size: 2338 */
  EF_EFILE_PNG_HOME_72X72_PNG = 3,                              /*!< @brief file: png/home_72x72.png, size: 1896 */
  EF_EFILE_ICON_3BARS_80X80_JPG = 4,                            /*!< @brief file: icon-3bars_80x80.jpg, size: 1498 */
  EF_EFILE_ICON_ARROW_BACK_EN_160X160_JPG = 5,                  /*!< @brief file: icon-arrow-back_en_160x160.jpg, size: 6015 */
  EF_EFILE_ICON_ARROW_DOWN_DISA_120X60_JPG = 6,                 /*!< @brief file: icon-arrow-down_disa_120x60.jpg, size: 893 */
  EF_EFILE_ICON_ARROW_DOWN_EN_120X60_JPG = 7,                   /*!< @brief file: icon-arrow-down_en_120x60.jpg, size: 2442 */
  EF_EFILE_ICON_ARROW_UP_DISA_120X60_JPG = 8,                   /*!< @brief file: icon-arrow-up_disa_120x60.jpg, size: 893 */
  EF_EFILE_ICON_ARROW_UP_EN_120X60_JPG = 9,                     /*!< @brief file: icon-arrow-up_en_120x60.jpg, size: 2347 */
  EF_EFILE_ICON_DISTRIBUTION_72X72_JPG = 10,                    /*!< @brief file: icon-distribution_72x72.jpg, size: 2313 */
  EF_EFILE_ICON_FAN_72X72_JPG = 11,                             /*!< @brief file: icon-fan_72x72.jpg, size: 1905 */
  EF_EFILE_ICON_FIREBOOST_72X72_JPG = 12,                       /*!< @brief file: icon-fireboost_72x72.jpg, size: 2036 */
  EF_EFILE_ICON_FLAME_224X288_JPG = 13,                         /*!< @brief file: icon-flame_224x288.jpg, size: 4751 */
  EF_EFILE_ICON_FLAME_56X72_JPG = 14,                           /*!< @brief file: icon-flame_56x72.jpg, size: 2295 */
  EF_EFILE_ICON_FLAME_56X72_PNG = 15,                           /*!< @brief file: icon-flame_56x72.png, size: 1711 */
  EF_EFILE_ICON_FLAME_80X80_JPG = 16,                           /*!< @brief file: icon-flame_80x80.jpg, size: 1606 */
  EF_EFILE_ICON_SBI_LOGO_152X112_JPG = 17,                      /*!< @brief file: icon-sbi-logo_152x112.jpg, size: 6923 */
  EF_EFILE_ICON_SETTING_160X160_JPG = 18,                       /*!< @brief file: icon-setting_160x160.jpg, size: 8110 */
  EF_EFILE_COUNT = 19
} EF_EFILE;

/*! @brief Check if compressed flag is active */
#define EF_ISFILECOMPRESSED(x) ((x & EF_EFLAGS_GZip) == EF_EFLAGS_GZip)


extern const EF_SFile EF_g_sFiles[EF_EFILE_COUNT];
extern const uint8_t EF_g_u8Blobs[];

extern const EF_SImage EF_g_sIMAGES_PNG_CLOCK_64X64_PNG;
extern const EF_SImage EF_g_sIMAGES_PNG_FAN_512X512_PNG;
extern const EF_SImage EF_g_sIMAGES_PNG_FAN_72X72_PNG;
extern const EF_SImage EF_g_sIMAGES_PNG_HOME_72X72_PNG;
extern const EF_SImage EF_g_sIMAGES_ICON_3BARS_80X80_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_ARROW_BACK_EN_160X160_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_ARROW_DOWN_DISA_120X60_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_ARROW_DOWN_EN_120X60_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_ARROW_UP_DISA_120X60_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_ARROW_UP_EN_120X60_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_DISTRIBUTION_72X72_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_FAN_72X72_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_FIREBOOST_72X72_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_FLAME_224X288_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_FLAME_56X72_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_FLAME_56X72_PNG;
extern const EF_SImage EF_g_sIMAGES_ICON_FLAME_80X80_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_SBI_LOGO_152X112_JPG;
extern const EF_SImage EF_g_sIMAGES_ICON_SETTING_160X160_JPG;

#ifdef __cplusplus
}
#endif

#endif
