#include <stdio.h>
#include "misc_utils.h"

int32_t MISCUTIL_PrettyHexaString(char* dst, uint32_t u32DstLen, const uint8_t u8Inputs[], uint32_t u32InputLen)
{
    const char hex[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 'A', 'B', 'C', 'D', 'E', 'F' };
    const uint32_t requiredDstLen = u32InputLen * 3; // Include trailing 0

    if (u32InputLen == 0 || u32DstLen < requiredDstLen)
        return 0;
    
    // Iterate over each byte in the array
    int32_t s32Ix = 0;
    for (int32_t i = 0; i < u32InputLen; i++)
    {
        const uint8_t u8 = u8Inputs[i];
        dst[s32Ix++] = hex[((u8 >> 4) & 0x0F)];
        dst[s32Ix++] = hex[(u8 & 0x0F)];
        dst[s32Ix++] = ' ';
    } 
    // Replace the last trailing space by 0.
    dst[s32Ix-1] = 0x00;
    return s32Ix;
}
