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
#include <queue>
#include <sstream>
#include <system_error>
#include <thread>
#include <vector>

// Include necessary headers
#include "compilation_state.hpp"
#include "compiler/compiler.hpp"
#include "errors/errors.hpp"
#include "include/cli11.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#define DEBUG_LEXER
#define DEBUG_PARSER

class Compiler {
  public:
    Compiler(const std::filesystem::path& srcDir, const std::filesystem::path& buildDir, const std::string& optimizationLevel, bool verbose, const std::string& target_triple = "")
        : srcDir(srcDir), buildDir(buildDir), optimizationLevel(optimizationLevel), verbose(verbose), target_triple(target_triple), irDir(buildDir / "ir"), objDir(buildDir / "obj") {
        Utils::createDirectories(irDir);
        Utils::createDirectories(objDir);

        if (verbose) {
            std::cout << "Compiler initialized with:\n"
                      << " Source Directory: " << srcDir << "\n"
                      << " Build Directory: " << buildDir << "\n"
                      << " Optimization Level: " << (optimizationLevel.empty() ? "None" : optimizationLevel) << "\n";
        }
    }

    /**
     * @brief Compiles all supported files in the source directory.
     *
     * @param rootFolder The root folder to track compiled files.
     */
    void compileAll(compilationState::RecordFolder* rootFolder) {
        // Collect all relevant files
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
            if (entry.is_regular_file() && isSupportedFile(entry.path())) { files.emplace_back(entry.path()); }
        }

        if (verbose) { std::cout << "Found " << files.size() << " file(s) to compile." << std::endl; }

        // Spawning one thread per file to avoid thread starvation deadlocks when waiting on imports
        if (verbose) { std::cout << "Starting compilation with " << files.size() << " thread(s)..." << std::endl; }

        std::vector<std::thread> workers;
        for (const auto& file : files) {
            workers.emplace_back([this, file, rootFolder]() { compileFile(file, rootFolder); });
        }

        for (auto& worker : workers) {
            if (worker.joinable()) { worker.join(); }
        }

