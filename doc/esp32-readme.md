# Burn eFuse

To resolve a GPIO conflict where GPIO 12 is used to select the LDO voltage, we use an E-Fuse option to force it at 3.3v and free the GPIO pin.

```
components/esptool_py/esptool/espefuse.py set_flash_voltage 3.3V 
```
