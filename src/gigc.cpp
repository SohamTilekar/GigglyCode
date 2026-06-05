#include <array>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <memory>
#include <mutex>
#include <sstream>
#include <system_error>
#include <vector>

// Include project headers
#include "compilation_state.hpp"
#include "compiler/compiler.hpp"
#include "errors/errors.hpp"
#include "include/cli11.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#define DEBUG_LEXER
#define DEBUG_PARSER

// Helper to debug Lexer
void debugLexer(const std::string& fileContent, const std::filesystem::path& file_path, const std::filesystem::path& buildDir) {
#ifdef DEBUG_LEXER
    Lexer debugLexer(fileContent, file_path);
    std::filesystem::path outputPath = buildDir / "lexer_output.log";
    std::ofstream debugOutput(outputPath, std::ios::trunc);
    if (!debugOutput) {
        std::cerr << "Error: Could not open debug output file " << outputPath.string() << std::endl;
        return;
    }
    while (debugLexer.current_char != "") {
        token::Token token = debugLexer.nextToken();
        debugOutput << token.toString(false) << std::endl;
    }
#endif
}

// Helper to debug Parser
void debugParser(const std::string& fileContent, const std::filesystem::path& file_path, const std::filesystem::path& buildDir) {
#ifdef DEBUG_PARSER
    Lexer debug_lexer(fileContent, file_path);
    parser::Parser debugParser(&debug_lexer);
    auto program = debugParser.parseProgram();
    std::filesystem::path outputPath = buildDir / "parser_output.yaml";
    std::ofstream file(outputPath, std::ios::trunc);
    if (file) {
        file << program->toStr() << std::endl;
    } else {
        errors::raiseCompilationError("Unable to open parser debug output file: " + outputPath.string());
    }
    delete program;
#endif
}

// Recursive compilation function
void compileSingleFile(const std::filesystem::path& filePath,
                       const std::filesystem::path& srcDir,
                       const std::filesystem::path& buildDir,
                       const std::string& optimizationLevel,
                       bool verbose,
                       compilationState::RecordFolder* rootFolder,
                       const std::filesystem::path& customOutput = "",
                       bool emitLLVMOnly = false,
                       const std::string& target_triple = "") {
    auto relative = std::filesystem::relative(filePath, srcDir);
    std::filesystem::path outputIRPath = buildDir / "ir" / (relative.string() + ".ll");
    std::filesystem::path objFilePath = buildDir / "obj" / (relative.string() + ".o");

    if (!customOutput.empty()) {
        if (emitLLVMOnly || customOutput.extension() == ".ll") {
            outputIRPath = customOutput;
        } else {
            objFilePath = customOutput;
        }
    }

    if (verbose) {
        std::cout << "Compiling: " << filePath << "\n"
                  << "  IR output: " << outputIRPath << "\n"
                  << "  OBJ output: " << objFilePath << "\n";
    }

    // Ensure output directories exist
    if (!outputIRPath.parent_path().empty()) { Utils::createDirectories(outputIRPath.parent_path()); }
    if (!objFilePath.parent_path().empty()) { Utils::createDirectories(objFilePath.parent_path()); }

    // Fetch compilation record
    compilationState::RecordFile* fileRecord = findOrCreateFileRecord(rootFolder, relative);

    std::string fileContent = Utils::readFileToString(filePath);

    // Debugging hooks
#ifdef DEBUG_LEXER
    if (filePath.filename() == "main.gc") { debugLexer(fileContent, filePath, buildDir); }
#endif
#ifdef DEBUG_PARSER
    if (filePath.filename() == "main.gc") { debugParser(fileContent, filePath, buildDir); }
#endif

    // Parse and compile to LLVM IR
    Lexer lexer(fileContent, filePath);
    parser::Parser parser(&lexer);
    auto program = parser.parseProgram();

    compiler::Compiler comp(fileContent, std::filesystem::absolute(filePath), fileRecord, buildDir, relative.string(), target_triple);

    // Set up the synchronous dependency compiler callback
    comp.compile_dependency_cb = [&](const std::filesystem::path& depPath) { compileSingleFile(depPath, srcDir, buildDir, optimizationLevel, verbose, rootFolder, "", emitLLVMOnly, target_triple); };

    comp.compile(program);
    delete program;

    // Write LLVM IR to file
    std::error_code EC;
    llvm::raw_fd_ostream irFile(outputIRPath.string(), EC, llvm::sys::fs::OF_None);
    if (EC) { errors::raiseCompilationError("Could not open IR file " + outputIRPath.string() + ": " + EC.message()); }
    comp.llvm_module->print(irFile, nullptr);
    irFile.close();

    // Mark as compiled
    fileRecord->compiled = true;

    // Unless we are emitting LLVM IR only, compile to object file
    if (!emitLLVMOnly) {
        std::string optFlag = optimizationLevel.empty() ? "" : " -O" + optimizationLevel;
        std::string clangCommand = "clang -c \"" + outputIRPath.string() + "\" -o \"" + objFilePath.string() + "\" -Woverride-module" + optFlag;

        int clangResult;
        std::string clangOutput = runCommand(clangCommand, clangResult);
        if (clangResult != 0) {
            errors::raiseCompilationError("Failed to convert " + outputIRPath.string() + " to " + objFilePath.string() + "\nCommand: " + clangCommand + "\nOutput: " + clangOutput);
        }
    }

    if (verbose) { std::cout << "Successfully compiled: " << filePath << std::endl; }
}

int main(int argc, char* argv[]) {
    CLI::App app{"GigglyCode Single-File Compiler Frontend"};

    std::filesystem::path inputFile;
    std::filesystem::path outputFile;
    std::string optimizationLevel = "";
    std::string target_triple = "";
    bool emitLLVM = false;
    bool verbose = false;

    app.add_option("input_file", inputFile, "Input source file (.gc)")->required()->check(CLI::ExistingFile);
    app.add_option("-o,--output", outputFile, "Output file path");
    app.add_option("-O,--optimization", optimizationLevel, "Optimization level (O1, O2, O3, Os, Ofast)")->default_val("");
    app.add_flag("-S,--emit-llvm", emitLLVM, "Emit LLVM IR instead of object file");
    app.add_flag("-v,--verbose", verbose, "Enable verbose output");
    app.add_option("--target", target_triple, "Override target triple for cross-compilation (e.g. aarch64-unknown-linux-gnu). Default: host native.")->default_val("");

    CLI11_PARSE(app, argc, argv);

    // Resolve source directory and build directory relative to input file
    std::filesystem::path srcDir = inputFile.parent_path();
    std::filesystem::path buildDir = srcDir / "build";
    Utils::createDirectories(buildDir);

    // If output file is empty, default it
    if (outputFile.empty()) {
        outputFile = inputFile.stem();
        if (emitLLVM) {
            outputFile += ".ll";
        } else {
            outputFile += ".o";
        }
    }

    compilationState::RecordFolder rootFolder;

    try {
        compileSingleFile(inputFile, srcDir, buildDir, optimizationLevel, verbose, &rootFolder, outputFile, emitLLVM, target_triple);
    } catch (const std::exception& e) {
        std::cerr << "Compilation failed: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
