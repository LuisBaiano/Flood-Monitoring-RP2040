# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.1.1)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.1.1)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================


cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(PICO_BOARD pico_w CACHE STRING "Board type")
include(pico_sdk_import.cmake)
set(FREERTOS_KERNEL_PATH "/home/luis/pico_projects/residencia/FreeRTOS-Kernel")
include(${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake)

project(main C CXX ASM)
pico_sdk_init()


# *** Update include directories ***
include_directories(
    include             # Top-level include (if any non-hw headers exist)
    # Location of hardware_config.h and others
    src                 # Main source directory
    src/include # Hardware source directory (sometimes needed)
    lib/ssd1306         # Display library header
    ${CMAKE_BINARY_DIR} # Needed for generated pio header
)

# *** Update executable sources with new paths ***
add_executable(main
        main.c
        include/buttons.c
        include/buzzer.c
        include/debouncer.c
        include/display.c
        include/led_matrix.c
        include/joystick.c
        include/lib/ssd1306/ssd1306.c
        )

pico_generate_pio_header(main ${CMAKE_CURRENT_SOURCE_DIR}/include/pio/led_matrix.pio)

# Link necessary libraries (should be mostly the same)
target_link_libraries(main
        pico_stdlib
        hardware_gpio
        hardware_i2c
        hardware_pwm
        hardware_clocks
        hardware_irq
        hardware_pio
        hardware_adc
        FreeRTOS-Kernel       
        FreeRTOS-Kernel-Heap4
        pico_bootrom
        )

pico_enable_stdio_usb(main 1)
pico_enable_stdio_uart(main 0)
pico_add_extra_outputs(main)