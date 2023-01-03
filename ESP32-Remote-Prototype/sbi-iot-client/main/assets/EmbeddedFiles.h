#ifndef _EMBEDDEDFILES_
#define _EMBEDDEDFILES_

#include <stdint.h>

typedef enum
{
   EF_EFLAGS_None = 0,
   EF_EFLAGS_GZip = 1,
} EF_EFLAGS;

typedef struct
{
   const char* strFilename;
   uint32_t u32Length;
   EF_EFLAGS eFlags;
   const uint8_t* pu8StartAddr;
} EF_SFile;

typedef enum
{
  EF_EFILE_CLOCK_64X64_PNG = 0,                                 /*!< @brief file: clock_64x64.png, size: 2436 */
  EF_EFILE_FAN_512X512_PNG = 1,                                 /*!< @brief file: fan_512x512.png, size: 7814 */
  EF_EFILE_FAN_72X72_PNG = 2,                                   /*!< @brief file: fan_72x72.png, size: 2338 */
  EF_EFILE_FLAME_56X72_PNG = 3,                                 /*!< @brief file: flame_56x72.png, size: 1711 */
  EF_EFILE_HOME_72X72_PNG = 4,                                  /*!< @brief file: home_72x72.png, size: 1896 */
  EF_EFILE_ICON_ARROW_DOWN_120X60_JPG = 5,                      /*!< @brief file: icon-arrow-down_120x60.jpg, size: 2442 */
  EF_EFILE_ICON_ARROW_UP_120X60_JPG = 6,                        /*!< @brief file: icon-arrow-up_120x60.jpg, size: 2347 */
  EF_EFILE_ICON_SBI_LOGO_152X112_JPG = 7,                       /*!< @brief file: icon-sbi-logo_152x112.jpg, size: 6923 */
  EF_EFILE_ICON_SETTING_160X160_JPG = 8,                        /*!< @brief file: icon-setting_160x160.jpg, size: 8110 */
  EF_EFILE_COUNT = 9
} EF_EFILE;

/*! @brief Check if compressed flag is active */
#define EF_ISFILECOMPRESSED(x) ((x & EF_EFLAGS_GZip) == EF_EFLAGS_GZip)

extern const EF_SFile EF_g_sFiles[EF_EFILE_COUNT];
extern const uint8_t EF_g_u8Blobs[];

#endif
