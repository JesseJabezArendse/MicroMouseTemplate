# ARM Cortex-M Toolchain for CMake
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

# Toolchain prefix
set(TOOLCHAIN_PREFIX "arm-none-eabi-")

# Get toolchain path from cache variable or environment
if(ARM_TOOLCHAIN_PATH)
    set(TOOLCHAIN_PATH ${ARM_TOOLCHAIN_PATH})
    message(STATUS "Using ARM Toolchain Path: ${TOOLCHAIN_PATH}")
elseif(DEFINED ENV{ARM_TOOLCHAIN_PATH})
    set(TOOLCHAIN_PATH $ENV{ARM_TOOLCHAIN_PATH})
    message(STATUS "Using ARM Toolchain Path from environment: ${TOOLCHAIN_PATH}")
else()
    # Try to find common STM32 toolchain locations
    if(WIN32)
        # Common STM32CubeIDE locations on Windows
        if(EXISTS "C:/ST/STM32CubeIDE_1.13.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.11.3.rel1.win32_1.1.0.202305231506/tools/bin")
            set(TOOLCHAIN_PATH "C:/ST/STM32CubeIDE_1.13.1/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.11.3.rel1.win32_1.1.0.202305231506/tools/bin")
            message(STATUS "Found ARM Toolchain at: ${TOOLCHAIN_PATH}")
        else()
            message(WARNING "ARM Toolchain not found. Please set ARM_TOOLCHAIN_PATH")
        endif()
    endif()
endif()

# Set compilers with full paths if toolchain path is known
if(TOOLCHAIN_PATH)
    set(CMAKE_C_COMPILER "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}gcc.exe" CACHE FILEPATH "C compiler")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}g++.exe" CACHE FILEPATH "C++ compiler")
    set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}gcc.exe" CACHE FILEPATH "ASM compiler")
    set(CMAKE_OBJCOPY "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}objcopy.exe" CACHE FILEPATH "Objcopy tool")
    set(CMAKE_OBJDUMP "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}objdump.exe" CACHE FILEPATH "Objdump tool")
    set(CMAKE_SIZE "${TOOLCHAIN_PATH}/${TOOLCHAIN_PREFIX}size.exe" CACHE FILEPATH "Size tool")
else()
    # Fallback: assume tools are in PATH (without .exe extension for cross-platform compatibility)
    set(CMAKE_C_COMPILER "${TOOLCHAIN_PREFIX}gcc" CACHE FILEPATH "C compiler")
    set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PREFIX}g++" CACHE FILEPATH "C++ compiler")
    set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PREFIX}gcc" CACHE FILEPATH "ASM compiler")
    set(CMAKE_OBJCOPY "${TOOLCHAIN_PREFIX}objcopy" CACHE FILEPATH "Objcopy tool")
    set(CMAKE_OBJDUMP "${TOOLCHAIN_PREFIX}objdump" CACHE FILEPATH "Objdump tool")
    set(CMAKE_SIZE "${TOOLCHAIN_PREFIX}size" CACHE FILEPATH "Size tool")
endif()

# Set flags
set(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_CXX_FLAGS_INIT "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")
set(CMAKE_ASM_FLAGS_INIT "-mcpu=cortex-m4 -mthumb -mfpu=fpv4-sp-d16 -mfloat-abi=hard")

# Don't run linker checks
set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

# Search in the toolchain directory first if set
if(TOOLCHAIN_PATH)
    set(CMAKE_FIND_ROOT_PATH ${TOOLCHAIN_PATH})
endif()
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
