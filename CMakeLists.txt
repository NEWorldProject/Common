cmake_minimum_required(VERSION 3.9)

nw_module(CoreFeatures 0.0.1 INFINIDEAS)

nwstd_add_library(Core SHARED ${CMAKE_CURRENT_SOURCE_DIR})
nw_module_target(CoreFeatures Core)

if (UNIX)
    target_link_libraries(Core dl)
endif()
