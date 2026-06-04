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
    MC
    Target
    AsmPrinter
    # Link every target backend so --target <triple> works for cross-compilation.
    # LLVM_TARGETS_TO_BUILD is set by find_package(LLVM) to the list of backends
    # compiled into this LLVM installation (X86, AArch64, ARM, WebAssembly, ...).
    ${LLVM_TARGETS_TO_BUILD}
)

# === target: gigly (Build System/Driver) ===
add_executable(gigly gigly.cpp compilation_state.cpp)
target_link_libraries(gigly compiler parser lexer ${llvm_libs} ${CLANG_LIBRARIES})
target_include_directories(gigly PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/lexer"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser"
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

# === target: gigc (Single-File Compiler Frontend) ===
add_executable(gigc gigc.cpp compilation_state.cpp)
target_link_libraries(gigc compiler parser lexer ${llvm_libs} ${CLANG_LIBRARIES})
target_include_directories(gigc PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/lexer"
    "${CMAKE_CURRENT_SOURCE_DIR}/parser"
    "${CMAKE_CURRENT_SOURCE_DIR}/compiler"
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
    "${CMAKE_CURRENT_SOURCE_DIR}"
)


