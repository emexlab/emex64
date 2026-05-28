cmake_minimum_required(VERSION 3.16)

project(EMEX64VM LANGUAGES C)

add_executable(emex64vm
    src/main.c

    src/core.c
    src/machine.c
    src/memory.c
    src/mmio.c
    src/mmu.c

    src/device/timer.c
    src/device/interrupt.c
    src/device/uart.c
    src/device/rtc.c
    src/device/mc.c
    src/device/platform.c
    src/device/display.c

    src/instruction/core.c
    src/instruction/data.c
    src/instruction/alu.c
    src/instruction/ctrl.c
)

target_include_directories(emex64vm
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(emex64vm
    PRIVATE emexutils
)

if(UNIX AND NOT APPLE)
    target_link_libraries(emex64vm
        PRIVATE glfw
        PRIVATE GLEW
        PRIVATE GL
    )
endif()

if(APPLE)
    target_sources(emex64vm
        PRIVATE
            src/device/display.m
    )

    find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
    find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)

    target_link_libraries(emex64vm
        PRIVATE
            ${COCOA_FRAMEWORK}
            ${OPENGL_FRAMEWORK}
    )

    set_source_files_properties(src/device/display.m PROPERTIES
        COMPILE_FLAGS "-fobjc-arc"
    )
endif()

target_compile_features(emex64vm PRIVATE c_std_99)

install(TARGETS emex64vm
    RUNTIME DESTINATION bin
)
