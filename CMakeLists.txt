cmake_minimum_required(VERSION 3.16)

project(emex64Toolchain LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(EMEX64_BUILD_TOOLS "Builds tools that make interfacing the emex64 toolchain easy" ON)
option(EMEX64_BUILD_EXAMPLES "Builds examples that showcase how to use the emex64 toolchain" OFF)
option(EMEX64LIB_STATIC "Build emex64lib as a static library" OFF)

add_subdirectory(evObj)
add_subdirectory(emex64lib)

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