nw_project_prepare(CoreFeatures)

nwstd_add_library(Core SHARED ${CMAKE_CURRENT_SOURCE_DIR})

target_compile_definitions(Core PRIVATE -DBOOST_STACKTRACE_LINK)

if (WIN32)
    find_package(Boost REQUIRED COMPONENTS stacktrace_windbg)
else()
    find_package(Boost REQUIRED COMPONENTS stacktrace_basic)
endif()

if (UNIX)
    target_link_libraries(Core dl)
endif()

target_include_directories(Core PUBLIC ${Boost_INCLUDE_DIRS})
target_link_libraries(Core ${Boost_LIBRARIES})