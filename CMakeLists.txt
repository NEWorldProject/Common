cmake_minimum_required(VERSION 3.9)

nwstd_add_library(Core SHARED ${CMAKE_CURRENT_SOURCE_DIR})

if (UNIX)
    target_link_libraries(Core dl)
endif()
