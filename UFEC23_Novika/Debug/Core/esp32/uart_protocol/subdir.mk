################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Core/esp32/uart_protocol/uart_protocol_dec.c \
../Core/esp32/uart_protocol/uart_protocol_enc.c 

OBJS += \
./Core/esp32/uart_protocol/uart_protocol_dec.o \
./Core/esp32/uart_protocol/uart_protocol_enc.o 

C_DEPS += \
./Core/esp32/uart_protocol/uart_protocol_dec.d \
./Core/esp32/uart_protocol/uart_protocol_enc.d 


# Each subdirectory must supply rules for building sources it contributes
Core/esp32/uart_protocol/uart_protocol_dec.o: ../Core/esp32/uart_protocol/uart_protocol_dec.c Core/esp32/uart_protocol/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F105xC -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/esp32/uart_protocol/uart_protocol_dec.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"
Core/esp32/uart_protocol/uart_protocol_enc.o: ../Core/esp32/uart_protocol/uart_protocol_enc.c Core/esp32/uart_protocol/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F105xC -c -I../Core/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"Core/esp32/uart_protocol/uart_protocol_enc.d" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

