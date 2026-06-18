cmake_minimum_required(VERSION 3.16)

project(EMEX64VM LANGUAGES C)

add_executable(emex64vm
    src/main.c
)

target_link_libraries(emex64vm
    PRIVATE emex64
)

target_compile_features(emex64vm PRIVATE c_std_23)

install(TARGETS emex64vm
    RUNTIME DESTINATION bin
)

option(EMEX64VM_DEVICE_DISPLAY "Enable display device support" ON)

target_compile_definitions(emex64vm PRIVATE
    EMEX64VM_DEVICE_DISPLAY=$<BOOL:${EMEX64VM_DEVICE_DISPLAY}>
)
