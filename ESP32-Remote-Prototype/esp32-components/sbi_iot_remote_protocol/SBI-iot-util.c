#include "SBI.iot.pb.h"
#include "SBI-iot-util.h"

const char* SBIIOTUTIL_GetCmdPayloadPrettyString(pb_size_t which_payload)
{
    switch (which_payload)
    {
        case SBI_iot_Cmd_c2s_get_status_tag: 
            return "c2s_get_status";
        case SBI_iot_Cmd_s2c_get_status_resp_tag: 
            return "s2c_get_status";
        case SBI_iot_Cmd_c2s_change_settingsp_tag: 
            return "c2s_change_settingsp";
        case SBI_iot_Cmd_s2c_change_settingsp_resp_tag: 
            return "c2s_change_settingsp_resp";
    }
    return "";
}