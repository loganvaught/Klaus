################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
D:/MyDocuments/stm32projects/Klaus/Drivers/Custom/Src/drv2605l.c 

OBJS += \
./Drivers/Custom/Src/drv2605l.o 

C_DEPS += \
./Drivers/Custom/Src/drv2605l.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Custom/Src/drv2605l.o: D:/MyDocuments/stm32projects/Klaus/Drivers/Custom/Src/drv2605l.c Drivers/Custom/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"D:/MyDocuments/stm32projects/Klaus/Drivers/Custom/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-Custom-2f-Src

clean-Drivers-2f-Custom-2f-Src:
	-$(RM) ./Drivers/Custom/Src/drv2605l.cyclo ./Drivers/Custom/Src/drv2605l.d ./Drivers/Custom/Src/drv2605l.o ./Drivers/Custom/Src/drv2605l.su

.PHONY: clean-Drivers-2f-Custom-2f-Src

