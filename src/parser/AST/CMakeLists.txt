add_library(AST ast.cpp)

target_include_directories(AST PUBLIC
    "${PROJECT_SOURCE_DIR}/src/include"
    "${PROJECT_SOURCE_DIR}/src/lexer"
)