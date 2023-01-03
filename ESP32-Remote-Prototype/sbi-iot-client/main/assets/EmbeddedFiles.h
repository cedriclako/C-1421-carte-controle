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
  EF_EFILE_CLOCK_64X64_PNG = 0,    /*!< @brief File: clock_64x64.png */
  EF_EFILE_FAN_512X512_PNG = 1,    /*!< @brief File: fan_512x512.png */
  EF_EFILE_FAN_72X72_PNG = 2,    /*!< @brief File: fan_72x72.png */
  EF_EFILE_FLAME_56X72_PNG = 3,    /*!< @brief File: flame_56x72.png */
  EF_EFILE_HOME_72X72_PNG = 4,    /*!< @brief File: home_72x72.png */
  EF_EFILE_ICON_ARROW_DOWN_120X60_JPG = 5,    /*!< @brief File: icon-arrow-down_120x60.jpg */
  EF_EFILE_ICON_ARROW_UP_120X60_JPG = 6,    /*!< @brief File: icon-arrow-up_120x60.jpg */
  EF_EFILE_ICON_SBI_LOGO_152X112_JPG = 7,    /*!< @brief File: icon-sbi-logo_152x112.jpg */
  EF_EFILE_ICON_SETTING_160X160_JPG = 8,    /*!< @brief File: icon-setting_160x160.jpg */
  EF_EFILE_COUNT = 9
} EF_EFILE;

/*! @brief Check if compressed flag is active */
#define EF_ISFILECOMPRESSED(x) ((x & EF_EFLAGS_GZip) == EF_EFLAGS_GZip)

extern const EF_SFile EF_g_sFiles[EF_EFILE_COUNT];
extern const uint8_t EF_g_u8Blobs[];

#endif
