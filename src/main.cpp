#include <array>
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
#include <stdexcept>
#include <system_error>
#include <thread>
#include <vector>

// Include necessary headers
#include "compiler/compiler.hpp"
#include "include/cli11.hpp"
#include "include/json.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

// #define DEBUG_LEXER
// #define DEBUG_PARSER

constexpr char DEBUG_LEXER_OUTPUT_PATH[] = "./dump/lexer_output.log";
constexpr char DEBUG_PARSER_OUTPUT_PATH[] = "./dump/parser_output.yaml";

using json = nlohmann::json;

// =======================================
// Custom Exception Classes
// =======================================

/**
 * @brief Exception thrown when a required file is not found.
 */
class FileNotFoundException : public std::runtime_error {
  public:
    /**
     * @brief Construct a new File Not Found Exception object
     *
     * @param message Detailed error message.
     */
    explicit FileNotFoundException(const std::string& message) : std::runtime_error(message) {}
};

/**
 * @brief Exception thrown during compilation failures.
 */
class CompilationException : public std::runtime_error {
  public:
    /**
     * @brief Construct a new Compilation Exception object
     *
     * @param message Detailed error message.
     */
    explicit CompilationException(const std::string& message) : std::runtime_error(message) {}
};

// =======================================
// Helper Function to Run External Commands
// =======================================

/**
 * @brief Executes a system command and captures its output.
 *
 * @param command The command to execute.
 * @param exit_code Reference to store the exit code of the command.
 * @return std::string The combined stdout and stderr output of the command.
 * @throws std::runtime_error If the command could not be executed.
 */
std::string runCommand(const std::string& command, int& exit_code) {
    std::array<char, 128> buffer;
    std::string result;

    // Open pipe to file
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) { throw std::runtime_error("popen() failed while executing command: " + command); }

    try {
        // Read till end of process
        while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) { result += buffer.data(); }
    } catch (...) {
        pclose(pipe);
        throw;
    }

    // Get the exit code
    exit_code = pclose(pipe);
    return result;
}

// =======================================
// Utility Namespace
// =======================================
namespace Utils {

/**
 * @brief Reads the entire content of a file into a string.
 *
 * @param filePath The path to the file.
 * @return std::string The content of the file.
 * @throws FileNotFoundException If the file cannot be opened.
 */
std::string readFileToString(const std::filesystem::path& filePath) {
    std::ifstream file(filePath, std::ios::in | std::ios::binary);
    if (!file) { throw FileNotFoundException("Error: Could not open file " + filePath.string()); }
    std::ostringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

/**
 * @brief Computes the hash of a string.
 *
 * @param content The string to hash.
 * @return size_t The computed hash.
 */
size_t computeHash(const std::string& content) {
    return std::hash<std::string>{}(content);
}

/**
 * @brief Creates directories if they do not exist.
 *
 * @param path The path to create.
 * @throws std::runtime_error If the directories cannot be created.
 */
void createDirectories(const std::filesystem::path& path) {
    std::error_code ec;
    if (!std::filesystem::create_directories(path, ec) && ec) { throw std::runtime_error("Error: Could not create directories " + path.string() + ": " + ec.message()); }
}

/**
 * @brief Writes JSON data to a file.
 *
 * @param filePath The path to the file.
 * @param data The JSON data to write.
 * @throws std::runtime_error If the file cannot be written.
 */
void writeJsonToFile(const std::filesystem::path& filePath, const json& data) {
    std::ofstream outFile(filePath, std::ios::trunc);
    if (!outFile) { throw std::runtime_error("Failed to write to JSON file: " + filePath.string()); }
    outFile << data.dump(4);
}

/**
 * @brief Reads JSON data from a file.
 *
 * @param filePath The path to the file.
 * @return json The JSON data read from the file.
 * @throws FileNotFoundException If the file cannot be opened.
 */
json readJsonFromFile(const std::filesystem::path& filePath) {
    std::ifstream inFile(filePath);
    if (!inFile) { throw FileNotFoundException("Failed to open JSON file: " + filePath.string()); }
    json data;
    inFile >> data;
    return data;
}

} // namespace Utils

// =======================================
// Environment Setup Class
// =======================================
class EnvManager {
  public:
    EnvManager() { loadEnvVars(); }

