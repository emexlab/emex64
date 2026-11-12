cmake_minimum_required(VERSION 3.16)

project(EmexToolchain
    VERSION 1.0.0
    LANGUAGES C
)

find_package(Threads REQUIRED)

option(EMEX64VM_DEVICE_DISPLAY "Enable display device support" ON)

if(EMEX64VM_DEVICE_DISPLAY AND APPLE)
    find_library(COCOA_FRAMEWORK Cocoa)
    find_library(METAL_FRAMEWORK Metal)
    find_library(METALKIT_FRAMEWORK MetalKit)
    find_library(QUARTZCORE_FRAMEWORK QuartzCore)

    if(NOT COCOA_FRAMEWORK OR NOT METAL_FRAMEWORK OR NOT METALKIT_FRAMEWORK OR NOT QUARTZCORE_FRAMEWORK)
        message(WARNING "Missing Cocoa, Metal, MetalKit and/or QuartzCore frameworks — EMEX64VM_DEVICE_DISPLAY disabled
 If this is intentional, no further action is required.
 This usually means the Xcode Command Line Tools are not installed.
 You can install them with 'xcode-select --install'.")
        set(EMEX64VM_DEVICE_DISPLAY OFF CACHE BOOL "" FORCE)
    endif()
elseif(EMEX64VM_DEVICE_DISPLAY)
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
    endif()
endif()

option(EMEX64VM_NET "Network controller with user mode NAT (libslirp)" ON)

if(EMEX64VM_NET)
    find_package(PkgConfig)
    if(PkgConfig_FOUND)
        pkg_check_modules(SLIRP IMPORTED_TARGET slirp)
    endif()

    if(NOT SLIRP_FOUND)
        message(WARNING "Missing libslirp — EMEX64VM_NET disabled
 If this is intentional, no further action is required.
 For the network controller (--net user). brew install libslirp on macOS,
 libslirp-dev on distributions based on Debian.")
        set(EMEX64VM_NET OFF CACHE BOOL "" FORCE)
    endif()
endif()

option(EMEX64LIB_STATIC "Link statically against EmexToolchain" OFF)
option(EMEX64LIB_NATIVE_ARCH "Optimizations for the host CPU" OFF)
option(EMEX64LIB_COMPILER_OPTIMIZATIONS "Appends compiler optimization flags" ON)

if(EMEX64LIB_STATIC)
    set(LIB_TYPE STATIC)
else()
    set(LIB_TYPE SHARED)
endif()

add_library(EmexToolchain ${LIB_TYPE}
    # Support API
    src/Support/parser.c
    src/Support/bitbolt.c

        ## Diagnostic API (Errors, Warnings, Notes, etc ^^)
        src/Support/diagnostic/diagnostic.c
        src/Support/diagnostic/consumer.c

        ## Ratchet API (Safe argument array construction, etc)
        src/Support/ratchet/args.c

        ## Hashmap API
        src/Support/hashmap/hashmap.c

    # Virtual Machine API
    src/VM/E64MachineOptions.c
    src/VM/E64Machine.c
    src/VM/E64Memory.c
    src/VM/E64MMIORegion.c
    src/VM/E64MMIOBus.c
    src/VM/E64Core.c

        ## Instruction Handlers
        src/VM/instruction/core.c
        src/VM/instruction/data.c
        src/VM/instruction/alu.c
        src/VM/instruction/ctrl.c
        src/VM/instruction/fpu.c

        ## Devices

            ### SoC internal devices
            src/VM/device/internal/controller/E64IC.c
            src/VM/device/internal/controller/mem.c
            src/VM/device/internal/timer.c
            src/VM/device/internal/cpuid.c
            src/VM/device/internal/controller/apic.c

            ### Board devices
            src/VM/device/board/controller/power.c
            src/VM/device/board/controller/8042.c
            #src/vm/device/board/controller/gfx.c
            #src/vm/device/board/gfx/vimmio.c
            src/VM/device/board/display.c
            src/VM/device/board/uart.c
            src/VM/device/board/rtc.c

            ### PCIe devices
            src/VM/device/board/pcie/pcie.c
            src/VM/device/board/pcie/sata.c
            src/VM/device/board/pcie/usb.c
            src/VM/device/board/pcie/usb_msc.c
            src/VM/device/board/pcie/gpu.c
            src/VM/device/board/pcie/net.c

    # Assembler API
    src/ETAssembler/ETAssemblerOptions.c
    src/ETAssembler/ETAssemblerJob.c
    src/ETAssembler/ETAssemblerDriver.c
    src/ETAssembler/ETAssemblerInvocation.c
    src/ETAssembler/ETAssemblerDiagnosticConsumer.c
    src/ETAssembler/lexer.c
    src/ETAssembler/expr.c
    src/ETAssembler/code.c
    src/ETAssembler/section.c

        ## Diagnostic API (Errors, Warnings, Notes, etc ^^)
        src/ETAssembler/diagnostic/consumer.c

        ## Preprocessor API
        src/ETAssembler/preprocessor/directive.c
        src/ETAssembler/preprocessor/macro.c
        src/ETAssembler/preprocessor/condition.c
        src/ETAssembler/preprocessor/preprocessor.c

        ## Label API
        src/ETAssembler/label/label.c
        src/ETAssembler/label/relocate.c

        ## Emitter API
        src/ETAssembler/emitter/emitter.c
        src/ETAssembler/emitter/immediate.c
        src/ETAssembler/emitter/opcode.c
        src/ETAssembler/emitter/register.c
        src/ETAssembler/emitter/elf.c

    # Linker API
    src/ETLinker/driver.c
    src/ETLinker/linker.c
    src/ETLinker/sym.c
    src/ETLinker/obj.c
    src/ETLinker/options.c
    src/ETLinker/script.c
    src/ETLinker/emit.c

        ## Diagnostic API (Errors, Warnings, Notes, etc ^^)
        src/ETLinker/diagnostic/consumer.c
)

target_compile_definitions(EmexToolchain PRIVATE
    EMEX64VM_DEVICE_DISPLAY=$<BOOL:${EMEX64VM_DEVICE_DISPLAY}>
    EMEX64VM_DEVICE_AUDIO=$<BOOL:${EMEX64VM_DEVICE_AUDIO}>
    EMEX64VM_NET=$<BOOL:${EMEX64VM_NET}>
    ET_PRIVATE=$<BOOL:YES>
)

set(EMEX64_VERSION_TAG "INDEV")
set(EMEX64_VERSION_HEADER ${CMAKE_CURRENT_BINARY_DIR}/include/EmexToolchain/Support/version.h)
add_custom_target(emex64_header_gen
    BYPRODUCTS ${EMEX64_VERSION_HEADER}
    COMMAND ${CMAKE_COMMAND}
        -DEMEX64_VERSION=${PROJECT_VERSION}
        -DEMEX64_VERSION_TAG=${EMEX64_VERSION_TAG}
        -DPROJECT_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
        -DPROJECT_VERSION_MINOR=${PROJECT_VERSION_MINOR}
        -DPROJECT_VERSION_PATCH=${PROJECT_VERSION_PATCH}
        -DSRC=${CMAKE_CURRENT_SOURCE_DIR}/cmake/version.h.in
        -DDST=${CMAKE_CURRENT_BINARY_DIR}/include/EmexToolchain/Support/version.h
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/gen_hdr.cmake
    COMMENT "generating headers")

set_source_files_properties(${EMEX64_VERSION_HEADER} PROPERTIES GENERATED TRUE)
set_source_files_properties(src/asm/driver.c PROPERTIES OBJECT_DEPENDS ${EMEX64_VERSION_HEADER})
set_source_files_properties(src/linker/driver.c PROPERTIES OBJECT_DEPENDS ${EMEX64_VERSION_HEADER})

add_dependencies(EmexToolchain emex64_header_gen)

target_include_directories(EmexToolchain
    PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/private>
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/public>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/include/public>
        $<INSTALL_INTERFACE:include>
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/include/EmexToolchain/Support/version.h
    DESTINATION include/EmexToolchain/support)

target_link_libraries(EmexToolchain
    PUBLIC EmexFoundation
)

target_compile_features(EmexToolchain PUBLIC c_std_23)

set_target_properties(EmexToolchain PROPERTIES
    C_EXTENSIONS ON
)

# To every maintainer that is confused where the block is with all flags and shit, it is in the root cmake file

target_link_libraries(EmexToolchain
    PRIVATE
        Threads::Threads
)

if(UNIX AND NOT APPLE)
    target_link_libraries(EmexToolchain PRIVATE m)
endif()

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    set_source_files_properties(src/VM/instruction/fpu.c PROPERTIES COMPILE_OPTIONS "-fno-fast-math;-frounding-math")
endif()

if(EMEX64VM_NET)
    target_link_libraries(EmexToolchain PRIVATE PkgConfig::SLIRP)
endif()

if(EMEX64VM_DEVICE_DISPLAY)
    if(UNIX AND NOT APPLE)
        target_link_libraries(EmexToolchain
            PRIVATE
                glfw
                GLEW::GLEW
                OpenGL::GL
        )
    elseif(APPLE)
        target_sources(EmexToolchain
            PRIVATE
                src/vm/device/board/display.m
        )

        target_link_libraries(EmexToolchain
            PRIVATE
                ${COCOA_FRAMEWORK}
                ${METAL_FRAMEWORK}
                ${METALKIT_FRAMEWORK}
                ${QUARTZCORE_FRAMEWORK}
        )

        set_source_files_properties(src/vm/device/board/display.m PROPERTIES
            COMPILE_FLAGS "-fobjc-arc"
        )
    endif()
endif()

if(APPLE)
    option(EMEX64LIB_STATIC "Link statically against EmexToolchain" OFF)
else()
    option(EMEX64LIB_STATIC "Link statically against EmexToolchain" ON)
endif()

install(TARGETS EmexToolchain
    LIBRARY DESTINATION lib
    RUNTIME DESTINATION bin
    COMPONENT EmexToolchain
)

install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/public/
    DESTINATION include
    COMPONENT Headers
    FILES_MATCHING
        PATTERN "*.h"
)

set_target_properties(EmexToolchain PROPERTIES
    INSTALL_NAME_DIR "/usr/local/lib"
)
