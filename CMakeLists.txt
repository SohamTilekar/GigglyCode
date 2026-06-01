cmake_minimum_required(VERSION 3.22)
project(GigglyCode LANGUAGES C CXX)

set(BUILD_ARCH "-m64")
set(TARGET_64 ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Place all binaries in the root of the build directory
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
endif()

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

find_package(Clang REQUIRED CONFIG)
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${CLANG_DEFINITIONS})

find_package(yaml-cpp REQUIRED CONFIG)
include_directories(${YAML_CPP_INCLUDE_DIR})
add_definitions(${YAML_CPP_LIBRARY_DIR})

add_subdirectory(src)
