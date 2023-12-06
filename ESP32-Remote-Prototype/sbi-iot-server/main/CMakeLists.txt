idf_component_register(SRCS "OTACheck.c" "log.c" "main.c" "spiff.c" "settings.c" "espnowprocess.c" 
    "webserver/webserver.c" 
    "webserver/StaticFileServe.c" "webserver/OTAUploadSTM32.c" "webserver/OTAUploadESP32.c" 
    "webserver/APIGet.c" "webserver/APIPost.c"
    "iot/IoTBridge.c"
    "OTACheck.c"
    "hardwaregpio.c" "uartbridge/uartbridge.c" "uartbridge/stovemb.c" "event.c"
    "assets/EmbeddedFilesBin.c" "assets/EmbeddedFiles.c"
    INCLUDE_DIRS "."
    REQUIRES bootloader_support bt lwip log json esp_rom esp_common esp_app_format driver esp_wifi esp_timer esp_hw_support nvs_flash
        app_update esp_http_server esp_http_client fatfs vfs spiffs esp-tls mbedtls
        misc_utils nvsjson sbi_iot_remote_protocol stm32-flasher uart_protocol ufec23_protocol)
