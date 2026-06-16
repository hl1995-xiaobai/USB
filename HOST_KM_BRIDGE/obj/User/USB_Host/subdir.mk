################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../User/USB_Host/app_km.c \
../User/USB_Host/ch32v30x_usbfs_host.c \
../User/USB_Host/usb_host_hid.c \
../User/USB_Host/usb_host_hub.c 

C_DEPS += \
./User/USB_Host/app_km.d \
./User/USB_Host/ch32v30x_usbfs_host.d \
./User/USB_Host/usb_host_hid.d \
./User/USB_Host/usb_host_hub.d 

OBJS += \
./User/USB_Host/app_km.o \
./User/USB_Host/ch32v30x_usbfs_host.o \
./User/USB_Host/usb_host_hid.o \
./User/USB_Host/usb_host_hub.o 

DIR_OBJS += \
./User/USB_Host/*.o \

DIR_DEPS += \
./User/USB_Host/*.d \

DIR_EXPANDS += \
./User/USB_Host/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
User/USB_Host/%.o: ../User/USB_Host/%.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/HOST_KM_BRIDGE/Debug" -I"d:/git_hl/USB/HOST_KM_BRIDGE/User/USB_Host" -I"d:/git_hl/USB/HOST_KM_BRIDGE/Core" -I"d:/git_hl/USB/HOST_KM_BRIDGE/User" -I"d:/git_hl/USB/HOST_KM_BRIDGE/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

