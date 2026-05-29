cmake_minimum_required(VERSION 3.16)

project(emex64lib
    VERSION 1.0.0
    LANGUAGES C
)

option(EMEX64VM_DEVICE_DISPLAY "Enable display device support" ON)

if(EMEX64VM_DEVICE_DISPLAY)
    find_package(OpenGL REQUIRED)

    if(UNIX AND NOT APPLE)
        find_package(glfw3 REQUIRED)
        find_package(GLEW REQUIRED)
    elseif(APPLE)
        find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
        find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)
    endif()
endif()

if(APPLE)
    option(EMEX64LIB_STATIC "Link statically against emex64lib" OFF)
else()
    option(EMEX64LIB_STATIC "Link statically against emex64lib" ON)
endif()

if(EMEX64LIB_STATIC)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

add_library(emex64lib ${LIB_TYPE}
    # Support

    src/support/bitwalker.c
    src/support/parser.c
    src/support/fdwalker.c

    # Virtual Machine

    src/vm/core.c
    src/vm/machine.c
    src/vm/memory.c
    src/vm/mmio.c
    src/vm/mmu.c

    src/vm/device/timer.c
    src/vm/device/interrupt.c
    src/vm/device/uart.c
    src/vm/device/rtc.c
    src/vm/device/mc.c
    src/vm/device/platform.c
    src/vm/device/display.c

    src/vm/instruction/core.c
    src/vm/instruction/data.c
    src/vm/instruction/alu.c
    src/vm/instruction/ctrl.c

    # Assembler

    src/asm/cmptok.c
    src/asm/code.c
    src/asm/emit.c
    src/asm/options.c
    src/asm/invocation.c
    src/asm/label.c
    src/asm/opcode.c
    src/asm/register.c
    src/asm/section.c
    src/asm/macro.c
    src/asm/diag.c
)

target_compile_definitions(emex64lib PRIVATE
    EMEX64VM_DEVICE_DISPLAY=$<BOOL:${EMEX64VM_DEVICE_DISPLAY}>
)

target_include_directories(emex64lib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(emex64lib PUBLIC c_std_23)

if (MSVC)
    target_compile_options(emex64lib PRIVATE /W4)
else()
    target_compile_options(emex64lib PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

if(EMEX64VM_DEVICE_DISPLAY)
    if(UNIX AND NOT APPLE)
        target_link_libraries(emex64lib
            PRIVATE
                glfw
                GLEW::GLEW
                OpenGL::GL
        )
    elseif(APPLE)
        target_sources(emex64lib
            PRIVATE
                src/vm/device/display.m
        )

        find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
        find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)

        target_link_libraries(emex64lib
            PRIVATE
                ${COCOA_FRAMEWORK}
                ${OPENGL_FRAMEWORK}
        )

        set_source_files_properties(src/vm/device/display.m PROPERTIES
            COMPILE_FLAGS "-fobjc-arc"
        )
    endif()
endif()

install(TARGETS emex64lib
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)
