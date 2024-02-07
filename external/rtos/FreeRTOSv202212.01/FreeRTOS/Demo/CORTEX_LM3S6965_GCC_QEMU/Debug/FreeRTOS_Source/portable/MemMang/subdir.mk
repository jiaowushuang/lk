################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
/home/wwppll/Downloads/FreeRTOSv202212.01/FreeRTOS/Source/portable/MemMang/heap_4.c 

OBJS += \
./FreeRTOS_Source/portable/MemMang/heap_4.o 

C_DEPS += \
./FreeRTOS_Source/portable/MemMang/heap_4.d 


# Each subdirectory must supply rules for building sources it contributes
FreeRTOS_Source/portable/MemMang/heap_4.o: /home/wwppll/Downloads/FreeRTOSv202212.01/FreeRTOS/Source/portable/MemMang/heap_4.c FreeRTOS_Source/portable/MemMang/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GNU ARM Cross C Compiler'
	arm-none-eabi-gcc -mcpu=cortex-m3 -mthumb -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -Wall -Wextra -g3 -I"/home/wwppll/Downloads/FreeRTOSv202212.01/FreeRTOS/Demo/Common/include" -I"/home/wwppll/Downloads/FreeRTOSv202212.01/FreeRTOS-Plus/Source/FreeRTOS-Plus-Trace/Include" -I"/home/wwppll/Downloads/FreeRTOSv202212.01/FreeRTOS/Demo/CORTEX_LM3S6965_GCC_QEMU" -I"/home/wwppll/Downloads/FreeRTOSv202212.01/FreeRTOS/Demo/Common/drivers/LuminaryMicro" -I"/home/wwppll/Downloads/FreeRTOSv202212.01/FreeRTOS/Demo/CORTEX_LM3S6965_GCC_QEMU/LocalDemoFiles" -I"/home/wwppll/Downloads/FreeRTOSv202212.01/FreeRTOS/Source/include" -I"/home/wwppll/Downloads/FreeRTOSv202212.01/FreeRTOS/Source/portable/GCC/ARM_CM3" -std=gnu90 -specs=nano.specs -specs=nosys.specs -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


