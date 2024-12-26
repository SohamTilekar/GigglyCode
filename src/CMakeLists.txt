cmake_minimum_required(VERSION 3.22)

project(GigglyCode LANGUAGES CXX)

set(BUILD_ARCH "-m64")
set(TARGET_64 ON)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(LLVM REQUIRED CONFIG)
include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

find_package(Clang REQUIRED CONFIG)
include_directories(${CLANG_INCLUDE_DIRS})
add_definitions(${CLANG_DEFINITIONS})

find_package(yaml-cpp REQUIRED CONFIG)
include_directories(${YAML_CPP_INCLUDE_DIR})
add_definitions(${YAML_CPP_LIBRARY_DIR})

add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(compiler)

add_executable(gigly main.cpp)

llvm_map_components_to_libnames(llvm_libs
    Analysis
    Core
    ExecutionEngine
    InstCombine
    Object
    OrcJIT
    RuntimeDyld
    ScalarOpts
    Support
    native
)

target_link_libraries(gigly ${llvm_libs})
target_link_libraries(gigly ${CLANG_LIBRARIES})

target_link_libraries(gigly lexer)
target_link_libraries(gigly parser)
target_link_libraries(gigly compiler)

target_include_directories(gigly PUBLIC
    "${PROJECT_SOURCE_DIR}/src/lexer"
    "${PROJECT_SOURCE_DIR}/src/parser"
    "${PROJECT_SOURCE_DIR}/src/compiler"
)
