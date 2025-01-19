if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
endif()

add_subdirectory(enviornment)

add_library(
    compiler
    compiler.cpp
    buildins.cpp
    initCSTD.cpp
)

target_link_libraries(
    compiler
    enviornment
)

