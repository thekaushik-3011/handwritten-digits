################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../BSP/Src/ili9341.c \
../BSP/Src/stm32f429i_discovery.c \
../BSP/Src/stm32f429i_discovery_io.c \
../BSP/Src/stm32f429i_discovery_lcd.c \
../BSP/Src/stm32f429i_discovery_sdram.c \
../BSP/Src/stm32f429i_discovery_ts.c \
../BSP/Src/stmpe811.c 

OBJS += \
./BSP/Src/ili9341.o \
./BSP/Src/stm32f429i_discovery.o \
./BSP/Src/stm32f429i_discovery_io.o \
./BSP/Src/stm32f429i_discovery_lcd.o \
./BSP/Src/stm32f429i_discovery_sdram.o \
./BSP/Src/stm32f429i_discovery_ts.o \
./BSP/Src/stmpe811.o 

C_DEPS += \
./BSP/Src/ili9341.d \
./BSP/Src/stm32f429i_discovery.d \
./BSP/Src/stm32f429i_discovery_io.d \
./BSP/Src/stm32f429i_discovery_lcd.d \
./BSP/Src/stm32f429i_discovery_sdram.d \
./BSP/Src/stm32f429i_discovery_ts.d \
./BSP/Src/stmpe811.d 


# Each subdirectory must supply rules for building sources it contributes
BSP/Src/%.o BSP/Src/%.su BSP/Src/%.cyclo: ../BSP/Src/%.c BSP/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F429xx -c -I../BSP/Inc -I../Components/Src -I../Components/Inc -I../Core/Inc -I../Drivers/CMSIS/Core/Include -I../Middlewares/ST/AI/Inc -I../X-CUBE-AI/App -I../Drivers/STM32F4xx_HAL_Driver/Inc -I../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../Drivers/CMSIS/Include -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-BSP-2f-Src

clean-BSP-2f-Src:
	-$(RM) ./BSP/Src/ili9341.cyclo ./BSP/Src/ili9341.d ./BSP/Src/ili9341.o ./BSP/Src/ili9341.su ./BSP/Src/stm32f429i_discovery.cyclo ./BSP/Src/stm32f429i_discovery.d ./BSP/Src/stm32f429i_discovery.o ./BSP/Src/stm32f429i_discovery.su ./BSP/Src/stm32f429i_discovery_io.cyclo ./BSP/Src/stm32f429i_discovery_io.d ./BSP/Src/stm32f429i_discovery_io.o ./BSP/Src/stm32f429i_discovery_io.su ./BSP/Src/stm32f429i_discovery_lcd.cyclo ./BSP/Src/stm32f429i_discovery_lcd.d ./BSP/Src/stm32f429i_discovery_lcd.o ./BSP/Src/stm32f429i_discovery_lcd.su ./BSP/Src/stm32f429i_discovery_sdram.cyclo ./BSP/Src/stm32f429i_discovery_sdram.d ./BSP/Src/stm32f429i_discovery_sdram.o ./BSP/Src/stm32f429i_discovery_sdram.su ./BSP/Src/stm32f429i_discovery_ts.cyclo ./BSP/Src/stm32f429i_discovery_ts.d ./BSP/Src/stm32f429i_discovery_ts.o ./BSP/Src/stm32f429i_discovery_ts.su ./BSP/Src/stmpe811.cyclo ./BSP/Src/stmpe811.d ./BSP/Src/stmpe811.o ./BSP/Src/stmpe811.su

.PHONY: clean-BSP-2f-Src