        if (verbose) { std::cout << "Compilation phase completed." << std::endl; }
    }

    /**
     * @brief Links all object files into the final executable.
     *
     * @param executablePath The path to the output executable.
     * @return int Exit status code.
     * @throws CompilationException If linking fails.
     */
    int linkAll(const std::filesystem::path& executablePath) const {
        std::string objFiles;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(objDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".o") { objFiles += "\"" + entry.path().string() + "\" "; }
        }

        if (objFiles.empty()) {
            std::cerr << "Error: No object files found to link." << std::endl;
            return 1;
        }

        std::string linkCommand = "clang++ " + objFiles + "-o \"" + executablePath.string() + "\"";
        if (verbose) {
            std::cout << "Linking object files into executable..." << std::endl;
            std::cout << "Link Command: " << linkCommand << std::endl;
        }

        int exit_code;
        std::string linkOutput = runCommand(linkCommand, exit_code);
        if (exit_code != 0) { errors::raiseCompilationError("Failed to link object files into executable " + executablePath.string() + "\nCommand: " + linkCommand + "\nOutput: " + linkOutput); }

        if (verbose) { std::cout << "Successfully linked object files into executable: " << executablePath << std::endl; }
        return 0;
    }

  private:
    std::filesystem::path srcDir;
    std::filesystem::path buildDir;
    std::string optimizationLevel;
    bool verbose;
    std::string target_triple; // empty = native

    std::filesystem::path irDir;
    std::filesystem::path objDir;

    /**
     * @brief Checks if a file has a supported extension.
     *
     * @param path The path to the file.
     * @return true If the file is supported.
     * @return false Otherwise.
     */
    bool isSupportedFile(const std::filesystem::path& path) const {
        static const std::vector<std::filesystem::path> supportedExtensions = {".gc", ".c", ".rs"};
        return std::find(supportedExtensions.begin(), supportedExtensions.end(), path.extension()) != supportedExtensions.end();
    }

    /**
     * @brief Compiles an individual file depending on its extension.
     *
     * @param filePath The path to the source file.
     * @param rootFolder The root folder to track compiled files.
     */
    void compileFile(const std::filesystem::path& filePath, compilationState::RecordFolder* rootFolder) {
        auto relative = std::filesystem::relative(filePath, srcDir);
        std::filesystem::path outputIRPath = irDir / (relative.string() + ".ll");
        std::filesystem::path objFilePath = objDir / (relative.string() + ".o");

        if (verbose) {
            std::cout << "Compiling file: " << filePath << "\n"
                      << " Output IR Path: " << outputIRPath << "\n"
                      << " Object File Path: " << objFilePath << "\n";
        }

        Utils::createDirectories(outputIRPath.parent_path());

        std::string fileContent = Utils::readFileToString(filePath);

        // Check if the file needs recompilation
        compilationState::RecordFile* fileRecord = findOrCreateFileRecord(rootFolder, relative);
        // Compile based on file extension
        std::string extension = filePath.extension().string();
        if (extension == ".gc") {
            compileGcFile(fileContent, filePath, outputIRPath, objFilePath, fileRecord);
        } else if (extension == ".c") {
            compileCFile(filePath, outputIRPath, objFilePath, fileRecord);
        } else if (extension == ".rs") {
            compileRustFile(filePath, outputIRPath, objFilePath, fileRecord);
        } else {
            errors::raiseCompilationError("Unsupported file type: " + filePath.string());
        }
    }

    /**
     * @brief Compiles a .gc source file.
     *
     * @param filePath The path to the .gc file.
     * @param outputIRPath The path to the output LLVM IR file.
     * @param objFilePath The path to the output object file.
     * @param fileRecord The record of the file being compiled.
     */
    void compileGcFile(
        std::string fileContent, const std::filesystem::path& filePath, const std::filesystem::path& outputIRPath, const std::filesystem::path& objFilePath, compilationState::RecordFile* fileRecord) {
// Debugging Lexer
#ifdef DEBUG_LEXER
        if (filePath.filename() == "main.gc") { debugLexer(Utils::readFileToString(filePath), filePath); }
#endif

// Debugging Parser
#ifdef DEBUG_PARSER
        if (filePath.filename() == "main.gc") { debugParser(Utils::readFileToString(filePath), filePath); }
#endif

        Lexer lexer(fileContent, filePath);
        parser::Parser parser(&lexer);
        auto program = parser.parseProgram();

        compiler::Compiler comp(fileContent, std::filesystem::absolute(filePath), fileRecord, buildDir, std::filesystem::relative(filePath, srcDir).string(), target_triple);
        comp.compile(program);
        delete program;

        // Write LLVM IR to file
        std::error_code EC;
        llvm::raw_fd_ostream irFile(outputIRPath.string(), EC, llvm::sys::fs::OF_None);
        if (EC) { errors::raiseCompilationError("Could not open IR file " + outputIRPath.string() + ": " + EC.message()); }
        comp.llvm_module->print(irFile, nullptr);
        irFile.close();

        // Compile IR to object file
        Utils::createDirectories(objFilePath.parent_path());
        std::string optFlag = optimizationLevel.empty() ? "" : " -O" + optimizationLevel;
        std::string clangCommand = "clang -c \"" + outputIRPath.string() + "\" -o \"" + objFilePath.string() + "\" -Woverride-module" + optFlag;

        int clangResult;
        std::string clangOutput = runCommand(clangCommand, clangResult);
        if (clangResult != 0) {
            errors::raiseCompilationError("Failed to convert " + outputIRPath.string() + " to " + objFilePath.string() + "\nCommand: " + clangCommand + "\nOutput: " + clangOutput);
        }

        // Update file record
        fileRecord->compiled = true;

        if (verbose) { std::cout << "Compiled .gc file: " << filePath << std::endl; }
    }

    /**
     * @brief Compiles a .c source file.
     *
     * @param filePath The path to the .c file.
     * @param outputIRPath The path to the output LLVM IR file.
     * @param objFilePath The path to the output object file.
     * @param fileRecord The record of the file being compiled.
     */
    void compileCFile(const std::filesystem::path& filePath, const std::filesystem::path& outputIRPath, const std::filesystem::path& objFilePath, compilationState::RecordFile* fileRecord) {
        // Compile to LLVM IR
        std::string optFlag = optimizationLevel.empty() ? "" : " -O" + optimizationLevel;
        std::string clangCommandIR = "clang -emit-llvm -S " + optFlag + " \"" + filePath.string() + "\" -o \"" + outputIRPath.string() + "\"";

        int clangResultIR;
        std::string clangOutputIR = runCommand(clangCommandIR, clangResultIR);
        if (clangResultIR != 0) { errors::raiseCompilationError("Failed to compile " + filePath.string() + " to LLVM IR" + "\nCommand: " + clangCommandIR + "\nOutput: " + clangOutputIR); }

        // Compile to object file
        std::string clangCommandObj = "clang -c \"" + filePath.string() + "\" -o \"" + objFilePath.string() + "\" " + optFlag;

        int clangResultObj;
        std::string clangOutputObj = runCommand(clangCommandObj, clangResultObj);
        if (clangResultObj != 0) { errors::raiseCompilationError("Failed to compile " + filePath.string() + " to object file" + "\nCommand: " + clangCommandObj + "\nOutput: " + clangOutputObj); }

        // Update file record
        fileRecord->compiled = true;

        if (verbose) { std::cout << "Compiled .c file: " << filePath << std::endl; }
    }

    /**
     * @brief Compiles a .rs source file.
     *
     * @param filePath The path to the .rs file.
     * @param outputIRPath The path to the output LLVM IR file.
     * @param objFilePath The path to the output object file.
     * @param fileRecord The record of the file being compiled.
     */
    void compileRustFile(const std::filesystem::path& filePath, const std::filesystem::path& outputIRPath, const std::filesystem::path& objFilePath, compilationState::RecordFile* fileRecord) {
        std::filesystem::path irFilePath = outputIRPath.string() + ".ll";
        Utils::createDirectories(irFilePath.parent_path());

        // Compile Rust to LLVM IR
        std::string rustcCommand = "rustc -emit=llvm-ir \"" + filePath.string() + "\" -o \"" + irFilePath.string() + "\"" + (optimizationLevel.empty() ? "" : " -C opt-level=" + optimizationLevel);

        int rustcResult;
        std::string rustcOutput = runCommand(rustcCommand, rustcResult);
        if (rustcResult != 0) { errors::raiseCompilationError("Failed to compile Rust file " + filePath.string() + " to LLVM IR" + "\nCommand: " + rustcCommand + "\nOutput: " + rustcOutput); }

        // Compile LLVM IR to object file
        std::string clangCommand = "clang -c \"" + irFilePath.string() + "\" -o \"" + objFilePath.string() + "\"" + (optimizationLevel.empty() ? "" : " -O" + optimizationLevel);

        int clangResult;
        std::string clangOutput = runCommand(clangCommand, clangResult);
        if (clangResult != 0) { errors::raiseCompilationError("Failed to convert " + irFilePath.string() + " to " + objFilePath.string() + "\nCommand: " + clangCommand + "\nOutput: " + clangOutput); }

        // Update file record
        fileRecord->compiled = true;

        if (verbose) { std::cout << "Compiled Rust file: " << filePath << std::endl; }
    }

    /**
     * @brief Handles lexer debugging.
     *
     * @param fileContent The content of the source file.
     */
    void debugLexer(const std::string& fileContent, const std::filesystem::path& file_path) const {
#ifdef DEBUG_LEXER
        std::cout << "=========== Lexer Debug ===========" << std::endl;
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
        std::cout << "Lexer debug output written to " << outputPath.string() << std::endl;
#endif
    }

    /**
     * @brief Handles parser debugging.
     *
     * @param fileContent The content of the source file.
     */
    void debugParser(const std::string& fileContent, const std::filesystem::path& file_path) const {
#ifdef DEBUG_PARSER
        Lexer debug_lexer(fileContent, file_path);
        parser::Parser debugParser(&debug_lexer);
        auto program = debugParser.parseProgram();
        std::cout << "=========== Parser Debug ===========" << std::endl;
        std::filesystem::path outputPath = buildDir / "parser_output.yaml";
        std::ofstream file(outputPath, std::ios::trunc);
        if (file) {
            file << program->toStr() << std::endl;
            std::cout << "Parser debug output written to " << outputPath.string() << std::endl;
        } else {
            errors::raiseCompilationError("Unable to open parser debug output file: " + outputPath.string());
        }
        delete program;
#endif
    }
};

