cmake_minimum_required(VERSION 3.16)

project(emexutils
    VERSION 1.0.0
    LANGUAGES C
)

add_library(emexutils STATIC
    src/bitwalker.c
    src/parser.c
    src/fdwalker.c
    src/object.c
)

target_include_directories(emexutils
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_include_directories(emexutils
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
        $<INSTALL_INTERFACE:include>
)

target_compile_features(emexutils PUBLIC c_std_23)

if (MSVC)
    target_compile_options(emexutils PRIVATE /W4)
else()
    target_compile_options(emexutils PRIVATE -Wall -Wextra -Wpedantic)
endif()
