cmake_minimum_required(VERSION 3.16)

project(EMEX64 LANGUAGES C)

add_subdirectory(emex64lib)
add_subdirectory(emex64vm)
add_subdirectory(emex64asm)

if(APPLE)
    set(CMAKE_MACOSX_RPATH TRUE)
    set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
    
    set_target_properties(emex64asm PROPERTIES INSTALL_RPATH "@loader_path/../lib")
    set_target_properties(emex64vm PROPERTIES INSTALL_RPATH "@loader_path/../lib")
endif()