cmake_minimum_required(VERSION 3.16)

project(emex64vm LANGUAGES C)

add_executable(emex64vm
    src/main.c
)

target_link_libraries(emex64vm
    PRIVATE EmexToolchain
)

target_compile_features(emex64vm PRIVATE c_std_23)

set_target_properties(emex64vm PROPERTIES
    C_EXTENSIONS ON
)

install(TARGETS emex64vm
    RUNTIME DESTINATION bin
    COMPONENT emex64vm
)

option(EMEX64VM_DEVICE_DISPLAY "Enable display device support" ON)

target_compile_definitions(emex64vm PRIVATE
    EMEX64VM_DEVICE_DISPLAY=$<BOOL:${EMEX64VM_DEVICE_DISPLAY}>
)
