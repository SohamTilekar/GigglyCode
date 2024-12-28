#include <llvm/IR/Module.h>
#include <llvm/Support/FileSystem.h>
#include <memory>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <iostream>
#include <system_error>
#include <vector>
#include <tuple>
#include <cstdlib>
#include <functional>
#include <thread>
#include <mutex>
#include <queue>
#include <condition_variable>

#include "compiler/compiler.hpp"
#include "include/cli11.hpp"
#include "include/json.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

// #define DEBUG_LEXER
// #define DEBUG_PARSER
#define DEBUG_LEXER_OUTPUT_PATH "./dump/lexer_output.log"
#define DEBUG_PARSER_OUTPUT_PATH "./dump/parser_output.yaml"

using json = nlohmann::json;
namespace fs = std::filesystem;

// =======================================
// Custom Exception Classes
// =======================================
class CompilerException : public std::runtime_error {
public:
    explicit CompilerException(const std::string& message)
        : std::runtime_error(message) {}
};

class FileNotFoundException : public CompilerException {
public:
    explicit FileNotFoundException(const std::string& message)
        : CompilerException(message) {}
};

class CompilationException : public CompilerException {
public:
    explicit CompilationException(const std::string& message)
        : CompilerException(message) {}
};

// =======================================
// Logging System
// =======================================
enum class LogLevel {
    DEBUG,
    INFO,
    WARNING,
    ERROR,
    CRITICAL
};

class Logger {
public:
    Logger(LogLevel level = LogLevel::INFO)
        : current_level(level) {}

    void setLevel(LogLevel level) {
        current_level = level;
    }

    void log(LogLevel level, const std::string& message) {
        if (level >= current_level) {
            std::lock_guard<std::mutex> lock(log_mutex);
            std::cout << "[" << levelToString(level) << "] " << message << std::endl;
        }
    }

    void debug(const std::string& message) {
        log(LogLevel::DEBUG, message);
    }

    void info(const std::string& message) {
        log(LogLevel::INFO, message);
    }

    void warning(const std::string& message) {
        log(LogLevel::WARNING, message);
    }

    void error(const std::string& message) {
        log(LogLevel::ERROR, message);
    }

    void critical(const std::string& message) {
        log(LogLevel::CRITICAL, message);
    }

private:
    LogLevel current_level;
    std::mutex log_mutex;

    std::string levelToString(LogLevel level) const {
        switch (level) {
            case LogLevel::DEBUG:    return "DEBUG";
            case LogLevel::INFO:     return "INFO";
            case LogLevel::WARNING:  return "WARNING";
            case LogLevel::ERROR:    return "ERROR";
            case LogLevel::CRITICAL: return "CRITICAL";
            default:                 return "UNKNOWN";
        }
    }
};

// =======================================
// Thread Pool Implementation
// =======================================
class ThreadPool {
public:
    ThreadPool(size_t num_threads)
        : stop(false) {
        for (size_t i = 0; i < num_threads; ++i)
            workers.emplace_back([this]() { this->worker(); });
    }

    ~ThreadPool() {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            stop = true;
        }
        condition.notify_all();
        for (std::thread &worker : workers)
            worker.join();
    }

    // Enqueue a task
    template<class F, class... Args>
    void enqueue(F&& f, Args&&... args) {
        {
            std::unique_lock<std::mutex> lock(queue_mutex);
            tasks.emplace(std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        }
        condition.notify_one();
    }

private:
    // Workers
    std::vector<std::thread> workers;
    // Task queue
    std::queue<std::function<void()>> tasks;
    // Synchronization
    std::mutex queue_mutex;
    std::condition_variable condition;
    bool stop;

    // Worker thread function
    void worker() {
        while (true) {
            std::function<void()> task;
            {
                std::unique_lock<std::mutex> lock(this->queue_mutex);
                this->condition.wait(lock,
                    [this]() { return this->stop || !this->tasks.empty(); });
                if (this->stop && this->tasks.empty())
                    return;
                task = std::move(this->tasks.front());
                this->tasks.pop();
            }
            task();
        }
    }
};

// =======================================
// Utility Namespace
// =======================================
namespace Utils {

