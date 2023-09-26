mkdir .\Core\esp32

rmdir .\Core\esp32\ufec23_protocol
rmdir .\Core\esp32\uart_protocol

mklink /J .\Core\esp32\ufec23_protocol "../ESP32-Remote-Prototype/esp32-components/ufec23_protocol"
mklink /J .\Core\esp32\uart_protocol "../ESP32-Remote-Prototype/esp32-components/uart_protocol"