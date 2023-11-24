/*
 * BinaryMarker.c
 *
 *  Created on: Nov 20, 2023
 *      Author: mcarrier
 */
#include "BinaryMarker.h"
#include "GitCommit.h"

// Trailer, used to identify the app-bin size at runtime.
const uint8_t BM_g_u8Trailers[16] __attribute__((__section__(".TrailerMarker"))) = { 'S', 'B', 'I', 0xDF, 0x2F, 0x87, 0x68, 0x29, 0xe3, 0x43, 0x37, 'T', 'R', 'A', 'I', 'L' };

const BM_Marker BM_g_sMarker __attribute__((__section__(".BinaryMarker")))  =
{
    .u8Magics = { 'S', 'B', 'I', 'M', 'A', 'R', 'K', 0x01, 0x9c, 0xb3, 0x26, 0xae, 0x0b, 0x78, 0xd7, 0x12 },
    .u16FirmwareID = FWV_FIRMWAREID,
    .u8Versions = FWV_VERSION,
    #ifdef DEBUG
    .u8CompilationMode = 'D',
    #else
    .u8CompilationMode = 'R',
    #endif
    .u8GitIsDirty = GITCOMMIT_ISDIRTY,
    .u8GitCommitID = GITCOMMIT_COMMITID,
    .u8GitBranch = GITCOMMIT_BRANCH,
    .u8CompileDate = __DATE__,
    .u8CompileTime = __TIME__,
};
