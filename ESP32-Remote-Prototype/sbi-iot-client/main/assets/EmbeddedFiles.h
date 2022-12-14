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
  EF_EFILE_EXEMPLE_JPG = 0,    /*!< @brief File: Exemple.jpg */
  EF_EFILE_EXEMPLE_PNG = 1,    /*!< @brief File: Exemple.png */
  EF_EFILE_ICON_ARROW_DOWN_JPG = 2,    /*!< @brief File: icon-arrow-down.jpg */
  EF_EFILE_ICON_ARROW_UP_JPG = 3,    /*!< @brief File: icon-arrow-up.jpg */
  EF_EFILE_ICON_FAN_JPG = 4,    /*!< @brief File: icon-fan.jpg */
  EF_EFILE_ICON_HOME_JPG = 5,    /*!< @brief File: icon-home.jpg */
  EF_EFILE_ICON_SBI_LOGO_JPG = 6,    /*!< @brief File: icon-sbi-logo.jpg */
  EF_EFILE_ICON_SETTING_JPG = 7,    /*!< @brief File: icon-setting.jpg */
  EF_EFILE_COUNT = 8
} EF_EFILE;

/*! @brief Check if compressed flag is active */
#define EF_ISFILECOMPRESSED(x) ((x & EF_EFLAGS_GZip) == EF_EFLAGS_GZip)

extern const EF_SFile EF_g_sFiles[EF_EFILE_COUNT];
extern const uint8_t EF_g_u8Blobs[];

#endif