    bool isValid() const { return valid; }

    const std::filesystem::path& getStdDir() const { return GC_STD_DIR; }

    const std::filesystem::path& getStdIrGcMap() const { return GC_STD_IRGCMAP; }

  private:
    std::filesystem::path GC_STD_DIR;
    std::filesystem::path GC_STD_IRGCMAP;
    bool valid = true;

    /**
     * @brief Loads and validates required environment variables.
     */
    void loadEnvVars() {
        const char* gcStdDir = std::getenv("GC_STD_DIR");
        if (!gcStdDir) {
            std::cerr << "Warning: GC_STD_DIR environment variable is not set." << std::endl;
            valid = false;
        } else {
            GC_STD_DIR = std::filesystem::path(gcStdDir);
        }

        const char* gc_Std_IrGcMap = std::getenv("GC_STD_IRGCMAP");
        if (!gc_Std_IrGcMap) {
            std::cerr << "Warning: GC_STD_IRGCMAP environment variable is not set." << std::endl;
            valid = false;
        } else {
            GC_STD_IRGCMAP = std::filesystem::path(gc_Std_IrGcMap);
        }
    }
};

// =======================================
// Compiler Class
// =======================================
class Compiler {
  public:
    Compiler(const std::filesystem::path& srcDir, const std::filesystem::path& buildDir, const std::string& optimizationLevel, bool verbose)
        : srcDir(srcDir), buildDir(buildDir), optimizationLevel(optimizationLevel), verbose(verbose), irDir(buildDir / "ir"), irGcMapDir(buildDir / "ir_gc_map"), objDir(buildDir / "obj"),
          recordFilePath(buildDir / "compiled_files_record.json") {
        Utils::createDirectories(irDir);
        Utils::createDirectories(irGcMapDir);
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
     * @param compiledFilesRecord JSON object to track compiled files.
     */
    void compileAll(json& compiledFilesRecord) {
        // Update the ir_gc_map files
        updateIrGcMaps(compiledFilesRecord);

        // Collect all relevant files
        std::vector<std::filesystem::path> files;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
            if (entry.is_regular_file() && isSupportedFile(entry.path())) { files.emplace_back(entry.path()); }
        }

        if (verbose) { std::cout << "Found " << files.size() << " file(s) to compile." << std::endl; }

        // Multithreading setup
        const unsigned int numThreads = std::thread::hardware_concurrency();
        if (verbose) { std::cout << "Starting compilation with " << numThreads << " threads..." << std::endl; }

        std::mutex queueMutex;
        std::queue<std::filesystem::path> fileQueue;
        for (const auto& file : files) { fileQueue.push(file); }

        std::vector<std::thread> workers;
        for (unsigned int i = 0; i < numThreads; ++i) {
            workers.emplace_back([&]() {
                while (true) {
                    std::filesystem::path file;
                    {
                        std::lock_guard<std::mutex> lock(queueMutex);
                        if (fileQueue.empty()) return;
                        file = fileQueue.front();
                        fileQueue.pop();
                    }
                    try {
                        compileFile(file, compiledFilesRecord);
                    } catch (const std::exception& e) { std::cerr << "Error compiling " << file << ": " << e.what() << std::endl; }
                }
            });
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
        if (exit_code != 0) { throw CompilationException("Failed to link object files into executable " + executablePath.string() + "\nCommand: " + linkCommand + "\nOutput: " + linkOutput); }

        if (verbose) { std::cout << "Successfully linked object files into executable: " << executablePath << std::endl; }
        return 0;
    }

  private:
    std::filesystem::path srcDir;
    std::filesystem::path buildDir;
    std::string optimizationLevel;
    bool verbose;

    std::filesystem::path irDir;
    std::filesystem::path irGcMapDir;
    std::filesystem::path objDir;
    std::filesystem::path recordFilePath;

    std::mutex recordMutex; // Mutex to protect compiledFilesRecord

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
     * @brief Updates IR GC Map files before compilation.
     *
     * @param compiledFilesRecord JSON object tracking compiled files.
     */
    void updateIrGcMaps(const json& compiledFilesRecord) const {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
            if (entry.is_regular_file() && isSupportedFile(entry.path())) {
                auto relative = std::filesystem::relative(entry.path(), srcDir);
                std::filesystem::path irGcMapPath = irGcMapDir / (relative.string() + ".json");
                setIrGcMap(entry.path(), irGcMapPath, compiledFilesRecord);
            }
        }
    }

