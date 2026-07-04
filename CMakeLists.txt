cmake_minimum_required(VERSION 3.16)

project(emex64Toolchain LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(EMEX64_BUILD_TOOLS "Builds tools that make interfacing the emex64 toolchain easy" ON)
option(EMEX64_BUILD_EXAMPLES "Builds examples that showcase how to use the emex64 toolchain" OFF)
option(EMEX64LIB_STATIC "Build EmexToolchain as a static library" OFF)

add_subdirectory(EmexFoundation)
add_subdirectory(EmexToolchain)

if(EMEX64_BUILD_TOOLS)
    add_subdirectory(tools/emex64vm)
    add_subdirectory(tools/emex64asm)
    add_subdirectory(tools/emex64ld)
endif()

if(EMEX64_BUILD_EXAMPLES)
    add_subdirectory(examples/emex64hello)
endif()

if(NOT EMEX64LIB_STATIC)
    if(APPLE)
        set(CMAKE_MACOSX_RPATH TRUE)
        set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
    
        if(EMEX64_BUILD_TOOLS)
            set_target_properties(emex64asm PROPERTIES INSTALL_RPATH "@loader_path/../lib")
            set_target_properties(emex64vm PROPERTIES INSTALL_RPATH "@loader_path/../lib")
            set_target_properties(emex64ld PROPERTIES INSTALL_RPATH "@loader_path/../lib")
        endif()

        if(EMEX64_BUILD_EXAMPLES)
            set_target_properties(emex64hello PROPERTIES INSTALL_RPATH "@loader_path/../lib")
        endif()
    elseif(UNIX)
        set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

        if(EMEX64_BUILD_TOOLS)
            set_target_properties(emex64asm PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
            set_target_properties(emex64vm PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
            set_target_properties(emex64ld PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
        endif()

        if(EMEX64_BUILD_EXAMPLES)
            set_target_properties(emex64hello PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
        endif()
    endif()
endif()

if(MSVC)
    # Fuck MSVC nobody likes you
    message(FATAL_ERROR "MSVC is not supported. Switch to Linux moron, microslop sheep!")
else()

    # Setting up compiler flags
    set(_EMEX64_WARN_FLAGS -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wformat-truncation)
    set(_EMEX64_OPT_FLAGS -fPIC)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND _EMEX64_OPT_FLAGS -g -O0)
    else()
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

        include(CheckIPOSupported)
        check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_output)
        if(_ipo_supported)
            set_target_properties(EmexToolchain PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

            if(EMEX64_BUILD_TOOLS)
                target_compile_options(emex64asm PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
                target_compile_options(emex64vm PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
                target_compile_options(emex64ld PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
            endif()

            if(EMEX64_BUILD_EXAMPLES)
                target_compile_options(emex64hello PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
            endif()
        else()
            message(STATUS "LTO not supported: ${_ipo_output}")
        endif()
    endif()

    target_compile_options(EmexToolchain PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})

    if(EMEX64_BUILD_TOOLS)
        target_compile_options(emex64asm PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
        target_compile_options(emex64vm PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
        target_compile_options(emex64ld PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
    endif()

    if(EMEX64_BUILD_EXAMPLES)
        target_compile_options(emex64hello PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
    endif()
endif()
