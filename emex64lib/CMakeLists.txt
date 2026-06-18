cmake_minimum_required(VERSION 3.16)

project(emex64
    VERSION 1.0.0
    LANGUAGES C
)

find_package(Threads REQUIRED)

option(EMEX64VM_DEVICE_DISPLAY "Enable display device support" ON)

if(EMEX64VM_DEVICE_DISPLAY)
    find_package(OpenGL)

    if(NOT OPENGL_FOUND)
        message(WARNING "Missing OpenGL — EMEX64VM_DEVICE_DISPLAY disabled
 If this is intentional, no further action is required.
 If you intend to have display support, please provide an OpenGL implementation.
 On most distributions, primarily ones based on Debian, this is named libgl1-mesa-dev.")
        set(EMEX64VM_DEVICE_DISPLAY OFF CACHE BOOL "" FORCE)
    elseif(UNIX AND NOT APPLE)
        find_package(glfw3)
        find_package(GLEW)

        if(NOT glfw3_FOUND OR NOT GLEW_FOUND)
            message(WARNING "Missing GLFW and/or GLEW — EMEX64VM_DEVICE_DISPLAY disabled
 If this is intentional, no further action is required.
 If you intend to have display support, please provide the headers for GLFW and GLEW.
 On most distributions, primarily ones based on Debian, these are named libglfw3-dev and libglew-dev.")
            set(EMEX64VM_DEVICE_DISPLAY OFF CACHE BOOL "" FORCE)
        endif()
    elseif(APPLE)
        find_library(COCOA_FRAMEWORK Cocoa)
        find_library(OPENGL_FRAMEWORK OpenGL)

        if(NOT COCOA_FRAMEWORK OR NOT OPENGL_FRAMEWORK)
            message(WARNING "Missing Cocoa and/or OpenGL frameworks — EMEX64VM_DEVICE_DISPLAY disabled
 If this is intentional, no further action is required.
 This usually means the Xcode Command Line Tools are not installed.
 You can install them with 'xcode-select --install'.")
            set(EMEX64VM_DEVICE_DISPLAY OFF CACHE BOOL "" FORCE)
        endif()
    endif()
endif()

option(EMEX64LIB_STATIC "Link statically against emex64lib" OFF)
option(EMEX64LIB_NATIVE_ARCH "Optimizations for the host CPU" OFF)
option(EMEX64LIB_DEBUG "Debugging" OFF)
option(EMEX64LIB_COMPILER_OPTIMIZATIONS "Appends compiler optimization flags" ON)

if(EMEX64LIB_STATIC)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

add_library(emex64 ${LIB_TYPE}
    # Support

    src/support/virtual/vpage.c
    src/support/virtual/vpageobj.c
    src/support/virtual/vfd.c

    src/support/parser.c
    src/support/bitwalker.c
    src/support/fdwalker.c
    src/support/diag.c
    src/support/file.c
    src/support/bitbolt.c

    # Virtual Machine

    src/vm/core.c
    src/vm/machine.c
    src/vm/memory.c
    src/vm/mmio.c
    src/vm/instruction/core.c
    src/vm/instruction/data.c
    src/vm/instruction/alu.c
    src/vm/instruction/ctrl.c

    # Virtual Machine Devices

    src/vm/device/internal/controller/ic.c
    src/vm/device/internal/controller/mem.c
    src/vm/device/internal/timer.c

    src/vm/device/board/controller/power.c
    src/vm/device/board/controller/8042.c
    # src/vm/device/board/controller/gfx.c
    # src/vm/device/board/gfx/vimmio.c
    src/vm/device/board/display.c
    src/vm/device/board/uart.c
    src/vm/device/board/rtc.c

    # Assembler

    src/asm/driver.c
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

    # Linker

    src/linker/driver.c
    src/linker/linker.c
    src/linker/sym.c
    src/linker/obj.c
    src/linker/options.c
    src/linker/script.c
    src/linker/emit.c

    # C Compiler

    src/c/ASTUnit.c
)

target_compile_definitions(emex64 PRIVATE
    EMEX64VM_DEVICE_DISPLAY=$<BOOL:${EMEX64VM_DEVICE_DISPLAY}>
    EMEX64VM_DEVICE_AUDIO=$<BOOL:${EMEX64VM_DEVICE_AUDIO}>
)

target_include_directories(emex64
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(emex64
    PUBLIC evObj
)

target_compile_features(emex64 PUBLIC c_std_23)

if (MSVC)
    # Fuck MSVC nobody likes you
    message(FATAL_ERROR "MSVC is not supported. Switch to Linux moron, microslop sheep!")
else()

    # Setting up compiler flags
    set(_EMEX64_WARN_FLAGS -Wall -Wextra -Wpedantic -Wno-unused-parameter)
    set(_EMEX64_OPT_FLAGS -fPIC)

    if(EMEX64LIB_COMPILER_OPTIMIZATIONS)
        list(APPEND _EMEX64_OPT_FLAGS -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -funroll-loops)
    endif()

    if(EMEX64LIB_NATIVE_ARCH)
        list(APPEND _EMEX64_OPT_FLAGS -march=native -mtune=native)
    endif()

    if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
        list(APPEND _EMEX64_OPT_FLAGS -fgraphite-identity -floop-nest-optimize -fipa-pta)
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        list(APPEND _EMEX64_OPT_FLAGS -fvectorize -fslp-vectorize)
    endif()

    if(EMEX64LIB_DEBUG)
        list(APPEND _EMEX64_OPT_FLAGS -g)
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
                src/vm/device/board/display.m
        )

        find_library(COCOA_FRAMEWORK Cocoa REQUIRED)
        find_library(OPENGL_FRAMEWORK OpenGL REQUIRED)

        target_link_libraries(emex64
            PRIVATE
                ${COCOA_FRAMEWORK}
                ${OPENGL_FRAMEWORK}
        )

        set_source_files_properties(src/vm/device/board/display.m PROPERTIES
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
