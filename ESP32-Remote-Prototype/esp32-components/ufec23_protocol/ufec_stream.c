#include "ufec_stream.h"

void UFECSTREAM_Init(UFECSTREAM_SContext *pContext, uint8_t u8Buffer[], uint32_t u32BufferLength)
{
	pContext->pu8Buffer = u8Buffer;
	pContext->u32BufferLength = u32BufferLength;
	UFECSTREAM_Reset(pContext);
}

void UFECSTREAM_Reset(UFECSTREAM_SContext *pContext)
{
	pContext->u32Pointer = 0;
}

uint32_t UFECSTREAM_Count(UFECSTREAM_SContext *pContext)
{
	return pContext->u32Pointer;
}

bool UFECSTREAM_EncUInt32LE(UFECSTREAM_SContext *pContext, uint32_t value)
{
	if (pContext->u32Pointer + sizeof(uint32_t) > pContext->u32BufferLength)
		return false;
    memcpy( (uint8_t*)(pContext->pu8Buffer+pContext->u32Pointer), &value, sizeof(uint32_t));
    return true;
}

bool UFECSTREAM_EncUInt16LE(UFECSTREAM_SContext *pContext, uint16_t value)
{
	if (pContext->u32Pointer + sizeof(uint16_t) > pContext->u32BufferLength)
		return false;
    memcpy( (uint8_t*)(pContext->pu8Buffer+pContext->u32Pointer), &value, sizeof(uint16_t));
    return true;
}
