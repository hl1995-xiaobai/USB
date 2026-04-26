################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
S_UPPER_SRCS += \
../SRC/Startup/startup_ch32v30x_D8.S \
../SRC/Startup/startup_ch32v30x_D8C.S 

S_UPPER_DEPS += \
./SRC/Startup/startup_ch32v30x_D8.d \
./SRC/Startup/startup_ch32v30x_D8C.d 

OBJS += \
./SRC/Startup/startup_ch32v30x_D8.o \
./SRC/Startup/startup_ch32v30x_D8C.o 

DIR_OBJS += \
./SRC/Startup/*.o \

DIR_DEPS += \
./SRC/Startup/*.d \

DIR_EXPANDS += \
./SRC/Startup/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
SRC/Startup/%.o: ../SRC/Startup/%.S
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -x assembler-with-cpp -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

