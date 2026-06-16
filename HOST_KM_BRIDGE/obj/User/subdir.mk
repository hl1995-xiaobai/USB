################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/ch32v30x_it.c \
../User/main.c \
../User/submouse_serial.c \
../User/system_ch32v30x.c \
../User/usbd_hs_mouse.c 

C_DEPS += \
./User/ch32v30x_it.d \
./User/main.d \
./User/submouse_serial.d \
./User/system_ch32v30x.d \
./User/usbd_hs_mouse.d 

OBJS += \
./User/ch32v30x_it.o \
./User/main.o \
./User/submouse_serial.o \
./User/system_ch32v30x.o \
./User/usbd_hs_mouse.o 

DIR_OBJS += \
./User/*.o \

DIR_DEPS += \
./User/*.d \

DIR_EXPANDS += \
./User/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
User/%.o: ../User/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/HOST_KM_BRIDGE/Debug" -I"d:/git_hl/USB/HOST_KM_BRIDGE/User/USB_Host" -I"d:/git_hl/USB/HOST_KM_BRIDGE/Core" -I"d:/git_hl/USB/HOST_KM_BRIDGE/User" -I"d:/git_hl/USB/HOST_KM_BRIDGE/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

