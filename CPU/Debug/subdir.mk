################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../ansisop.c \
../cpu.c \
../libreriaCPU.c \
../pcb.c 

OBJS += \
./ansisop.o \
./cpu.o \
./libreriaCPU.o \
./pcb.o 

C_DEPS += \
./ansisop.d \
./cpu.d \
./libreriaCPU.d \
./pcb.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/home/utnso/workspace/tp-2017-1c-Los-rezagados/Herramientas" -O0 -g3 -Wall -c -fmessage-length=0 -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


