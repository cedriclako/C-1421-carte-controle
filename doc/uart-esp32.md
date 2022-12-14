# ESP32 exchange protocol

To exchange data between the ESP32 and the microcontroller we use a simple framing protocol.

## Protocol details

| Byte | Value | Description |
|---|---|---|
| START_BYTE | 0xCC | Start of Frame
| STOP_BYTE | 0x99 | End of Frame


Basically: [START BYTE] [ID] [DATA LENGTH] [DATAS ...] [CHECKSUM] [STOP BYTE] 

We start reading on 0xCC until we get 0x99
Then we calculate the checksum, if it pass the frame is accepted.

The protocol support up to 255 bytes frame


## Calculate the checksum

The check sum is composed of every bytes between start byte and checksum. 
Followed by a bitwise operation.

Example:

Frame: [0xCC 01 04 BA DC OF FE 52 0x99]

01 + 04 + BA + DC + OF + FE = 0x02AD

We keep the last 8 bits so:
0xAD, then we do bitwise on it: 0x52