    // Function to read file content into a string
    std::string readFileToString(const fs::path& filePath) {
        std::ifstream file(filePath, std::ios::in | std::ios::binary);
        if (!file) {
            throw FileNotFoundException("Error: Could not open file " + filePath.string());
        }
        std::ostringstream ss;
        ss << file.rdbuf();
        return ss.str();
    }

    // Function to compute hash of a string
    size_t computeHash(const std::string& content) {
        return std::hash<std::string>{}(content);
    }

    // Function to create directories safely
    void createDirectories(const fs::path& path) {
        std::error_code ec;
        if (!fs::exists(path)) {
            if (!fs::create_directories(path, ec)) {
                throw CompilerException("Error: Could not create directory " + path.string() + ". " + ec.message());
            }
        }
    }

    // Function to load JSON configuration
    json loadConfig(const fs::path& configPath) {
        if (!fs::exists(configPath)) {
            throw FileNotFoundException("Configuration file not found: " + configPath.string());
        }
        std::ifstream configFile(configPath);
        if (!configFile.is_open()) {
            throw CompilerException("Failed to open configuration file: " + configPath.string());
        }
        json config;
        configFile >> config;
        return config;
    }

    // Function to save JSON data atomically
    void writeJson(const fs::path& filePath, const json& data) {
        fs::path tempPath = filePath;
        tempPath += ".tmp";
        std::ofstream outFile(tempPath, std::ios::trunc);
        if (!outFile.is_open()) {
            throw CompilerException("Failed to open file for writing: " + tempPath.string());
        }
        outFile << data.dump(4);
        outFile.close();
        fs::rename(tempPath, filePath);
    }

    // Function to read JSON data
    json readJson(const fs::path& filePath) {
        if (!fs::exists(filePath)) {
            throw FileNotFoundException("Record file not found: " + filePath.string());
        }
        std::ifstream inFile(filePath);
        if (!inFile.is_open()) {
            throw CompilerException("Failed to open record file: " + filePath.string());
        }
        json data;
        inFile >> data;
        return data;
    }

    // Function to escape shell arguments
    std::string escapeArg(const std::string& arg) {
        std::string escaped = "\"";
        for (char c : arg) {
            if (c == '\\' || c == '\"') {
                escaped += '\\';
            }
            escaped += c;
        }
        escaped += '"';
        return escaped;
    }

} // namespace Utils

// =======================================
// Environment Setup Class
// =======================================
class EnvManager {
public:
    EnvManager(Logger& logger)
        : valid(true), logger(logger) {
        loadEnvVars();
    }

    bool isValid() const {
        return valid;
    }

    fs::path getStdDir() const {
        return GC_STD_DIR;
    }

    fs::path getStdIrGcMap() const {
        return GC_STD_IRGCMAP;
    }

private:
    fs::path GC_STD_DIR;
    fs::path GC_STD_IRGCMAP;
    bool valid;
    Logger& logger;

    void loadEnvVars() {
        const char* gcStdDir = std::getenv("GC_STD_DIR");
        if (gcStdDir == nullptr) {
            logger.warning("GC_STD_DIR environment variable is not set.");
            valid = false;
        } else {
            GC_STD_DIR = fs::path(gcStdDir);
        }

        const char* gc_Std_IrGcMap = std::getenv("GC_STD_IRGCMAP");
        if (gc_Std_IrGcMap == nullptr) {
            logger.warning("GC_STD_IRGCMAP environment variable is not set.");
            valid = false;
        } else {
            GC_STD_IRGCMAP = fs::path(gc_Std_IrGcMap);
        }
    }
};

// =======================================
// Compiler Class
// =======================================
class Compiler {
public:
    Compiler(const fs::path& srcDir, const fs::path& buildDir,
             const std::string& optimizationLevel, Logger& logger)
        : srcDir(srcDir), buildDir(buildDir),
          optimizationLevel(optimizationLevel), logger(logger),
          threadPool(std::thread::hardware_concurrency()) {

        irDir = buildDir / "ir";
        irGcMapDir = buildDir / "ir_gc_map";
        objDir = buildDir / "obj";
        recordFilePath = buildDir / "compiled_files_record.json";

        Utils::createDirectories(irDir);
        Utils::createDirectories(irGcMapDir);
        Utils::createDirectories(objDir);

        logger.info("Compiler initialized with:");
        logger.info(" Source Directory: " + srcDir.string());
        logger.info(" Build Directory: " + buildDir.string());
        logger.info(" Optimization Level: " + (optimizationLevel.empty() ? "None" : optimizationLevel));
    }

