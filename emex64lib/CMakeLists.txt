cmake_minimum_required(VERSION 3.16)

project(emex64
    VERSION 1.0.0
    LANGUAGES C
)

find_package(Threads REQUIRED)

option(EMEX64VM_DEVICE_AUDIO "Enable AC97 audio output (PulseAudio on Linux)" ON)

if(EMEX64VM_DEVICE_AUDIO)
    if(UNIX AND NOT APPLE)
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(PULSEAUDIO libpulse-simple libpulse)
        if(NOT PULSEAUDIO_FOUND)
            message(WARNING "PulseAudio not found — EMEX64VM_DEVICE_AUDIO disabled")
            set(EMEX64VM_DEVICE_AUDIO OFF CACHE BOOL "" FORCE)
        endif()
    elseif(APPLE)
        # macOS uses AudioQueue (CoreAudio), no extra find_package needed
    endif()
endif()

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

option(EMEX64LIB_STATIC "Link statically against emex64lib" OFF)
option(EMEX64_NATIVE_ARCH "Optimizations for the host CPU" OFF)

if(EMEX64LIB_STATIC)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

add_library(emex64 ${LIB_TYPE}
    # Support

    src/support/parser.c
    src/support/fdwalker.c
    src/support/diag.c
    src/support/file.c

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
    src/vm/device/8042.c
    src/vm/device/ac97.c

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
    src/asm/elf.c

    # C Compiler

    src/c/ASTUnit.c
)

option(EMEX64VM_USE_INSCACHE "Copies instruction into a cache before fetching it, for more correct emulation, it is noticably slower" ON)

target_compile_definitions(emex64 PRIVATE
    EMEX64VM_DEVICE_DISPLAY=$<BOOL:${EMEX64VM_DEVICE_DISPLAY}>
    EMEX64VM_DEVICE_AUDIO=$<BOOL:${EMEX64VM_DEVICE_AUDIO}>
    EMEX64VM_USE_INSCACHE=$<BOOL:${EMEX64VM_USE_INSCACHE}>
)

target_include_directories(emex64
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(emex64 PUBLIC c_std_23)

if (MSVC)
    # Fuck MSVC nobody likes you
    target_compile_options(emex64 PRIVATE /W4)
else()

    # Setting up compiler flags
    set(_EMEX64_WARN_FLAGS -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    set(_EMEX64_OPT_FLAGS -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -funroll-loops)
    
    if(EMEX64_NATIVE_ARCH)
        list(APPEND _EMEX64_OPT_FLAGS -march=native -mtune=native)
    endif()


    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        list(APPEND _EMEX64_OPT_FLAGS -fgraphite-identity -floop-nest-optimize -fipa-pta)
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        list(APPEND _EMEX64_OPT_FLAGS -fvectorize -fslp-vectorize)
    endif()
    
    include(CheckIPOSupported)
    check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_output)
    if(_ipo_supported)
        set_target_properties(emex64 PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "LTO not supported: ${_ipo_output}")
    endif()

    target_compile_options(emex64 PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
endif()

target_link_libraries(emex64
    PRIVATE
        Threads::Threads
)

if(EMEX64VM_DEVICE_DISPLAY)
    if(UNIX AND NOT APPLE)
        target_link_libraries(emex64
            PRIVATE
                glfw
                GLEW::GLEW
                OpenGL::GL
        )
    elseif(APPLE)
        target_sources(emex64
            PRIVATE
                src/vm/device/display.m
        )

        find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
        find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)

        target_link_libraries(emex64
            PRIVATE
                ${COCOA_FRAMEWORK}
                ${OPENGL_FRAMEWORK}
        )

        set_source_files_properties(src/vm/device/display.m PROPERTIES
            COMPILE_FLAGS "-fobjc-arc"
        )
    endif()
endif()

if(EMEX64VM_DEVICE_AUDIO)
    if(UNIX AND NOT APPLE)
        target_include_directories(emex64 PRIVATE ${PULSEAUDIO_INCLUDE_DIRS})
        target_link_libraries(emex64 PRIVATE ${PULSEAUDIO_LIBRARIES})
    elseif(APPLE)
        find_library(AUDIOTOOLBOX_FRAMEWORK AudioToolbox REQUIRED)
        target_link_libraries(emex64 PRIVATE ${AUDIOTOOLBOX_FRAMEWORK})
    endif()
endif()

if(APPLE)
    option(EMEX64LIB_STATIC "Link statically against emex64" OFF)
else()
    option(EMEX64LIB_STATIC "Link statically against emex64" ON)
endif()

if(NOT EMEX64LIB_STATIC)
    install(TARGETS emex64
        LIBRARY DESTINATION lib
        RUNTIME DESTINATION bin
    )

    install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
        DESTINATION include
        FILES_MATCHING
            PATTERN "*.h"
    )
endif()