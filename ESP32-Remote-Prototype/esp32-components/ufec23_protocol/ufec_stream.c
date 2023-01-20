#include "ufec_stream.h"

uint8_t* UFECSTREAM_EncUint32LE(uint8_t* u8Datas, uint32_t value)
{
    memcpy(u8Datas, &value, sizeof(uint32_t));
    return (uint8_t*)(u8Datas + sizeof(uint32_t));
}
