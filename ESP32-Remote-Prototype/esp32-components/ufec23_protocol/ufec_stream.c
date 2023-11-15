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

bool UFECSTREAM_WriteUInt32LE(UFECSTREAM_SContext *pContext, uint32_t value)
{
	if (pContext->u32Pointer + sizeof(uint32_t) > pContext->u32BufferLength)
		return false;
  memcpy( (uint8_t*)(pContext->pu8Buffer+pContext->u32Pointer), &value, sizeof(uint32_t));
  pContext->u32Pointer += sizeof(uint32_t);
  return true;
}

bool UFECSTREAM_ReadUInt32LE(UFECSTREAM_SContext *pContext, uint32_t* pOutValue)
{
  if (pContext->u32Pointer + sizeof(uint32_t) > pContext->u32BufferLength)
    return false;
  memcpy( pOutValue, (uint8_t*)(pContext->pu8Buffer+pContext->u32Pointer), sizeof(uint32_t));
  pContext->u32Pointer += sizeof(uint32_t);
  return true;
}

bool UFECSTREAM_WriteUInt8LE(UFECSTREAM_SContext *pContext, uint8_t value)
{
  if (pContext->u32Pointer + sizeof(uint8_t) > pContext->u32BufferLength)
    return false;
  *((uint8_t*)(pContext->pu8Buffer+pContext->u32Pointer)) = value;
  pContext->u32Pointer += sizeof(uint8_t);
  return true;
}

bool UFECSTREAM_ReadUInt8LE(UFECSTREAM_SContext *pContext, uint8_t* pOutValue)
{
  if (pContext->u32Pointer + sizeof(uint8_t) > pContext->u32BufferLength)
    return false;
  *pOutValue = *((uint8_t*)(pContext->pu8Buffer+pContext->u32Pointer));
  pContext->u32Pointer += sizeof(uint8_t);
  return true;
}

bool UFECSTREAM_WriteUInt16LE(UFECSTREAM_SContext *pContext, uint16_t value)
{
  const uint32_t u32NextPointer = pContext->u32Pointer + sizeof(uint16_t);
	if (u32NextPointer > pContext->u32BufferLength)
		return false;
  memcpy( (uint8_t*)(pContext->pu8Buffer+pContext->u32Pointer), &value, sizeof(uint16_t));
  pContext->u32Pointer = u32NextPointer;
  return true;
}

bool UFECSTREAM_ReadUInt16LE(UFECSTREAM_SContext *pContext, uint16_t* p16OutValue)
{
  const uint32_t u32NextPointer = pContext->u32Pointer + sizeof(uint16_t);
  if (u32NextPointer > pContext->u32BufferLength)
    return false;
  memcpy(p16OutValue, (uint8_t*)(pContext->pu8Buffer+pContext->u32Pointer), sizeof(uint16_t));
  pContext->u32Pointer = u32NextPointer;
  return true;
}

bool UFECSTREAM_WriteString(UFECSTREAM_SContext *pContext, const char* szText)
{
  // Len
  const uint32_t u32Len = strlen(szText);
  const int32_t s32NextPointer = pContext->u32Pointer + sizeof(uint16_t) + u32Len + sizeof(uint8_t);
  if (s32NextPointer > pContext->u32BufferLength)
    return false;

  const uint16_t u16Len = u32Len + 1; // Include the tailing 0.
  memcpy( (uint8_t*)(pContext->pu8Buffer+pContext->u32Pointer), &u16Len, sizeof(uint16_t));
  uint8_t* p = (uint8_t*)(pContext->pu8Buffer+pContext->u32Pointer + sizeof(uint16_t));
  memcpy(p, szText, u32Len);
  p[u32Len] = 0;

  pContext->u32Pointer = s32NextPointer;
  return true;
}

bool UFECSTREAM_ReadString(UFECSTREAM_SContext *pContext, char* szDest, uint32_t u32DestLen)
{
  if (pContext->u32Pointer + sizeof(uint16_t) > pContext->u32BufferLength)
    return false;
  // The length include the trailing 0 here.
  uint16_t u16Len = 0;
  memcpy(&u16Len, (uint8_t*)(pContext->pu8Buffer+pContext->u32Pointer), sizeof(uint16_t));

  if (u16Len > u32DestLen) // Need enough space to store it ...
    return false;

  const int32_t s32NextPointer = pContext->u32Pointer + sizeof(uint16_t) + u16Len;
  if (s32NextPointer > pContext->u32BufferLength)
    return false;

  const uint8_t* p = (uint8_t*)(pContext->pu8Buffer+pContext->u32Pointer + sizeof(uint16_t));
  memcpy(szDest, p, u16Len);

  pContext->u32Pointer = s32NextPointer;
  return true;
}
