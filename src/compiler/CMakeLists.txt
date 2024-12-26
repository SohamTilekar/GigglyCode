add_subdirectory(enviornment)

add_library(
    compiler
    compiler.cpp
    initCSTD.cpp
)

target_link_libraries(
    compiler
    enviornment
)
