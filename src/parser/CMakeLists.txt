add_subdirectory(AST)

add_library(parser parser.cpp)

target_link_libraries(parser AST)
target_link_libraries(parser errors)

target_include_directories(parser PUBLIC
    "${PROJECT_SOURCE_DIR}/src/AST"
    "${PROJECT_SOURCE_DIR}/src/lexer"
    "${PROJECT_SOURCE_DIR}/src/errors"
)
