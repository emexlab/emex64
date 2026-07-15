find_package(Git QUIET)

if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD
        OUTPUT_VARIABLE EMEX64_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --tags --exact-match
        OUTPUT_VARIABLE EMEX64_VERSION_TAG
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE EMEX64_TAG_RESULT
        ERROR_QUIET)
    if(EMEX64_TAG_RESULT EQUAL 0)
        set(EMEX64_COMMITS_SINCE_TAG 0)
    else()
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --abbrev=0
            OUTPUT_VARIABLE EMEX64_VERSION_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)
        if(EMEX64_VERSION_TAG)
            execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list --count ${EMEX64_VERSION_TAG}..HEAD
                OUTPUT_VARIABLE EMEX64_COMMITS_SINCE_TAG
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)
        else()
            set(EMEX64_VERSION_TAG "untagged")
            execute_process(
                COMMAND ${GIT_EXECUTABLE} rev-list --count HEAD
                OUTPUT_VARIABLE EMEX64_COMMITS_SINCE_TAG
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)
        endif()
    endif()
endif()

if(NOT EMEX64_GIT_HASH)
    set(EMEX64_GIT_HASH "unknown")
endif()

if(NOT EMEX64_VERSION_TAG)
    set(EMEX64_VERSION_TAG "untagged")
endif()

if(EMEX64_COMMITS_SINCE_TAG EQUAL 0)
    set(EMEX64_VERSION_FULL
        "EmexToolchain-${EMEX64_VERSION}-${EMEX64_VERSION_TAG}")
else()
    set(EMEX64_VERSION_FULL
        "EmexToolchain-${EMEX64_VERSION}-${EMEX64_VERSION_TAG}+${EMEX64_COMMITS_SINCE_TAG}.g${EMEX64_GIT_HASH}")
endif()

configure_file(${SRC} ${DST} @ONLY)