    /**
     * @brief Sets or updates the IR GC Map for a specific file.
     *
     * @param filePath The path to the source file.
     * @param irGcMapPath The path to the IR GC Map file.
     * @param compiledFilesRecord JSON object tracking compiled files.
     */
    void setIrGcMap(const std::filesystem::path& filePath, const std::filesystem::path& irGcMapPath, const json& compiledFilesRecord) const {
        std::string fileContent = Utils::readFileToString(filePath);
        size_t currentHash = Utils::computeHash(fileContent);

        json irGcMapJson;
        if (!std::filesystem::exists(irGcMapPath)) {
            irGcMapJson["uptodate"] = false;
            irGcMapJson["functions"] = json::object();
            irGcMapJson["structs"] = json::object();
            irGcMapJson["GSinstance"] = json::object();
            irGcMapJson["GFinstance"] = json::object();
            Utils::createDirectories(irGcMapPath.parent_path());

            Utils::writeJsonToFile(irGcMapPath, irGcMapJson);
        } else {
            irGcMapJson = Utils::readJsonFromFile(irGcMapPath);
        }

        if (compiledFilesRecord.contains(filePath.string()) && compiledFilesRecord[filePath.string()] == currentHash) {
            irGcMapJson["uptodate"] = true;
        } else {
            irGcMapJson["uptodate"] = false;
        }

        Utils::writeJsonToFile(irGcMapPath, irGcMapJson);
    }

    /**
     * @brief Compiles an individual file depending on its extension.
     *
     * @param filePath The path to the source file.
     * @param compiledFilesRecord JSON object tracking compiled files.
     */
    void compileFile(const std::filesystem::path& filePath, json& compiledFilesRecord) {
        auto relative = std::filesystem::relative(filePath, srcDir);
        std::filesystem::path outputIRPath = irDir / (relative.string() + ".ll");
        std::filesystem::path irGcMapPath = irGcMapDir / (relative.string() + ".json");
        std::filesystem::path objFilePath = objDir / (relative.string() + ".o");

        if (verbose) {
            std::cout << "Compiling file: " << filePath << "\n"
                      << " Output IR Path: " << outputIRPath << "\n"
                      << " IR GC Map Path: " << irGcMapPath << "\n"
                      << " Object File Path: " << objFilePath << "\n";
        }

        Utils::createDirectories(outputIRPath.parent_path());

        std::string fileContent = Utils::readFileToString(filePath);
        size_t currentHash = Utils::computeHash(fileContent);

        // Check if the file needs recompilation
        {
            std::lock_guard<std::mutex> lock(recordMutex);
            // if (compiledFilesRecord.contains(filePath.string()) && compiledFilesRecord[filePath.string()] == currentHash) {
            //     if (verbose) { std::cout << "Skipping up-to-date file: " << filePath << std::endl; }
            //     return;
            // }
        }

        // Compile based on file extension
        std::string extension = filePath.extension().string();
        if (extension == ".gc") {
            compileGcFile(fileContent, filePath, outputIRPath, irGcMapPath, objFilePath, compiledFilesRecord, currentHash);
        } else if (extension == ".c") {
            compileCFile(filePath, outputIRPath, irGcMapPath, objFilePath, compiledFilesRecord, currentHash);
        } else if (extension == ".rs") {
            compileRustFile(filePath, outputIRPath, objFilePath, compiledFilesRecord, currentHash);
        } else {
            throw std::runtime_error("Unsupported file type: " + filePath.string());
        }
    }

