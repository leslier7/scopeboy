# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
set(PICO_BOARD pico CACHE STRING "Board type")

option(PICO_HDMI_RUNTIME_MODES "Use runtime video mode switching (video_output_rt)" OFF)
option(PICO_HDMI_RT_RUNTIME_MODE_ATTRS "Allow runtime modes with different sync polarity and Data Island placement" OFF)
option(PICO_HDMI_DUMP_COMMAND_LISTS "Print generated HDMI/HSTX command lists at startup" OFF)

if(PICO_HDMI_RUNTIME_MODES)
    set(PICO_HDMI_VIDEO_OUTPUT_SRC src/video_output_rt.c)
else()
    set(PICO_HDMI_VIDEO_OUTPUT_SRC src/video_output.c)
endif()

add_library(pico_hdmi
    ${PICO_HDMI_VIDEO_OUTPUT_SRC}
    src/hstx_data_island_queue.c
    src/hstx_packet.c
)

target_include_directories(pico_hdmi PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(pico_hdmi
    pico_stdlib
    hardware_clocks
    hardware_dma
    hardware_irq
    hardware_gpio
)

if(PICO_HDMI_DUMP_COMMAND_LISTS)
    target_compile_definitions(pico_hdmi PRIVATE PICO_HDMI_DUMP_COMMAND_LISTS=1)
endif()

if(PICO_HDMI_RT_RUNTIME_MODE_ATTRS)
    target_compile_definitions(pico_hdmi PRIVATE PICO_HDMI_RT_RUNTIME_MODE_ATTRS=1)
endif()
