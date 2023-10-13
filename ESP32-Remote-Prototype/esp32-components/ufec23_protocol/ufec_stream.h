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

bool UFECSTREAM_EncUInt32LE(UFECSTREAM_SContext *pContext, uint32_t value);

bool UFECSTREAM_EncUInt16LE(UFECSTREAM_SContext *pContext, uint16_t value);

#endif
