cmake_minimum_required(VERSION 3.10)

# Project configuration
project(MicroMouseProgramming C ASM)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Target name
set(TARGET_NAME MicroMouseProgramming)
set(BUILD_DIR ${CMAKE_BINARY_DIR})

# Debug configuration
set(DEBUG 1)
set(OPT -Og)

# ============================================
# CPU and Hardware Configuration
# ============================================
set(CPU_FLAGS "-mcpu=cortex-m4")
set(FPU_FLAGS "-mfpu=fpv4-sp-d16")
set(FLOAT_ABI_FLAGS "-mfloat-abi=hard")
set(THUMB_FLAGS "-mthumb")

set(MCU_FLAGS "${CPU_FLAGS} ${THUMB_FLAGS} ${FPU_FLAGS} ${FLOAT_ABI_FLAGS}")

# ============================================
# C Sources
# ============================================
set(C_SOURCES
    Core/Src/main.c
    Core/Src/stm32l4xx_it.c
    Core/Src/stm32l4xx_hal_msp.c
    Core/Src/system_stm32l4xx.c
    Core/Src/stm32l4xx_hal_timebase_tim.c
    Core/Src/ADCs.c
    Core/Src/Buttons.c
    Core/Src/IMU.c
    Core/Src/INA219.c
    Core/Src/LEDs.c
    Core/Src/Motors.c
    Core/Src/preformatted_flash.c
    Core/Src/SSD1306.c
    Core/Src/VL53L0X.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rcc.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_rcc_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_flash_ramfunc.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_gpio.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_dma.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_dma_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_pwr_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_cortex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_exti.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_tim.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_tim_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_i2c.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_i2c_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_uart.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_uart_ex.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_adc.c
    Drivers/STM32L4xx_HAL_Driver/Src/stm32l4xx_hal_adc_ex.c
)

# ============================================
# ASM Sources
# ============================================
set(ASM_SOURCES
    startup_stm32l476xx.s
)

# ============================================
# Include Directories
# ============================================
set(INCLUDE_DIRS
    Core/Inc
    Drivers/STM32L4xx_HAL_Driver/Inc
    Drivers/STM32L4xx_HAL_Driver/Inc/Legacy
    Drivers/CMSIS/Device/ST/STM32L4xx/Include
    Drivers/CMSIS/Include
)

# ============================================
# C Defines
# ============================================
set(C_DEFS
    USE_HAL_DRIVER
    STM32L476xx
)

# ============================================
# Compiler Flags
# ============================================
set(CMAKE_C_FLAGS "${MCU_FLAGS} ${OPT} -Wall -fdata-sections -ffunction-sections")
set(CMAKE_ASM_FLAGS "${MCU_FLAGS} ${OPT} -Wall -fdata-sections -ffunction-sections")

if(DEBUG)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -gdwarf-2")
endif()

# Generate dependency information
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -MMD -MP")

# ============================================
# Create Executable
# ============================================
add_executable(${TARGET_NAME}.elf
    ${C_SOURCES}
    ${ASM_SOURCES}
)

# Add include directories
target_include_directories(${TARGET_NAME}.elf PRIVATE ${INCLUDE_DIRS})

# Add compiler definitions
target_compile_definitions(${TARGET_NAME}.elf PRIVATE ${C_DEFS})

# ============================================
# Linker Configuration
# ============================================
set(LDSCRIPT "${CMAKE_SOURCE_DIR}/STM32L476XX_FLASH.ld")

target_link_options(${TARGET_NAME}.elf PRIVATE
    "SHELL:-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard"
    -specs=nano.specs
    -T${LDSCRIPT}
    -lc -lm -lnosys
    "-Wl,-Map=${BUILD_DIR}/${TARGET_NAME}.map,--cref"
    -Wl,--gc-sections
)

# ============================================
# Custom Commands for HEX and BIN Files
# ============================================
add_custom_command(TARGET ${TARGET_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O ihex ${BUILD_DIR}/${TARGET_NAME}.elf ${BUILD_DIR}/${TARGET_NAME}.hex
    COMMAND ${CMAKE_OBJCOPY} -O binary -S ${BUILD_DIR}/${TARGET_NAME}.elf ${BUILD_DIR}/${TARGET_NAME}.bin
    COMMAND ${CMAKE_SIZE} ${BUILD_DIR}/${TARGET_NAME}.elf
    COMMENT "Building ${TARGET_NAME}.hex and ${TARGET_NAME}.bin"
)

# ============================================
# Custom Targets
# ============================================
add_custom_target(memory
    COMMAND ${CMAKE_SIZE} ${BUILD_DIR}/${TARGET_NAME}.elf
    DEPENDS ${TARGET_NAME}.elf
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -P "${CMAKE_BINARY_DIR}/cmake_clean.cmake"
    COMMENT "Cleaning build directory"
)
