cmake_minimum_required(VERSION 3.16)

project(emex64asm LANGUAGES C)

add_executable(emex64asm
    src/main.c
)

target_link_libraries(emex64asm
    PRIVATE EmexToolchain
)

target_compile_features(emex64asm PRIVATE c_std_23)

install(TARGETS emex64asm
    RUNTIME DESTINATION bin
)