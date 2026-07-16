cmake_minimum_required(VERSION 3.16)

project(emex64ld LANGUAGES C)

add_executable(emex64ld
    src/main.c
)

target_compile_features(emex64ld PUBLIC c_std_23)

set_target_properties(emex64ld PROPERTIES
    C_EXTENSIONS ON
)

if(MSVC)
    target_compile_options(emex64ld PRIVATE /W4)
else()
    target_compile_options(emex64ld PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter)
endif()

target_link_libraries(emex64ld
    PRIVATE EmexToolchain
)

install(TARGETS emex64ld
    RUNTIME DESTINATION bin
    COMPONENT emex64ld
)
