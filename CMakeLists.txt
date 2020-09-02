cmake_minimum_required (VERSION 3.5)

option(EXAMPLES "Build examples and tiny_loopback" OFF)

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

else()

    idf_component_register(SRCS ${SOURCE_FILES}
                           INCLUDE_DIRS "src")

endif()
