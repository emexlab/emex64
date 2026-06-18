cmake_minimum_required(VERSION 3.16)

project(EMEX64LD LANGUAGES C)

add_executable(emex64ld
    src/main.c
)

target_compile_features(emex64ld PUBLIC c_std_11)

if(MSVC)
    target_compile_options(emex64ld PRIVATE /W4)
else()
    target_compile_options(emex64ld PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

target_link_libraries(emex64ld
    PRIVATE emex64
)

install(TARGETS emex64ld
    RUNTIME DESTINATION bin
)
