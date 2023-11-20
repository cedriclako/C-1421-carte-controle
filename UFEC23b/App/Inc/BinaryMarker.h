/*
 * BinaryMarker.h
 *
 *  Created on: Nov 20, 2023
 *      Author: mcarrier
 */

#ifndef INC_BINARYMARKER_H_
#define INC_BINARYMARKER_H_

#include <stdint.h>
#include "FirmwareVersion.h"

typedef struct
{
  uint8_t u8Magics[16];
  uint16_t u16FirmwareID;
  uint8_t u8Versions[3];
  uint8_t u8IsDebug;
  uint8_t u8GitIsDirty;
  uint8_t u8GitCommitID[96];
  uint8_t u8GitBranch[96];
  uint8_t u8CompileDate[32];
  uint8_t u8CompileTime[32];
} BM_Marker;

extern const uint8_t BM_g_u8Trailers[16];
extern const BM_Marker BM_g_sMarker;

#endif /* INC_BINARYMARKER_H_ */
