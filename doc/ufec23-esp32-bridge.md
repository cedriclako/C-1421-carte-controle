# Compile UFEC23

ESP32 bridge and UFEC23 share part of their code for UART communication.
To allow compilation we need to create a junction between esp32 components and UFEC.

| UFEC | Source |
|---|---|
| .\Core\esp32\ufec23_protocol | ../ESP32-Remote-Prototype/esp32-components/ufec23_protocol |
| .\Core\esp32\uart_protocol | ../ESP32-Remote-Prototype/esp32-components/uart_protocol |

Execute once:

> MakeJunction.bat

It will make '.\Core\esp32\' point to '../ESP32-Remote-Prototype/esp32-components/'

