cmake_minimum_required(VERSION 3.16)

project(emex64hello LANGUAGES C)

add_executable(emex64hello
    src/main.c
)

target_link_libraries(emex64hello
    PRIVATE EmexToolchain
)

target_compile_features(emex64hello PRIVATE c_std_23)

install(TARGETS emex64hello
    RUNTIME DESTINATION bin
    COMPONENT Tools
)
