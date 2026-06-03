add_subdirectory(lexer)
add_subdirectory(parser)
add_subdirectory(compiler)

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

# === target: gigly (Build System/Driver) ===
add_executable(gigly gigly.cpp compilation_state.cpp)
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

# === target: gigc (Single-File Compiler Frontend) ===
add_executable(gigc gigc.cpp compilation_state.cpp)
target_link_libraries(gigc ${llvm_libs})
target_link_libraries(gigc ${CLANG_LIBRARIES})
target_link_libraries(gigc lexer)
target_link_libraries(gigc parser)
target_link_libraries(gigc compiler)
target_include_directories(gigc PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/lexer"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser"
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)


