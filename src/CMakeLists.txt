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
    "${CMAKE_CURRENT_SOURCE_DIR}/lexer"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser"
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

