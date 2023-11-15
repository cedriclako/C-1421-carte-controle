#ifndef _UFEC_STREAM_H_
#define _UFEC_STREAM_H_

#include <stdint.h>
#include <stdbool.h>
#include <string.h>

typedef struct
{
	uint8_t* pu8Buffer;
	uint32_t u32BufferLength;
	uint32_t u32Pointer;
} UFECSTREAM_SContext;

void UFECSTREAM_Init(UFECSTREAM_SContext *pContext, uint8_t u8Buffer[], uint32_t u32BufferLength);

void UFECSTREAM_Reset(UFECSTREAM_SContext *pContext);

uint32_t UFECSTREAM_Count(UFECSTREAM_SContext *pContext);

bool UFECSTREAM_WriteUInt32LE(UFECSTREAM_SContext *pContext, uint32_t value);
bool UFECSTREAM_ReadUInt32LE(UFECSTREAM_SContext *pContext, uint32_t* pOutValue);

bool UFECSTREAM_WriteUInt16LE(UFECSTREAM_SContext *pContext, uint16_t value);
bool UFECSTREAM_ReadUInt16LE(UFECSTREAM_SContext *pContext, uint16_t* p16OutValue);

bool UFECSTREAM_WriteUInt8LE(UFECSTREAM_SContext *pContext, uint8_t value);
bool UFECSTREAM_ReadUInt8LE(UFECSTREAM_SContext *pContext, uint8_t* pOutValue);

bool UFECSTREAM_WriteString(UFECSTREAM_SContext *pContext, const char* szText);
bool UFECSTREAM_ReadString(UFECSTREAM_SContext *pContext, char* szDest, uint32_t u32DestLen);

#endif
