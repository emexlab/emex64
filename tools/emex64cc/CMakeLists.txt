cmake_minimum_required(VERSION 3.16)

project(emex64cc LANGUAGES C)

add_executable(emex64cc
    src/main.c
)

target_link_libraries(emex64cc
    PRIVATE EmexToolchain
)

target_compile_features(emex64cc PRIVATE c_std_23)

set_target_properties(emex64cc PROPERTIES
    C_EXTENSIONS ON
)

install(TARGETS emex64cc
    RUNTIME DESTINATION bin
    COMPONENT emex64cc
)

if(UNIX AND NOT APPLE)
set_target_properties(emex64cc PROPERTIES
  INSTALL_RPATH "$ORIGIN/../lib"
)
endif()