    void compileAll(json& compiledFilesRecord) {
        // Update the ir_gc_map files
        updateIrGcMaps(compiledFilesRecord);

        // Collect all relevant files
        std::vector<fs::path> files;
        for (const auto& entry : fs::recursive_directory_iterator(srcDir)) {
            if (entry.is_regular_file() && isSupportedFile(entry.path())) {
                files.emplace_back(entry.path());
            }
        }

        const size_t totalFiles = files.size();
        logger.info("Starting compilation of " + std::to_string(totalFiles) + " files...");

        std::mutex record_mutex;

        for (const auto& file : files) {
            threadPool.enqueue([this, &file, &compiledFilesRecord, &record_mutex]() {
                try {
                    compileFile(file, compiledFilesRecord, record_mutex);
                } catch (const std::exception& e) {
                    logger.error(e.what());
                }
            });
        }
        // Destructor of ThreadPool will wait for all tasks to finish
    }

    int linkAll(const fs::path& executablePath) {
        std::vector<std::string> objFiles;
        for (const auto& entry : fs::recursive_directory_iterator(objDir)) {
            if (entry.is_regular_file() && entry.path().extension() == ".o") {
                objFiles.emplace_back(entry.path().string());
            }
        }

        if (objFiles.empty()) {
            logger.error("No object files found to link.");
            return 1;
        }

        logger.info("Linking object files into executable...");

        // Construct the clang command safely
        std::string cmd = "clang";
        for (const auto& obj : objFiles) {
            cmd += " " + Utils::escapeArg(obj);
        }
        cmd += " -o " + Utils::escapeArg(executablePath.string());

        int linkResult = std::system(cmd.c_str());
        if (linkResult != 0) {
            logger.error("Failed to link object files into executable " + executablePath.string());
            return 1;
        }

        logger.info("Successfully linked object files into executable: " + executablePath.string());
        return 0;
    }

private:
    fs::path srcDir;
    fs::path buildDir;
    std::string optimizationLevel;
    fs::path irDir;
    fs::path irGcMapDir;
    fs::path objDir;
    fs::path recordFilePath;
    Logger& logger;
    ThreadPool threadPool;

    // Helper to check supported file extensions
    bool isSupportedFile(const fs::path& path) const {
        return path.extension() == ".gc" || path.extension() == ".c" || path.extension() == ".rs";
    }

    // Update ir_gc_map files before compilation
    void updateIrGcMaps(json& compiledFilesRecord) {
        for (const auto& entry : fs::recursive_directory_iterator(srcDir)) {
            if (entry.is_regular_file() && isSupportedFile(entry.path())) {
                fs::path irGcMapPath = irGcMapDir / (fs::relative(entry.path(), srcDir).string() + ".json");
                setIrGcMap(entry.path(), irGcMapPath, compiledFilesRecord);
            }
        }
    }

    // Function to set/update the IR GC Map
    void setIrGcMap(const fs::path& filePath, const fs::path& irGcMapPath, json& compiledFilesRecord) {
        std::string fileContent = Utils::readFileToString(filePath);
        size_t currentHash = Utils::computeHash(fileContent);

        json irGcMapJson;
        if (!fs::exists(irGcMapPath)) {
            irGcMapJson["uptodate"] = false;
            irGcMapJson["functions"] = json::object();
            irGcMapJson["structs"] = json::object();
            irGcMapJson["GSinstance"] = json::object();
            irGcMapJson["GFinstance"] = json::object();
            Utils::createDirectories(irGcMapPath.parent_path());

            Utils::writeJson(irGcMapPath, irGcMapJson);
            logger.debug("Created new IR GC Map file: " + irGcMapPath.string());
        } else {
            irGcMapJson = Utils::readJson(irGcMapPath);
        }

        if (compiledFilesRecord.contains(filePath.string()) && compiledFilesRecord[filePath.string()] == currentHash) {
            irGcMapJson["uptodate"] = true;
        } else {
            irGcMapJson["uptodate"] = false;
        }

        Utils::writeJson(irGcMapPath, irGcMapJson);
    }

