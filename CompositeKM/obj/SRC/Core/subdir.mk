################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../SRC/Core/core_riscv.c 

C_DEPS += \
./SRC/Core/core_riscv.d 

OBJS += \
./SRC/Core/core_riscv.o 

DIR_OBJS += \
./SRC/Core/*.o \

DIR_DEPS += \
./SRC/Core/*.d \

DIR_EXPANDS += \
./SRC/Core/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
SRC/Core/%.o: ../SRC/Core/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/CompositeKM/User" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

