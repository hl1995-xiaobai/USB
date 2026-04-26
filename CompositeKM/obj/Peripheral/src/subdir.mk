################################################################################
# MRS Version: 2.4.0
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_adc.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_bkp.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_can.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_crc.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_dac.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_dbgmcu.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_dma.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_dvp.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_eth.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_exti.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_flash.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_fsmc.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_gpio.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_i2c.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_iwdg.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_misc.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_opa.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_pwr.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_rcc.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_rng.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_rtc.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_sdio.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_spi.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_tim.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_usart.c \
d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_wwdg.c 

C_DEPS += \
./Peripheral/src/ch32v30x_adc.d \
./Peripheral/src/ch32v30x_bkp.d \
./Peripheral/src/ch32v30x_can.d \
./Peripheral/src/ch32v30x_crc.d \
./Peripheral/src/ch32v30x_dac.d \
./Peripheral/src/ch32v30x_dbgmcu.d \
./Peripheral/src/ch32v30x_dma.d \
./Peripheral/src/ch32v30x_dvp.d \
./Peripheral/src/ch32v30x_eth.d \
./Peripheral/src/ch32v30x_exti.d \
./Peripheral/src/ch32v30x_flash.d \
./Peripheral/src/ch32v30x_fsmc.d \
./Peripheral/src/ch32v30x_gpio.d \
./Peripheral/src/ch32v30x_i2c.d \
./Peripheral/src/ch32v30x_iwdg.d \
./Peripheral/src/ch32v30x_misc.d \
./Peripheral/src/ch32v30x_opa.d \
./Peripheral/src/ch32v30x_pwr.d \
./Peripheral/src/ch32v30x_rcc.d \
./Peripheral/src/ch32v30x_rng.d \
./Peripheral/src/ch32v30x_rtc.d \
./Peripheral/src/ch32v30x_sdio.d \
./Peripheral/src/ch32v30x_spi.d \
./Peripheral/src/ch32v30x_tim.d \
./Peripheral/src/ch32v30x_usart.d \
./Peripheral/src/ch32v30x_wwdg.d 

OBJS += \
./Peripheral/src/ch32v30x_adc.o \
./Peripheral/src/ch32v30x_bkp.o \
./Peripheral/src/ch32v30x_can.o \
./Peripheral/src/ch32v30x_crc.o \
./Peripheral/src/ch32v30x_dac.o \
./Peripheral/src/ch32v30x_dbgmcu.o \
./Peripheral/src/ch32v30x_dma.o \
./Peripheral/src/ch32v30x_dvp.o \
./Peripheral/src/ch32v30x_eth.o \
./Peripheral/src/ch32v30x_exti.o \
./Peripheral/src/ch32v30x_flash.o \
./Peripheral/src/ch32v30x_fsmc.o \
./Peripheral/src/ch32v30x_gpio.o \
./Peripheral/src/ch32v30x_i2c.o \
./Peripheral/src/ch32v30x_iwdg.o \
./Peripheral/src/ch32v30x_misc.o \
./Peripheral/src/ch32v30x_opa.o \
./Peripheral/src/ch32v30x_pwr.o \
./Peripheral/src/ch32v30x_rcc.o \
./Peripheral/src/ch32v30x_rng.o \
./Peripheral/src/ch32v30x_rtc.o \
./Peripheral/src/ch32v30x_sdio.o \
./Peripheral/src/ch32v30x_spi.o \
./Peripheral/src/ch32v30x_tim.o \
./Peripheral/src/ch32v30x_usart.o \
./Peripheral/src/ch32v30x_wwdg.o 

DIR_OBJS += \
./Peripheral/src/*.o \

DIR_DEPS += \
./Peripheral/src/*.d \

DIR_EXPANDS += \
./Peripheral/src/*.234r.expand \


# Each subdirectory must supply rules for building sources it contributes
Peripheral/src/ch32v30x_adc.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_adc.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_bkp.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_bkp.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_can.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_can.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_crc.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_crc.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_dac.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_dac.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_dbgmcu.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_dbgmcu.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_dma.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_dma.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_dvp.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_dvp.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_eth.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_eth.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_exti.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_exti.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_flash.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_flash.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_fsmc.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_fsmc.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_gpio.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_gpio.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_i2c.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_i2c.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_iwdg.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_iwdg.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_misc.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_misc.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_opa.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_opa.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_pwr.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_pwr.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_rcc.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_rcc.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_rng.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_rng.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_rtc.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_rtc.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_sdio.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_sdio.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_spi.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_spi.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_tim.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_tim.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_usart.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_usart.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
Peripheral/src/ch32v30x_wwdg.o: d:/git_hl/USB/SRC/Peripheral/src/ch32v30x_wwdg.c
	@	riscv-none-embed-gcc -march=rv32imacxw -mabi=ilp32 -msmall-data-limit=8 -msave-restore -fmax-errors=20 -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -Wunused -Wuninitialized -Wextra -g -I"d:/git_hl/USB/SRC/Debug" -I"d:/git_hl/USB/SRC/Core" -I"d:/git_hl/USB/CompositeKM/User" -I"d:/git_hl/USB/SRC/Peripheral/inc" -std=gnu99 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"

