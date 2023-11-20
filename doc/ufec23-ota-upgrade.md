# Process to update the UFEC23

## Embed the UFEC23 binary

The UFEC23 binary will be embedded inside the ESP32 OTA update binary.
The ESP32 will update itself before, then on the next reboot it will attempt to update the UFEC23.

## Update the UFEC23

The ESP32 shall use the UART to get informations about the UFEC23 before attempting to update.
What matter is the CRC32.

| CRC32 | Identical? | Operation |
|---|---|---|
| CRC32 | YES | Do nothing, consider the update is done |
| CRC32 | NO | Update the UFEC23 using the DFU bootloader |
| CRC32 | Unknown | In case you cannot get the CRC32, attempt to update. If it fail, consider the ESP32 update also failed and rollback |

### Reset the UFEC23

Before attempting to update, reset the UFEC23 by pulling down the UFEC23_RESET_PIN for half a second then release.

### Get the CRC32

To get the CRC32 you need to send a command through the UART port until you get a response. 
Allow a reasonable amount of time (like 30s) and do many attempts.

> Send UFEC23PROTOCOL_FRAMEID_ServerFirmwareInfo // then wait for response

This command will return the CRC32 for the currently running UFEC23.

| Field | Info |
|---|---|
| CRC32 | Running UFEC23 firmware |
| Size | Firmware size |
| Firmware ID | Firmware ID |
| Version.Major | Version (major) |
| Version.Minor | Version (minor) |
| Version.Revision | Version (revision) |

Ensure the firmware ID also match.

### DFU Update

Use the DFU update process to send the binary STM32 file by using the UART.

### Prevent rollback

The ESP32 needs to confirm the update has been done correctly.
Use these functions depending on the result:

| Function | Description |
|---|---|
| esp_ota_mark_app_valid_cancel_rollback | Confirm the update |
| esp_ota_mark_app_invalid_rollback_and_reboot | Cancel the update and restart to the previous version |