    // Function to compile individual files
    void compileFile(const fs::path& filePath, json& compiledFilesRecord, std::mutex& record_mutex) {
        std::string relativePathStr = fs::relative(filePath, srcDir).string();
        fs::path outputIRPath = irDir / (relativePathStr + ".ll");
        fs::path irGcMapPath = irGcMapDir / (relativePathStr + ".json");
        fs::path objFilePath = objDir / (relativePathStr + ".o");

        logger.debug("Compiling file: " + filePath.string());
        logger.debug(" Output IR Path: " + outputIRPath.string());
        logger.debug(" IR GC Map Path: " + irGcMapPath.string());
        logger.debug(" Object File Path: " + objFilePath.string());

        Utils::createDirectories(outputIRPath.parent_path());

        std::string fileContent = Utils::readFileToString(filePath);
        size_t currentHash = Utils::computeHash(fileContent);

        // Check if the file needs recompilation
        {
            std::lock_guard<std::mutex> lock(record_mutex);
            if (compiledFilesRecord.contains(filePath.string()) && compiledFilesRecord[filePath.string()] == currentHash) {
                logger.info("Skipping up-to-date file: " + filePath.string());
                return;
            }
        }

        // Compile based on file extension
        if (filePath.extension() == ".gc") {
            compileGcFile(filePath, outputIRPath, irGcMapPath, objFilePath, relativePathStr, compiledFilesRecord, record_mutex);
        } else if (filePath.extension() == ".c") {
            compileCFile(filePath, outputIRPath, irGcMapPath, objFilePath, relativePathStr, compiledFilesRecord, record_mutex);
        } else if (filePath.extension() == ".rs") {
            compileRustFile(filePath, outputIRPath, objFilePath, relativePathStr, compiledFilesRecord, currentHash, record_mutex);
        } else {
            throw CompilationException("Unsupported file type: " + filePath.string());
        }
    }

    // Compile .gc files
    void compileGcFile(const fs::path& filePath,
                       const fs::path& outputIRPath,
                       const fs::path& irGcMapPath,
                       const fs::path& objFilePath,
                       const std::string& relativePathStr,
                       json& compiledFilesRecord,
                       std::mutex& record_mutex) {
        std::string fileContent = Utils::readFileToString(filePath);

#ifdef DEBUG_LEXER
        debugLexer(fileContent);
#endif

#ifdef DEBUG_PARSER
        debugParser(fileContent);
#endif

        Lexer lexer(fileContent);
        parser::Parser parser(&lexer);
        auto program = parser.parseProgram();

        compiler::Compiler comp(fileContent, fs::absolute(filePath),
                                irGcMapPath, buildDir, relativePathStr);
        comp.compile(program);
        delete program;

        // Write LLVM IR to file
        std::error_code EC;
        llvm::raw_fd_ostream irFile(outputIRPath.string(), EC, llvm::sys::fs::OF_None);
        if (EC) {
            throw CompilerException("Could not open IR file " + outputIRPath.string() + ": " + EC.message());
        }
        comp.llvm_module->print(irFile, nullptr);
        irFile.close();

        // Compile IR to object file
        Utils::createDirectories(objFilePath.parent_path());
        std::string optFlag = optimizationLevel.empty() ? "" : " -O" + optimizationLevel;
        std::string clangCommand = "clang -c " + Utils::escapeArg(outputIRPath.string()) +
                                   " -o " + Utils::escapeArg(objFilePath.string()) + optFlag;
        int clangResult = std::system(clangCommand.c_str());
        if (clangResult != 0) {
            throw CompilationException("Failed to convert " + outputIRPath.string() +
                                       " to " + objFilePath.string());
        }

        // Update compiled files record and IR GC Map
        {
            std::lock_guard<std::mutex> lock(record_mutex);
            compiledFilesRecord[filePath.string()] = Utils::computeHash(fileContent);
        }

        json irGcMapJson = Utils::readJson(irGcMapPath);
        irGcMapJson["uptodate"] = true;
        Utils::writeJson(irGcMapPath, irGcMapJson);

        logger.info("Compiled .gc file: " + filePath.string());
    }

