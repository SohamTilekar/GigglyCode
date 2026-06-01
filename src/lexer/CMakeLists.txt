add_subdirectory(
    "${CMAKE_CURRENT_SOURCE_DIR}/../errors"
    "${CMAKE_CURRENT_BINARY_DIR}/errors"
)

add_library(lexer lexer.cpp token.cpp)

target_link_libraries(lexer errors)

target_include_directories(lexer PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../errors"
)