    /**
     * @brief Compiles a .gc source file.
     *
     * @param filePath The path to the .gc file.
     * @param outputIRPath The path to the output LLVM IR file.
     * @param irGcMapPath The path to the IR GC Map file.
     * @param objFilePath The path to the output object file.
     * @param compiledFilesRecord JSON object tracking compiled files.
     * @param currentHash The current hash of the source file.
     */
    void compileGcFile(std::string fileContent,
                       const std::filesystem::path& filePath,
                       const std::filesystem::path& outputIRPath,
                       const std::filesystem::path& irGcMapPath,
                       const std::filesystem::path& objFilePath,
                       json& compiledFilesRecord,
                       size_t currentHash) {
// Debugging Lexer
#ifdef DEBUG_LEXER
        debugLexer(Utils::readFileToString(filePath));
#endif

// Debugging Parser
#ifdef DEBUG_PARSER
        debugParser(Utils::readFileToString(filePath));
#endif

        Lexer lexer(fileContent);
        parser::Parser parser(&lexer);
        auto program = parser.parseProgram();

        compiler::Compiler comp(fileContent, std::filesystem::absolute(filePath), irGcMapPath, buildDir, std::filesystem::relative(filePath, srcDir).string());
        comp.compile(program);
        delete program;

        // Write LLVM IR to file
        std::error_code EC;
        llvm::raw_fd_ostream irFile(outputIRPath.string(), EC, llvm::sys::fs::OF_None);
        if (EC) { throw CompilationException("Could not open IR file " + outputIRPath.string() + ": " + EC.message()); }
        comp.llvm_module->print(irFile, nullptr);
        irFile.close();

        // Compile IR to object file
        Utils::createDirectories(objFilePath.parent_path());
        std::string optFlag = optimizationLevel.empty() ? "" : " -O" + optimizationLevel;
        std::string clangCommand = "clang -c \"" + outputIRPath.string() + "\" -o \"" + objFilePath.string() + "\" -Woverride-module" + optFlag;

        int clangResult;
        std::string clangOutput = runCommand(clangCommand, clangResult);
        if (clangResult != 0) { throw CompilationException("Failed to convert " + outputIRPath.string() + " to " + objFilePath.string() + "\nCommand: " + clangCommand + "\nOutput: " + clangOutput); }

        // Update compiled files record and IR GC Map
        {
            std::lock_guard<std::mutex> lock(recordMutex);
            compiledFilesRecord[filePath.string()] = currentHash;
        }

        json irGcMapJson = Utils::readJsonFromFile(irGcMapPath);
        irGcMapJson["uptodate"] = true;
        Utils::writeJsonToFile(irGcMapPath, irGcMapJson);

        if (verbose) { std::cout << "Compiled .gc file: " << filePath << std::endl; }
    }

    /**
     * @brief Compiles a .c source file.
     *
     * @param filePath The path to the .c file.
     * @param outputIRPath The path to the output LLVM IR file.
     * @param irGcMapPath The path to the IR GC Map file.
     * @param objFilePath The path to the output object file.
     * @param compiledFilesRecord JSON object tracking compiled files.
     * @param currentHash The current hash of the source file.
     */
    void compileCFile(const std::filesystem::path& filePath,
                      const std::filesystem::path& outputIRPath,
                      const std::filesystem::path& irGcMapPath,
                      const std::filesystem::path& objFilePath,
                      json& compiledFilesRecord,
                      size_t currentHash) {
        // Compile to LLVM IR
        std::string optFlag = optimizationLevel.empty() ? "" : " -O" + optimizationLevel;
        std::string clangCommandIR = "clang -emit-llvm -S " + optFlag + " \"" + filePath.string() + "\" -o \"" + outputIRPath.string() + "\"";

        int clangResultIR;
        std::string clangOutputIR = runCommand(clangCommandIR, clangResultIR);
        if (clangResultIR != 0) { throw CompilationException("Failed to compile " + filePath.string() + " to LLVM IR" + "\nCommand: " + clangCommandIR + "\nOutput: " + clangOutputIR); }

        // Compile to object file
        std::string clangCommandObj = "clang -c \"" + filePath.string() + "\" -o \"" + objFilePath.string() + "\" " + optFlag;

        int clangResultObj;
        std::string clangOutputObj = runCommand(clangCommandObj, clangResultObj);
        if (clangResultObj != 0) { throw CompilationException("Failed to compile " + filePath.string() + " to object file" + "\nCommand: " + clangCommandObj + "\nOutput: " + clangOutputObj); }

        // Update compiled files record and IR GC Map
        {
            std::lock_guard<std::mutex> lock(recordMutex);
            compiledFilesRecord[filePath.string()] = currentHash;
        }

        json irGcMapJson = Utils::readJsonFromFile(irGcMapPath);
        irGcMapJson["uptodate"] = true;
        Utils::writeJsonToFile(irGcMapPath, irGcMapJson);

        if (verbose) { std::cout << "Compiled .c file: " << filePath << std::endl; }
    }