    // Compile .c files
    void compileCFile(const fs::path& filePath,
                      const fs::path& outputIRPath,
                      const fs::path& irGcMapPath,
                      const fs::path& objFilePath,
                      const std::string& relativePathStr,
                      json& compiledFilesRecord,
                      std::mutex& record_mutex) {
        // Compile to LLVM IR
        std::string optFlag = optimizationLevel.empty() ? "" : " -O" + optimizationLevel;
        std::string clangCommandIR = "clang -emit-llvm -S " + optFlag + " " +
                                     Utils::escapeArg(filePath.string()) + " -o " +
                                     Utils::escapeArg(outputIRPath.string());
        int clangResultIR = std::system(clangCommandIR.c_str());
        if (clangResultIR != 0) {
            throw CompilationException("Failed to compile " + filePath.string() + " to LLVM IR");
        }

        // Compile to object file
        Utils::createDirectories(objFilePath.parent_path());
        std::string clangCommandObj = "clang -c " + Utils::escapeArg(filePath.string()) +
                                      " -o " + Utils::escapeArg(objFilePath.string()) + optFlag;
        int clangResultObj = std::system(clangCommandObj.c_str());
        if (clangResultObj != 0) {
            throw CompilationException("Failed to compile " + filePath.string() + " to object file");
        }

        // Update compiled files record and IR GC Map
        std::string fileContent = Utils::readFileToString(filePath);
        size_t currentHash = Utils::computeHash(fileContent);
        {
            std::lock_guard<std::mutex> lock(record_mutex);
            compiledFilesRecord[filePath.string()] = currentHash;
        }

        json irGcMapJson = Utils::readJson(irGcMapPath);
        irGcMapJson["uptodate"] = true;
        Utils::writeJson(irGcMapPath, irGcMapJson);

        logger.info("Compiled .c file: " + filePath.string());
    }

    // Compile .rs files
    void compileRustFile(const fs::path& filePath,
                         const fs::path& outputIRPath,
                         const fs::path& objFilePath,
                         const std::string& relativePathStr,
                         json& compiledFilesRecord,
                         size_t currentHash,
                         std::mutex& record_mutex) {
        fs::path irFilePath = outputIRPath;
        irFilePath += ".ll";
        Utils::createDirectories(irFilePath.parent_path());

        // Compile Rust to LLVM IR
        std::string rustcCommand = "rustc -emit=llvm-ir " + Utils::escapeArg(filePath.string()) +
                                    " -o " + Utils::escapeArg(irFilePath.string()) +
                                    (optimizationLevel.empty() ? "" : " -C opt-level=" + optimizationLevel);
        int rustcResult = std::system(rustcCommand.c_str());
        if (rustcResult != 0) {
            throw CompilationException("Failed to compile Rust file " + filePath.string() + " to LLVM IR");
        }

        // Compile LLVM IR to object file
        Utils::createDirectories(objFilePath.parent_path());
        std::string clangCommand = "clang -c " + Utils::escapeArg(irFilePath.string()) +
                                   " -o " + Utils::escapeArg(objFilePath.string()) +
                                   (optimizationLevel.empty() ? "" : " -O" + optimizationLevel);
        int clangResult = std::system(clangCommand.c_str());
        if (clangResult != 0) {
            throw CompilationException("Failed to convert " + irFilePath.string() + " to " + objFilePath.string());
        }

        // Update compiled files record
        {
            std::lock_guard<std::mutex> lock(record_mutex);
            compiledFilesRecord[filePath.string()] = currentHash;
        }

        logger.info("Compiled Rust file: " + filePath.string());
    }

    // Function to handle lexer debugging
    void debugLexer(const std::string& fileContent) {
#ifdef DEBUG_LEXER
        std::ofstream debugOutput(DEBUG_LEXER_OUTPUT_PATH, std::ios::trunc);
        if (!debugOutput.is_open()) {
            logger.error("Could not open debug output file " + std::string(DEBUG_LEXER_OUTPUT_PATH));
            return;
        }

        Lexer debugLexer(fileContent);
        while (debugLexer.current_char != "") {
            token::Token token = debugLexer.nextToken();
            debugOutput << token.toString(false) << std::endl;
        }
        debugOutput.close();
        logger.debug("Lexer debug output written to " + std::string(DEBUG_LEXER_OUTPUT_PATH));
#endif
    }

