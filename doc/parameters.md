# Parameters Tables

The parameter table allows to change the system parameters live and transfer them to the ESP32.
All parameters except those with 'volatile' option active are saved into the internal flash memory.

Refer to: [Memory layout](./mem-layout.md) for more details.

## Parameter item

Every single parameter items need to be configured.

| Variable | Description |
|---|---|
| szKey         | Key, a name for the variable |
| szDesc        | Description |
| eType         | Type of data (int32, float) |
| vdVar         | Pointer to the int32 variable |
| uType         | Min/Max/Default |
| s32Default    | Default value |
| s32Min        | Minimum value |
| s32Max        | Maximum value |
| eOpt          | Options (volatile = don't save into flash) |

## Load parameter items from flash

Every value are saved twice, the second save applies a magic binary mask, so if flash memory parameters moves it will be detected and resetted to default value.

The magic mask is just a checksum of fixed all fixed parameter item. (Keys/Min/Max/Default).

If there is an error during the parameter item loading phase, the value get reset to the default value.

## Drawbacks

- Configurations are sequentially saved into flash memory. If a new parameter item is added in the middle of the parameter item table, it will force every other values beyond this one to be reset at default value.
- If new values are added at the end of the parameter item tables it won't cause any problems.