    /**
     * @brief Compiles a .rs source file.
     *
     * @param filePath The path to the .rs file.
     * @param outputIRPath The path to the output LLVM IR file.
     * @param objFilePath The path to the output object file.
     * @param compiledFilesRecord JSON object tracking compiled files.
     * @param currentHash The current hash of the source file.
     */
    void compileRustFile(const std::filesystem::path& filePath, const std::filesystem::path& outputIRPath, const std::filesystem::path& objFilePath, json& compiledFilesRecord, size_t currentHash) {
        std::filesystem::path irFilePath = outputIRPath.string() + ".ll";
        Utils::createDirectories(irFilePath.parent_path());

        // Compile Rust to LLVM IR
        std::string rustcCommand = "rustc -emit=llvm-ir \"" + filePath.string() + "\" -o \"" + irFilePath.string() + "\"" + (optimizationLevel.empty() ? "" : " -C opt-level=" + optimizationLevel);

        int rustcResult;
        std::string rustcOutput = runCommand(rustcCommand, rustcResult);
        if (rustcResult != 0) { throw CompilationException("Failed to compile Rust file " + filePath.string() + " to LLVM IR" + "\nCommand: " + rustcCommand + "\nOutput: " + rustcOutput); }

        // Compile LLVM IR to object file
        std::string clangCommand = "clang -c \"" + irFilePath.string() + "\" -o \"" + objFilePath.string() + "\"" + (optimizationLevel.empty() ? "" : " -O" + optimizationLevel);

        int clangResult;
        std::string clangOutput = runCommand(clangCommand, clangResult);
        if (clangResult != 0) { throw CompilationException("Failed to convert " + irFilePath.string() + " to " + objFilePath.string() + "\nCommand: " + clangCommand + "\nOutput: " + clangOutput); }

        // Update compiled files record
        {
            std::lock_guard<std::mutex> lock(recordMutex);
            compiledFilesRecord[filePath.string()] = currentHash;
        }

        if (verbose) { std::cout << "Compiled Rust file: " << filePath << std::endl; }
    }

    /**
     * @brief Handles lexer debugging.
     *
     * @param fileContent The content of the source file.
     */
    void debugLexer(const std::string& fileContent) const {
#ifdef DEBUG_LEXER
        std::cout << "=========== Lexer Debug ===========" << std::endl;
        Lexer debugLexer(fileContent);
        if (std::filesystem::path(DEBUG_LEXER_OUTPUT_PATH).string().empty()) {
            while (debugLexer.current_char != "") {
                token::Token token = debugLexer.nextToken();
                std::cout << token.toString(true) << std::endl;
            }
        } else {
            std::ofstream debugOutput(DEBUG_LEXER_OUTPUT_PATH, std::ios::trunc);
            if (!debugOutput) {
                std::cerr << "Error: Could not open debug output file " << DEBUG_LEXER_OUTPUT_PATH << std::endl;
                return;
            }
            while (debugLexer.current_char != "") {
                token::Token token = debugLexer.nextToken();
                debugOutput << token.toString(false) << std::endl;
            }
            std::cout << "Lexer debug output written to " << DEBUG_LEXER_OUTPUT_PATH << std::endl;
        }
#endif
    }

