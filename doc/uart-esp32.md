# ESP32 exchange protocol

To exchange data between the ESP32 and the microcontroller we use a simple framing protocol.

Everything is defined as network byte order (BIG ENDIAN).

## Protocol details

| Byte | Value | Description |
|---|---|---|
| START_BYTE | 0xCC | Start of Frame
| ID | [0x00-0xFF] | Frame identifier
| Payload length | [0x0000-0xFFFF] | Payload length (16 bits)
| Payload | ... | Between 0 and 65536 bytes maximum
| STOP_BYTE | 0x99 | End of Frame

Basically: [START BYTE] [ID] [PAYLOAD LENGTH (2 bytes)] [PAYLOAD  ...] [CHECKSUM] [STOP BYTE] 

We start reading on 0xCC until we get 0x99
Then we calculate the checksum, if it pass the frame is accepted.

The protocol support up to 255 bytes as payload per frame.

## Calculate the checksum

The check sum is composed of every bytes between start byte and checksum. 
Followed by a bitwise operation.

Example:

Frame: [CC 01 00 04 BA DC 0F FE 57 99]

01 + 00 + 04 + BA + DC + 0F + FE = 0x02A8

We keep the last 8 bits so:
0xA8, then we do bitwise on it: 0x57
