################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (9-2020-q2-update)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Src/DebugManager.c \
../Src/DebugPort.c \
../Src/Hmi.c \
../Src/MotorManager.c \
../Src/Pid.c \
../Src/ProdTest.c \
../Src/TemperatureManager.c \
../Src/air_input.c \
../Src/algo.c \
../Src/freertos.c \
../Src/main.c \
../Src/slope.c \
../Src/stm32f1xx_hal_msp.c \
../Src/stm32f1xx_hal_timebase_tim.c \
../Src/stm32f1xx_it.c \
../Src/syscalls.c \
../Src/system_stm32f1xx.c 

OBJS += \
./Src/DebugManager.o \
./Src/DebugPort.o \
./Src/Hmi.o \
./Src/MotorManager.o \
./Src/Pid.o \
./Src/ProdTest.o \
./Src/TemperatureManager.o \
./Src/air_input.o \
./Src/algo.o \
./Src/freertos.o \
./Src/main.o \
./Src/slope.o \
./Src/stm32f1xx_hal_msp.o \
./Src/stm32f1xx_hal_timebase_tim.o \
./Src/stm32f1xx_it.o \
./Src/syscalls.o \
./Src/system_stm32f1xx.o 

C_DEPS += \
./Src/DebugManager.d \
./Src/DebugPort.d \
./Src/Hmi.d \
./Src/MotorManager.d \
./Src/Pid.d \
./Src/ProdTest.d \
./Src/TemperatureManager.d \
./Src/air_input.d \
./Src/algo.d \
./Src/freertos.d \
./Src/main.d \
./Src/slope.d \
./Src/stm32f1xx_hal_msp.d \
./Src/stm32f1xx_hal_timebase_tim.d \
./Src/stm32f1xx_it.d \
./Src/syscalls.d \
./Src/system_stm32f1xx.d 


# Each subdirectory must supply rules for building sources it contributes
Src/%.o: ../Src/%.c Src/subdir.mk
	arm-none-eabi-gcc -c "$<" -mcpu=cortex-m3 -std=gnu11 '-D__weak=__attribute__((weak))' '-D__packed=__attribute__((__packed__))' -DUSE_HAL_DRIVER -DSTM32F103xE -DREVC -DGRILL_CIRC -DRELEASE -c -I../Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

