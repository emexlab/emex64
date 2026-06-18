cmake_minimum_required(VERSION 3.16)

project(EMEX64 LANGUAGES C)

add_subdirectory(evObj)
add_subdirectory(emex64lib)
add_subdirectory(emex64vm)
add_subdirectory(emex64asm)
add_subdirectory(emex64ld)
add_subdirectory(emex64hello)

option(EMEX64LIB_STATIC "Link statically against emex64lib" OFF)

if(NOT EMEX64LIB_STATIC)
    if(APPLE)
        set(CMAKE_MACOSX_RPATH TRUE)
        set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
    
        set_target_properties(emex64asm PROPERTIES INSTALL_RPATH "@loader_path/../lib")
        set_target_properties(emex64vm PROPERTIES INSTALL_RPATH "@loader_path/../lib")
        set_target_properties(emex64ld PROPERTIES INSTALL_RPATH "@loader_path/../lib")
        set_target_properties(emex64hello PROPERTIES INSTALL_RPATH "@loader_path/../lib")
    elseif(UNIX)
        set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

        set_target_properties(emex64asm PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
        set_target_properties(emex64vm PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
        set_target_properties(emex64ld PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
        set_target_properties(emex64hello PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
    endif()
endif()