    /**
     * @brief Handles parser debugging.
     *
     * @param fileContent The content of the source file.
     */
    void debugParser(const std::string& fileContent) const {
#ifdef DEBUG_PARSER
        parser::Parser debugParser(new Lexer(fileContent));
        auto program = debugParser.parseProgram();
        std::cout << "=========== Parser Debug ===========" << std::endl;
        if (!std::filesystem::path(DEBUG_PARSER_OUTPUT_PATH).string().empty()) {
            std::ofstream file(DEBUG_PARSER_OUTPUT_PATH, std::ios::trunc);
            if (file) {
                file << program->toStr() << std::endl;
                std::cout << "Parser debug output written to " << DEBUG_PARSER_OUTPUT_PATH << std::endl;
            } else {
                std::cerr << "Unable to open parser debug output file." << std::endl;
                exit(1);
            }
        } else {
            std::cout << program->toStr();
        }
        delete program;
#endif
    }
};

// =======================================
// CLI Setup Function
// =======================================
void setupCLI(CLI::App& app, std::filesystem::path& inputFolderPath, std::string& optimizationLevel, std::filesystem::path& executablePath, bool& verbose) {
    app.add_option("input_folder", inputFolderPath, "Input folder path")->required()->check(CLI::ExistingDirectory);
    app.add_option("-O,--optimization", optimizationLevel, "Optimization level (O1, O2, O3, Os, Ofast)")->default_val("");
    app.add_option("-o,--output", executablePath, "Output executable path")->required();
    app.add_flag("-v,--verbose", verbose, "Enable verbose output");
}

// =======================================
// Main Function
// =======================================
int main(int argc, char* argv[]) {
    try {
        // Initialize CLI
        CLI::App app{"Folder Compiler"};
        std::filesystem::path inputFolderPath;
        std::string optimizationLevel;
        std::filesystem::path executablePath;
        bool verbose = false;
        setupCLI(app, inputFolderPath, optimizationLevel, executablePath, verbose);
        CLI11_PARSE(app, argc, argv);

        if (verbose) { std::cout << "Verbose mode enabled." << std::endl; }

        // Environment Variable Management
        EnvManager envManager;
        if (!envManager.isValid()) {
            std::cerr << "Environment variables are missing. Exiting." << std::endl;
            return 1;
        }

        // Define Source and Build Directories
        std::filesystem::path srcDir = inputFolderPath / "src";
        std::filesystem::path buildDir = inputFolderPath / "build";

        if (!std::filesystem::exists(srcDir)) {
            std::cerr << "Error: Source directory " << srcDir << " does not exist." << std::endl;
            return 1;
        }

        // Initialize Compiler with verbose flag
        Compiler compiler(srcDir, buildDir, optimizationLevel, verbose);

        // Load Compiled Files Record
        std::filesystem::path recordFilePath = buildDir / "compiled_files_record.json";
        json compiledFilesRecord;
        if (std::filesystem::exists(recordFilePath)) {
            try {
                compiledFilesRecord = Utils::readJsonFromFile(recordFilePath);
            } catch (const FileNotFoundException& e) { std::cerr << "Warning: " << e.what() << " Proceeding without it." << std::endl; }
        } else {
            if (verbose) { std::cout << "Compiled files record does not exist. A new one will be created." << std::endl; }
        }

        // Compile All Files
        compiler.compileAll(compiledFilesRecord);

        // Save Compiled Files Record
        try {
            Utils::writeJsonToFile(recordFilePath, compiledFilesRecord);
            if (verbose) { std::cout << "Compiled files record updated at " << recordFilePath << std::endl; }
        } catch (const std::exception& e) { std::cerr << "Warning: Could not save compiled files record. " << e.what() << std::endl; }

        // Link Object Files into Executable
        return compiler.linkAll(executablePath);
    } catch (const CLI::ParseError& e) { return CLI::App().exit(e); } catch (const FileNotFoundException& e) {
        std::cerr << "File error: " << e.what() << std::endl;
        return 1;
    } catch (const CompilationException& e) {
        std::cerr << "Compilation error: " << e.what() << std::endl;
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "Compilation failed: " << e.what() << std::endl;
        return 1;
    }
}