################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (14.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/Custom_Drivers/Src/drv2605l.c 

OBJS += \
./Drivers/Custom_Drivers/Src/drv2605l.o 

C_DEPS += \
./Drivers/Custom_Drivers/Src/drv2605l.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/Custom_Drivers/Src/%.o Drivers/Custom_Drivers/Src/%.su Drivers/Custom_Drivers/Src/%.cyclo: ../Drivers/Custom_Drivers/Src/%.c Drivers/Custom_Drivers/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m4 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F411xE -c -I../../Core/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc -I../../Drivers/STM32F4xx_HAL_Driver/Inc/Legacy -I../../Drivers/CMSIS/Device/ST/STM32F4xx/Include -I../../Drivers/CMSIS/Include -I"D:/MyDocuments/stm32projects/Klaus/STM32CubeIDE/Drivers/Custom_Drivers/Inc" -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Drivers-2f-Custom_Drivers-2f-Src

clean-Drivers-2f-Custom_Drivers-2f-Src:
	-$(RM) ./Drivers/Custom_Drivers/Src/drv2605l.cyclo ./Drivers/Custom_Drivers/Src/drv2605l.d ./Drivers/Custom_Drivers/Src/drv2605l.o ./Drivers/Custom_Drivers/Src/drv2605l.su

.PHONY: clean-Drivers-2f-Custom_Drivers-2f-Src

