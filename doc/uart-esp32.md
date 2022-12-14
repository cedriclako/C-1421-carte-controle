# ESP32 exchange protocol

To efficiently exchange data between the stove board and the ESP32 we need a UART framing protocol.

## Suggested protocol:

One frame can contains one or more data.

SOF [Frame Payload] [CRC32 16 bits] EOF

Frame payload needs to be escaped using the escape character.

| Name | Flag |
|---|---|
| Start of Frame flag | 0x12
| End of Frame flag | 0x13
| Escape (DLE) | 0x7D

Endianness? (To be defined)

## Frame payload

Frame payload is composed of one or more variable identified by an ID:

| Name | Type | Description |
|---|---|---|
| Variable ID | 16 bits | 1 array (1 bit) + Variable type (3 bits) + ID (12 bits)
| Variable Data | Variable data depend on variable type

### Variable Type

| Type ID | Name | Length
|---|---|---|
| 0x80 | Flag indicate if it's an array | The next character represent data length (Up to 255).
| 1 | byte | 1
| 2 | int16 | 2
| 3 | int32 | 4
| 4 | float | 4
| 5 | double | 8