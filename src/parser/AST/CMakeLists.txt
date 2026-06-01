add_library(AST ast.cpp)

target_link_libraries(AST ${YAML_CPP_LIBRARIES})

target_link_libraries(AST lexer)

target_include_directories(AST PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../lexer"
)

