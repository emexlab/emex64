cmake_minimum_required(VERSION 3.16)

project(emex64Toolchain LANGUAGES C)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

option(EMEX64_BUILD_TOOLS "Builds tools that make interfacing the emex64 toolchain easy" ON)
option(EMEX64_BUILD_EXAMPLES "Builds examples that showcase how to use the emex64 toolchain" OFF)
option(EMEX64LIB_STATIC "Build EmexToolchain as a static library" OFF)

add_subdirectory(EmexFoundation)
add_subdirectory(EmexToolchain)

if(EMEX64_BUILD_TOOLS)
    add_subdirectory(tools/emex64vm)
    add_subdirectory(tools/emex64asm)
    add_subdirectory(tools/emex64ld)
endif()

if(EMEX64_BUILD_EXAMPLES)
    add_subdirectory(examples/emex64hello)
endif()

if(NOT EMEX64LIB_STATIC)
    if(APPLE)
        set(CMAKE_MACOSX_RPATH TRUE)
        set(CMAKE_INSTALL_RPATH "@loader_path/../lib")

        if(EMEX64_BUILD_TOOLS)
            set_target_properties(emex64asm PROPERTIES INSTALL_RPATH "@loader_path/../lib")
            set_target_properties(emex64vm PROPERTIES INSTALL_RPATH "@loader_path/../lib")
            set_target_properties(emex64ld PROPERTIES INSTALL_RPATH "@loader_path/../lib")
        endif()

        if(EMEX64_BUILD_EXAMPLES)
            set_target_properties(emex64hello PROPERTIES INSTALL_RPATH "@loader_path/../lib")
        endif()
    elseif(UNIX)
        set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")

        if(EMEX64_BUILD_TOOLS)
            set_target_properties(emex64asm PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
            set_target_properties(emex64vm PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
            set_target_properties(emex64ld PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
        endif()

        if(EMEX64_BUILD_EXAMPLES)
            set_target_properties(emex64hello PROPERTIES INSTALL_RPATH "$ORIGIN/../lib")
        endif()
    endif()
endif()

if(MSVC)
    # Fuck MSVC nobody likes you
    message(FATAL_ERROR "MSVC is not supported. Switch to Linux moron, microslop sheep!")
else()

    # Setting up compiler flags
    set(_EMEX64_WARN_FLAGS -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wformat-truncation -Wno-gnu-conditional-omitted-operand)
    set(_EMEX64_OPT_FLAGS -fPIC)

    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        list(APPEND _EMEX64_OPT_FLAGS -g -O0)
    else()
        if(EMEX64LIB_COMPILER_OPTIMIZATIONS)
            list(APPEND _EMEX64_OPT_FLAGS -O3 -fomit-frame-pointer -fstrict-aliasing -ffast-math -funroll-loops)
        endif()

        if(EMEX64LIB_NATIVE_ARCH)
            list(APPEND _EMEX64_OPT_FLAGS -march=native -mtune=native)
        endif()

        if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
            list(APPEND _EMEX64_OPT_FLAGS -fgraphite-identity -floop-nest-optimize -fipa-pta)
        elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
            list(APPEND _EMEX64_OPT_FLAGS -fvectorize -fslp-vectorize)
        endif()

        include(CheckIPOSupported)
        check_ipo_supported(RESULT _ipo_supported OUTPUT _ipo_output)
        if(_ipo_supported)
            set_target_properties(EmexToolchain PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)

            if(EMEX64_BUILD_TOOLS)
                target_compile_options(emex64asm PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
                target_compile_options(emex64vm PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
                target_compile_options(emex64ld PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
            endif()

            if(EMEX64_BUILD_EXAMPLES)
                target_compile_options(emex64hello PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
            endif()
        else()
            message(STATUS "LTO not supported: ${_ipo_output}")
        endif()
    endif()

    target_compile_options(EmexToolchain PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})

    if(EMEX64_BUILD_TOOLS)
        target_compile_options(emex64asm PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
        target_compile_options(emex64vm PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
        target_compile_options(emex64ld PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
    endif()

    if(EMEX64_BUILD_EXAMPLES)
        target_compile_options(emex64hello PRIVATE ${_EMEX64_WARN_FLAGS} ${_EMEX64_OPT_FLAGS})
    endif()
endif()

set(CPACK_PACKAGE_NAME "EmexToolchain")
set(CPACK_PACKAGE_VENDOR "emexLabs")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "EmexToolchain compilation suite")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE.txt")

if(APPLE)
    set(CPACK_GENERATOR "productbuild")

    set(CPACK_SET_DESTDIR ON)
    set(CPACK_PACKAGING_INSTALL_PREFIX "/")

    set(CPACK_PRODUCTBUILD_DOMAINS ON)

    set(CPACK_COMPONENTS_ALL EmexToolchain emex64asm emex64ld emex64vm Headers)

    set(CPACK_COMPONENT_EMEXTOOLCHAIN_DESCRIPTION "Installs EmexToolchain library.")
    set(CPACK_COMPONENT_EMEXTOOLCHAIN_REQUIRED TRUE)

    set(CPACK_COMPONENT_EMEX64ASM_DESCRIPTION "Installs emex64 assembler.")
    set(CPACK_COMPONENT_EMEX64LD_DESCRIPTION "Installs emex64 linker.")
    set(CPACK_COMPONENT_EMEX64VM_DESCRIPTION "Installs emex64 virtual machine.")
    set(CPACK_COMPONENT_HEADERS_DESCRIPTION "Installs headers which can be used to develop tooling for emex64 development.")

    set(CPACK_PRODUCTBUILD_IDENTIFIER "org.emexlabs.emextoolchain")
elseif(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
else()
    set(CPACK_GENERATOR "DEB;RPM;TGZ")
    set(CPACK_DEBIAN_PACKAGE_MAINTAINER "lucavmu@emexlab.de")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.31)")
endif()

include(CPack)
