add_library(AST ast.cpp)

target_link_libraries(AST ${YAML_CPP_LIBRARIES})

target_link_libraries(AST lexer)

target_include_directories(AST PUBLIC
    "${PROJECT_SOURCE_DIR}/src/include"
    "${PROJECT_SOURCE_DIR}/src/lexer"
)
