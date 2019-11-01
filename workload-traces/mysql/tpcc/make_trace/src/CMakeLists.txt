cmake_minimum_required (VERSION 3.7)

project(make_trace)

# Set build type to Release if not set
if(NOT DEFINED CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
    message(STATUS "No build type set, defaulting to Release")
endif()

# Add variable to cache
set(CMAKE_BUILD_TYPE "${CMAKE_BUILD_TYPE}"
    CACHE STRING
    "Choose the type of build. Options are: <empty>, Debug, Release, RelWithDebInfo, MinSizeRel.")
message(STATUS "Using build type: ${CMAKE_BUILD_TYPE}")

include_directories("${PROJECT_SOURCE_DIR}")

# Setup compilation options
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_C_STANDARD 11)

set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

# Find external dependencies
include_directories("${CMAKE_CURRENT_LIST_DIR}/../../../../../3rdparty/rapidjson/include")

find_package(Boost REQUIRED
    COMPONENTS program_options)

find_package(OpenMP)
if(OPENMP_FOUND)
    set (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${OpenMP_C_FLAGS}")
    set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${OpenMP_CXX_FLAGS}")
endif()

# Add main compilation target
add_executable(make_trace
    main.cpp
    )
target_link_libraries(make_trace
    ${Boost_LIBRARIES}
    )
