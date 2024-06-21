UFEC23 : autoregulated stove



ESP32 (sbi-iot-server)
Web server, IO extender and comm with remote


Using :
ESP-IDF 5.1 in VS Code
Python3
cp2102n usb to uart controller driver
don't forget to regen assets before building


main file : main.c


-----------------------------------------------------------------


STM32 (UFEC23b)
Low level, main algo, control of motors and temperature measurements


stm32cubeide version 1.14
main file : main.c and algo.c (mostly algo.c)
don't forget to makejunction.bat


-------------------------------------------------------------------

M5EDP on esp32
Remote control


Using platformio in vs code
main file : main.cpp 
comm : espnowcomm.cpp




--------------------------------------------------------------------


Particles board

C-1333_-_Mesures_emissions.X

MPLAB X IDE 5.5

main file : main.c