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
option(EMEX64LIB_COMPILER_OPTIMIZATIONS "Appends compiler optimization flags" ON)

if(EMEX64LIB_STATIC)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

add_library(emex64 ${LIB_TYPE}
    # Support API
    src/support/parser.c
    src/support/file.c
    src/support/bitbolt.c

        ## Diagnostic API (Errors, Warnings, Notes, etc ^^)
        src/support/diagnostic/diagnostic.c
        src/support/diagnostic/consumer.c

        ## Virtual API (Virtualized UNIX Primitives)
        src/support/virtual/vpage.c
        src/support/virtual/vpageobj.c
        src/support/virtual/vfd.c
        src/support/virtual/vbitwalker.c

        ## Ratchet API (Safe argument array construction, etc)
        src/support/ratchet/args.c

        ## Hashmap API
        src/support/hashmap/hashmap.c

    # Virtual Machine API
    src/vm/core.c
    src/vm/machine.c
    src/vm/memory.c
    src/vm/mmio.c

        ## Instruction Handlers
        src/vm/instruction/core.c
        src/vm/instruction/data.c
        src/vm/instruction/alu.c
        src/vm/instruction/ctrl.c

        ## Devices

            ### SoC internal devices
            src/vm/device/internal/controller/ic.c
            src/vm/device/internal/controller/mem.c
            src/vm/device/internal/timer.c

            ### Board devices
            src/vm/device/board/controller/power.c
            src/vm/device/board/controller/8042.c
            #src/vm/device/board/controller/gfx.c
            #src/vm/device/board/gfx/vimmio.c
            src/vm/device/board/display.c
            src/vm/device/board/uart.c
            src/vm/device/board/rtc.c

    # Assembler API
    src/asm/driver.c
    src/asm/lexer.c
    src/asm/expr.c
    src/asm/code.c
    src/asm/options.c
    src/asm/invocation.c
    src/asm/section.c

        ## Diagnostic API (Errors, Warnings, Notes, etc ^^)
        src/asm/diagnostic/consumer.c

        ## Preprocessor API
        src/asm/preprocessor/directive.c
        src/asm/preprocessor/macro.c
        src/asm/preprocessor/condition.c
        src/asm/preprocessor/preprocessor.c

        ## Label API
        src/asm/label/label.c
        src/asm/label/relocate.c

        ## Emitter API
        src/asm/emitter/emitter.c
        src/asm/emitter/immediate.c
        src/asm/emitter/opcode.c
        src/asm/emitter/register.c
        src/asm/emitter/elf.c

    # Linker API
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

set(EMEX64_VERSION_TAG "INDEV")
set(EMEX64_VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/emex64lib/support/version.h)
add_custom_target(emex64_header_gen
    BYPRODUCTS ${EMEX64_VERSION_HEADER}
    COMMAND ${CMAKE_COMMAND}
        -DEMEX64_VERSION=${PROJECT_VERSION}
        -DEMEX64_VERSION_TAG=${EMEX64_VERSION_TAG}
        -DPROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        -DPROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        -DPROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        -DSRC=${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
        -DDST=${CMAKE_CURRENT_BINARY_DIR}/include/emex64lib/support/version.h
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gen_hdr.cmake
    COMMENT "generating headers")

set_source_files_properties(${EMEX64_VERSION_HEADER} PROPERTIES GENERATED TRUE)
set_source_files_properties(src/asm/driver.c PROPERTIES OBJECT_DEPENDS ${EMEX64_VERSION_HEADER})
set_source_files_properties(src/linker/driver.c PROPERTIES OBJECT_DEPENDS ${EMEX64_VERSION_HEADER})

add_dependencies(emex64 emex64_header_gen)

target_include_directories(emex64
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/emex64lib/support/version.h
    DESTINATION include/emex64lib/support)

target_link_libraries(emex64
    PUBLIC evObj
)

target_compile_features(emex64 PUBLIC c_std_23)

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

if(APPLE)
    option(EMEX64LIB_STATIC "Link statically against emex64" OFF)
else()
    option(EMEX64LIB_STATIC "Link statically against emex64" ON)
endif()

install(TARGETS emex64
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/
    DESTINATION include
    FILES_MATCHING
        PATTERN "*.h"
)
