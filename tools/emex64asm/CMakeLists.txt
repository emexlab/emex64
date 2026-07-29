cmake_minimum_required(VERSION 3.16)

project(emex64asm LANGUAGES C)

add_executable(emex64asm
    src/main.c
)

target_link_libraries(emex64asm
    PRIVATE EmexToolchain
)

target_compile_features(emex64asm PRIVATE c_std_23)

set_target_properties(emex64asm PROPERTIES
    C_EXTENSIONS ON
)

install(TARGETS emex64asm
    RUNTIME DESTINATION bin
    COMPONENT emex64asm
)

if(UNIX AND NOT APPLE)
set_target_properties(emex64asm PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib"
)
endif()
