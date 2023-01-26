#ifndef _SBI_IOT_UTIL_H_
#define _SBI_IOT_UTIL_H_

#include <pb.h>

#ifdef __cplusplus
extern "C" {
#endif

const char* SBIIOTUTIL_GetCmdPayloadPrettyString(pb_size_t which_payload);

#ifdef __cplusplus
}
#endif

#endif