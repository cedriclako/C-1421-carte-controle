# Parameters

## Goal

We want to save parameters into memory. We don't have an hardware EEPROM so we will use the internal flash.

### Caveat

- Shouln't write too often or it will burn the flash page (10 000 write cycle before burning it out.)
- Slow to write
- Needs to ensure software update won't overrite it.
- 2 KB sector minimum size as minimum erasure

The product may needs to be updated into the future, a bootloader will be required.
It may even need dual boot to be fail-safe but it's to be determined.

# MCU capability

The MCU allows to have multiple programs in parallel into the same flash memory. The only obstacle is having enough memory for every one of them.

## Memory layout

According to the documentation, the STM32F105RCTX have access to 256KB flash, divided into 128x 2KiB page.
Thise is an except from the manufacturer documentation.

![](./assets/memorylayout.png)

# Suggested solution

Partition the flash memory with some future proofing.
Partition layout could looks like this:

NOTE: Address doesn't start at 0 (flash memory is mapped at 0x0800_0000), refer to the datasheet for more details.
Nothing here is final, it's still suggested.

| Partition name | Start page | Page count | Description
|---|---|---|---|
| APP | 0 | 124 (124 KiB) | Main application, location 0 |
| PARAMS | 124 | 2 (4 KiB) | User parameters or calibration |
| EXTRAS | 126 | 2 (4 KiB) | In case we need to store extra informations not related to the parameters |

