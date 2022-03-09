cmake_minimum_required (VERSION 3.5)

option(EXAMPLES "Build examples and tiny_loopback" OFF)
option(UNITTEST "Build unit tests" OFF)
option(CUSTOM "Do not use built-in HAL, but use Custom instead" OFF)
option(ENABLE_FD_LOGS "Enable full duplex protocol logs" OFF)
# set(LOG_LEVEL "0" CACHE STRING "Logging level option" FORCE)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp src/*.c)
file(GLOB_RECURSE HEADER_FILES src/*.h)

if (NOT DEFINED COMPONENT_DIR)

    project (tinyproto)

    include_directories(src)

    add_library(tinyproto STATIC ${HEADER_FILES} ${SOURCE_FILES})

    if (EXAMPLES)
        add_subdirectory(examples/linux/loopback)
        add_subdirectory(examples/linux/hdlc_demo)
        add_subdirectory(examples/linux/hdlc_demo_multithread)
    endif()

    if (UNITTEST)
        add_subdirectory(unittest)
    endif()

else()

    idf_component_register(SRCS ${SOURCE_FILES}
                           INCLUDE_DIRS "src")

endif()

if (CUSTOM)
    add_definitions("-DTINY_CUSTOM_PLATFORM=1")
endif()
if (ENABLE_FD_LOGS)
    add_definitions("-DTINY_DEBUG=1")
    add_definitions("-DTINY_FD_DEBUG=1")
    if (LOG_LEVEL)
       add_definitions("-DTINY_LOG_LEVEL_DEFAULT=${LOG_LEVEL}")
    endif()
endif()