    // Function to handle parser debugging
    void debugParser(const std::string& fileContent) {
#ifdef DEBUG_PARSER
        parser::Parser debugParser(new Lexer(fileContent));
        auto program = debugParser.parseProgram();

        std::ofstream file(DEBUG_PARSER_OUTPUT_PATH, std::ios::trunc);
        if (file.is_open()) {
            file << program->toStr() << std::endl;
            file.close();
            logger.debug("Parser debug output written to " + std::string(DEBUG_PARSER_OUTPUT_PATH));
        } else {
            logger.error("Unable to open parser debug output file.");
            exit(1);
        }

        delete program;
#endif
    }
};

// =======================================
// CLI Setup Function
// =======================================
void setupCLI(CLI::App& app, std::string& inputFolderPath, std::string& optimizationLevel,
             std::string& executablePath, bool& verbose) {
    app.add_option("input_folder", inputFolderPath, "Input folder path")->required();
    app.add_option("-O,--optimization", optimizationLevel, "Optimization level (O1, O2, O3, Os, Ofast)")
       ->default_val("");
    app.add_option("-o,--output", executablePath, "Output executable path")->required();
    app.add_flag("-v,--verbose", verbose, "Enable verbose output");
}

// =======================================
// Main Function
// =======================================
int main(int argc, char* argv[]) {
    // Initialize Logger with default level INFO
    Logger logger(LogLevel::INFO);

    try {
        // Initialize CLI
        CLI::App app{"Folder Compiler"};
        std::string inputFolderPath;
        std::string optimizationLevel;
        std::string executablePath;
        bool verbose = false;
        setupCLI(app, inputFolderPath, optimizationLevel, executablePath, verbose);
        CLI11_PARSE(app, argc, argv);

        if (verbose) {
            logger.setLevel(LogLevel::DEBUG);
            logger.debug("Verbose mode enabled.");
        }

        // Environment Variable Management
        EnvManager envManager(logger);
        if (!envManager.isValid()) {
            logger.critical("Required environment variables are missing. Exiting.");
            return 1;
        }

        // Define Source and Build Directories
        fs::path srcDir = fs::path(inputFolderPath) / "src";
        fs::path buildDir = fs::path(inputFolderPath) / "build";

        // Initialize Compiler with verbose flag
        Compiler compiler(srcDir, buildDir, optimizationLevel, logger);

        // Load Compiled Files Record
        fs::path recordFilePath = buildDir / "compiled_files_record.json";
        json compiledFilesRecord;
        if (fs::exists(recordFilePath)) {
            try {
                compiledFilesRecord = Utils::readJson(recordFilePath);
                logger.info("Loaded compiled files record.");
            } catch (const FileNotFoundException& e) {
                logger.warning(e.what() + std::string(" Proceeding without it."));
            } catch (const CompilerException& e) {
                logger.warning(e.what() + std::string(" Proceeding without it."));
            }
        }

        // Compile All Files
        compiler.compileAll(compiledFilesRecord);

        // Save Compiled Files Record
        try {
            Utils::writeJson(recordFilePath, compiledFilesRecord);
            logger.info("Saved compiled files record.");
        } catch (const CompilerException& e) {
            logger.warning(e.what());
        }

        // Link Object Files into Executable
        int linkResult = compiler.linkAll(fs::path(executablePath));
        if (linkResult != 0) {
            logger.critical("Linking failed with exit code " + std::to_string(linkResult));
            return linkResult;
        }

        logger.info("Compilation and linking completed successfully.");
        return 0;
    } catch (const CLI::ParseError& e) {
        return CLI::App().exit(e);
    } catch (const FileNotFoundException& e) {
        logger.error(e.what());
        return 1;
    } catch (const CompilationException& e) {
        logger.error(e.what());
        return 1;
    } catch (const CompilerException& e) {
        logger.error(e.what());
        return 1;
    } catch (const std::exception& e) {
        logger.critical("Unexpected error: " + std::string(e.what()));
        return 1;
    }
}
