add_subdirectory(AST)

add_library(parser parser.cpp macrointerpreter.cpp)

target_link_libraries(parser AST)
target_link_libraries(parser errors)

target_include_directories(parser PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/AST"
    "${CMAKE_CURRENT_SOURCE_DIR}/../lexer"
    "${CMAKE_CURRENT_SOURCE_DIR}/../errors"
)

