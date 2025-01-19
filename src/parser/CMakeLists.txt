if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
endif()

add_subdirectory(AST)

add_library(parser parser.cpp macrointerpreter.cpp)

target_link_libraries(parser AST)
target_link_libraries(parser errors)

target_include_directories(parser PUBLIC
    "${PROJECT_SOURCE_DIR}/src/AST"
    "${PROJECT_SOURCE_DIR}/src/lexer"
    "${PROJECT_SOURCE_DIR}/src/errors"
)
