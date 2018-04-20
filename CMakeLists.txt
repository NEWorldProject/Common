cmake_minimum_required(VERSION 3.9)
project(Common)

set(CMAKE_MACOSX_RPATH ON)
set(CMAKE_CXX_STANDARD 17)
set(PROJECT_DIR ${CMAKE_SOURCE_DIR})

# Set Build Output Path
set(PRODUCT_DIR ${CMAKE_BINARY_DIR}/Product-$<CONFIG>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/SDK/Lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PRODUCT_DIR})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PRODUCT_DIR})

if(UNIX)
    link_libraries(dl stdc++fs)
endif()

find_package(Threads REQUIRED)

file(GLOB_RECURSE SRC_COMM Source/*.*)
add_library(Common SHARED ${SRC_COMM})

target_compile_definitions(Common
        PRIVATE -DNWCOREEXPORTS
        PRIVATE -DNWCompartmentLoggerPrefix="Core")

# Optional IPO. Do not use IPO if it's not supported by compiler.
cmake_policy(SET CMP0069 NEW)
include(CheckIPOSupported)
check_ipo_supported(RESULT result OUTPUT output)
if(result)
    set_property(TARGET Common PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
else()
    message(STATUS "IPO is not supported: ${output}")
endif()

target_include_directories(Common PUBLIC Source 3rdParty)
target_include_directories(Common SYSTEM PUBLIC 3rdParty)
target_link_libraries(Common Threads::Threads)
