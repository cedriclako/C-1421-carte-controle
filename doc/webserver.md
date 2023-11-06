# Webpage

All assets related to the webpage are bundled using an utility.
It is important to ensure the bundled resources are up to date.
Up regenerate the asset packages, run this command:

> .\embeddedgen.bat

All webpage made for casual user a prefixed with 'user-', those made for advanced user are prefixed 'mnt-'.

| Index | File Name | Size |
|-------|-----------|------|
| EF_EFILE_MNT_INDEX_HTML | mnt-index.html | 5185 |
| EF_EFILE_MNT_OTA_HTML | mnt-ota.html | 781 |
| EF_EFILE_USER_INDEX_HTML | user-index.html | 2913 |
| EF_EFILE_CSS_MNT_CONTENT_CSS | css/mnt-content.css | 1181 |
| EF_EFILE_CSS_USER_CONTENT_CSS | css/user-content.css | 1495 |
| EF_EFILE_JS_API_DEF_JS | js/api-def.js | 603 |
| EF_EFILE_JS_MNT_APP_JS | js/mnt-app.js | 7017 |
| EF_EFILE_JS_MNT_OTA_JS | js/mnt-ota.js | 1032 |
| EF_EFILE_JS_USER_APP_JS | js/user-app.js | 5543 |
| EF_EFILE_JS_VUE_MIN_JS | js/vue.min.js | 107165 |
| EF_EFILE_FAVICON_ICO | favicon.ico | 1673 |
| EF_EFILE_IMG_LOGO_SBI_350X256_PNG | img/logo-sbi-350x256.png | 14236 |
| EF_EFILE_FONT_ROBOTO_BOLD_TTF | font/Roboto-Bold.ttf | 167336 |
| EF_EFILE_FONT_ROBOTO_REGULAR_TTF | font/Roboto-Regular.ttf | 168260 | 

# End Points

'/' is automatically redirected to 'index.html' by default.

| File Name | Description | User | Maintenance |
|-------|------|------|------|
| /index.html | Main page, where a casual user will land | YES | YES |
| /api/access-maintenance-redirect | API to enter the password to access the maintenance mode. | YES | YES |
| /api/pairingsettings | Get/Set pairing settings | YES | YES |
| /api/getsettingsjson | Get all ESP32 settings | NO | YES |
| /api/setsettingsjson | Set all ESP32 settings | NO | YES |
| /api/wifisettings | Get/Set Wifi-Settings (* Cannot get the password) | YES | YES |
| /action/reboot | Reboot the ESP32 | YES | YES |
| /action/downloadconfig | Force to redownload the STM32 parameter file |  NO | YES |
| /api/getsysinfo | Get system informations | NO | YES |
| /api/getlivedata | Get live datas | NO | YES |
| /api/serverparameterfile | Get/Set the STM32 parameter file | NO | YES |
| /ota/upload_stm32 | Upload a new firmware into the STM32 using the UART | NO | YES |
| /ota/upload_esp32 | Upload a new firmware into the ESP32 | NO | YES |

## Upload new FW to STM32

You can test the upload to STM32 using 'Insomnia' or 'Postman'.
No webpage page has been created yet.

NOTE: The access is restricted, you may need to enter the 'Troubleshoot' password unless it is deactivated

Refer to: [Whitebox](./Whitebox.md) for more details.

http://172.16.40.231/ota/upload_stm32

![](./assets/post-upload-stm32.png)
