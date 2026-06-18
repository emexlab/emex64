cmake_minimum_required(VERSION 3.16)

project(EMEX64 LANGUAGES C)

option(EMEX64_BUILD_TOOLS "Builds tools that make interfacing the emex64 toolchain easy" ON)
option(EMEX64_BUILD_EXAMPLES "Builds examples that showcase how to use the emex64 toolchain" OFF)

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

option(EMEX64LIB_STATIC "Link statically against emex64lib" OFF)

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