// =======================================
// CLI Setup Function
// =======================================
void setupCLI(CLI::App& app, std::filesystem::path& inputFolderPath, std::string& optimizationLevel, std::filesystem::path& executablePath, bool& verbose, std::string& target_triple) {
    app.add_option("input_folder", inputFolderPath, "Input folder path")->required()->check(CLI::ExistingDirectory);
    app.add_option("-O,--optimization", optimizationLevel, "Optimization level (O1, O2, O3, Os, Ofast)")->default_val("");
    app.add_option("-o,--output", executablePath, "Output executable path")->required();
    app.add_flag("-v,--verbose", verbose, "Enable verbose output");
    app.add_option("--target", target_triple, "Override target triple for cross-compilation (e.g. aarch64-unknown-linux-gnu). Default: host native.")->default_val("");
}

// =======================================
// Main Function
// =======================================
int main(int argc, char* argv[]) {
    // Initialize CLI
    CLI::App app{"Folder Compiler"};
    std::filesystem::path inputFolderPath;
    std::string optimizationLevel;
    std::filesystem::path executablePath;
    bool verbose = false;
    std::string target_triple;
    setupCLI(app, inputFolderPath, optimizationLevel, executablePath, verbose, target_triple);
    CLI11_PARSE(app, argc, argv);

    if (verbose) {
        std::cout << "Verbose mode enabled." << std::endl;
        if (!target_triple.empty()) { std::cout << "Target triple override: " << target_triple << std::endl; }
    }

    // Define Source and Build Directories
    std::filesystem::path srcDir = inputFolderPath / "src";
    std::filesystem::path buildDir = inputFolderPath / "build";

    if (!std::filesystem::exists(srcDir)) {
        std::cerr << "Error: Source directory " << srcDir << " does not exist." << std::endl;
        return 1;
    }

    // Initialize Compiler with verbose flag and optional target triple
    Compiler compiler(srcDir, buildDir, optimizationLevel, verbose, target_triple);

    // Initialize rootFolder directly
    compilationState::RecordFolder rootFolder;

    // Compile All Files
    compiler.compileAll(&rootFolder);

    // Link Object Files into Executable
    return compiler.linkAll(executablePath);
    return 0;
}
