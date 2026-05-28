cmake_minimum_required(VERSION 3.16)

project(EMEX64ASM LANGUAGES C)

add_executable(emex64asm
    src/main.c
    src/cmptok.c
    src/code.c
    src/emit.c
    src/compiler.c
    src/label.c
    src/opcode.c
    src/register.c
    src/section.c
    src/macro.c
    src/diag.c
)

target_include_directories(emex64asm
    PRIVATE
        ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(emex64asm
    PRIVATE emexutils
)

target_compile_features(emex64asm PRIVATE c_std_99)

install(TARGETS emex64asm
    RUNTIME DESTINATION bin
)