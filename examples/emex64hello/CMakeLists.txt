cmake_minimum_required(VERSION 3.16)

project(EMEX64HELLO LANGUAGES C)

add_executable(emex64hello
    src/main.c
)

target_link_libraries(emex64hello
    PRIVATE emex64
)

target_compile_features(emex64hello PRIVATE c_std_23)

install(TARGETS emex64hello
    RUNTIME DESTINATION bin
)