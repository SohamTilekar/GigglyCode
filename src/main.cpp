#include <llvm/Support/FileSystem.h>
#include <memory>

#include "compiler/compiler.hpp"
#include "include/cli11.hpp"
#include "include/json.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#define DEBUG_LEXER
#define DEBUG_PARSER
#define DEBUG_LEXER_OUTPUT_PATH "./dump/lexer_output.log"
#define DEBUG_PARSER_OUTPUT_PATH "./dump/parser_output.yaml"

using json = nlohmann::json;
std::filesystem::path GC_STD_DIR;
std::filesystem::path GC_STD_IRGCMAP;
bool dev_node = true;

// Function to read the file content into a string
const std::string readFileToString(const std::string& filePath) {
    std::cout << "filePath: " << filePath << std::endl;
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return "";
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void setIrGcMap(const std::string& filePath, const std::string& ir_gc_map, json& compiledFilesRecord) {
    std::string fileContent = readFileToString(filePath);
    std::hash<std::string> hasher;
    size_t currentHash = hasher(fileContent);

    // Initialize the ir_gc_map JSON structure if the file does not exist
    json ir_gc_map_json;
    if (!std::filesystem::exists(ir_gc_map)) {
        ir_gc_map_json["uptodate"] = false;
        ir_gc_map_json["functions"] = json::object();
        ir_gc_map_json["structs"] = json::object();
        ir_gc_map_json["GSinstance"] = nlohmann::json::object();
        ir_gc_map_json["GFinstance"] = nlohmann::json::object();
        // Create the ir_gc_map file
        std::filesystem::create_directories(std::filesystem::path(ir_gc_map).parent_path());
        std::ofstream ir_gc_map_file_out(ir_gc_map, std::ios::trunc);
        if (!ir_gc_map_file_out.is_open()) { throw std::runtime_error("Failed to open ir_gc_map file for writing: " + ir_gc_map); }
        ir_gc_map_file_out << ir_gc_map_json.dump(4);
        ir_gc_map_file_out.close();
    } else {
        std::ifstream ir_gc_map_file(ir_gc_map);
        if (!ir_gc_map_file.is_open()) { throw std::runtime_error("Failed to open ir_gc_map file: " + ir_gc_map); }
        ir_gc_map_file >> ir_gc_map_json;
        ir_gc_map_file.close();
    }

    if (compiledFilesRecord.contains(filePath) && compiledFilesRecord[filePath] == currentHash) {
        ir_gc_map_json["uptodate"] = true;
    } else {
        ir_gc_map_json["uptodate"] = false;
    }

    std::ofstream ir_gc_map_file_out(ir_gc_map, std::ios::trunc);
    if (!ir_gc_map_file_out.is_open()) { throw std::runtime_error("Failed to open ir_gc_map file for writing: " + ir_gc_map); }
    ir_gc_map_file_out << ir_gc_map_json.dump(4);
    ir_gc_map_file_out.close();
}

void debugLexer(const std::string& fileContent) {
#ifdef DEBUG_LEXER
    std::cout << "=========== Lexer Debug ===========" << std::endl;
    Lexer debug_lexer(fileContent);
    if (std::string(DEBUG_LEXER_OUTPUT_PATH) != "") {
        std::ofstream debugOutput(DEBUG_LEXER_OUTPUT_PATH, std::ios::trunc);
        if (!debugOutput.is_open()) {
            std::cerr << "Error: Could not open debug output file " << DEBUG_LEXER_OUTPUT_PATH << std::endl;
            return;
        }
        while (debug_lexer.current_char != "") {
            token::Token token = debug_lexer.nextToken();
            debugOutput << token.toString(false) << std::endl;
        }
        debugOutput.close();
        std::cout << "Debug output written to " << DEBUG_LEXER_OUTPUT_PATH << std::endl;
    } else {
        while (debug_lexer.current_char != "") {
            token::Token token = debug_lexer.nextToken();
            std::cout << token.toString(true) << std::endl;
        }
    }
#endif
}

void debugParser(const std::string& fileContent) {
#ifdef DEBUG_PARSER
    parser::Parser debug_parser(new Lexer(fileContent));
    auto debug_program = debug_parser.parseProgram();
    std::cout << "=========== Parser Debug ===========" << std::endl;
    if (!std::string(DEBUG_PARSER_OUTPUT_PATH).empty()) {
        std::ofstream file(DEBUG_PARSER_OUTPUT_PATH, std::ios::trunc); // Open file in append mode
        if (file.is_open()) {
            file << debug_program->toStr() << std::endl;
            file.close();
        } else {
            std::cerr << "Unable to open file";
            exit(1);
        }
    } else {
        std::cout << debug_program->toStr();
    }
    if (!std::string(DEBUG_PARSER_OUTPUT_PATH).empty()) { std::cout << "Parser output dumped to " << DEBUG_PARSER_OUTPUT_PATH << std::endl; }
    delete debug_program;
#endif
}

void compileGcFile(const std::string& filePath,
                   const std::string& outputFilePath,
                   const std::string& ir_gc_map,
                   const std::string& objFilePath,
                   const std::string& buildDir,
                   const std::string& relativePath,
                   json& compiledFilesRecord,
                   const std::string& optimizationLevel,
                   size_t currentHash) {
    std::string fileContent = readFileToString(filePath);
    debugLexer(fileContent);
    debugParser(fileContent);

    auto lexer = new Lexer(fileContent);
    parser::Parser parsr(lexer);
    auto program = parsr.parseProgram();
    auto comp = compiler::Compiler(fileContent, std::filesystem::absolute(filePath), std::filesystem::path(ir_gc_map), buildDir, relativePath);
    comp.compile(program);
    delete program;

    std::error_code EC;
    llvm::raw_fd_ostream file(outputFilePath, EC, llvm::sys::fs::OF_None);
    if (EC) {
        std::cerr << "Could not open file " << outputFilePath << ": " << EC.message() << std::endl;
        exit(1);
    }
    comp.llvm_module->print(file, nullptr);
    file.close();
    std::cout << "Output File: " << outputFilePath << std::endl;

    std::filesystem::create_directories(std::filesystem::path(objFilePath).parent_path());
    std::string command = "clang -c " + outputFilePath + " -o " + objFilePath + " -Woverride-module" + (optimizationLevel != "" ? (" -O" + optimizationLevel) : "");
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Error: Failed to convert " << outputFilePath << " to " << objFilePath << std::endl;
    } else {
        std::cout << "Converted " << outputFilePath << " to " << objFilePath << std::endl;
    }

    compiledFilesRecord[filePath] = currentHash;
    std::ofstream ir_gc_map_file_out(ir_gc_map, std::ios::trunc);
    if (!ir_gc_map_file_out.is_open()) { throw std::runtime_error("Failed to open ir_gc_map file for writing: " + ir_gc_map); }
    comp.ir_gc_map_json["uptodate"] = true;
    ir_gc_map_file_out << comp.ir_gc_map_json.dump(4);
    ir_gc_map_file_out.close();
}

void compileCFile(const std::string& filePath,
                  const std::string& outputFilePath,
                  const std::string& ir_gc_map,
                  const std::string& objFilePath,
                  const std::string& optimizationLevel,
                  json& compiledFilesRecord,
                  size_t currentHash) {
    std::filesystem::create_directories(std::filesystem::path(objFilePath).parent_path());
    std::string command = "clang -emit-llvm -S " + (optimizationLevel != "" ? (" -O" + optimizationLevel) : "") + filePath + " -o " + outputFilePath;
    int result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Error: Failed to compile " << filePath << " to " << outputFilePath << std::endl;
        exit(1);
    } else {
        std::cout << "Compiled " << filePath << " to " << outputFilePath << std::endl;
    }

    command = "clang -c " + filePath + " -o " + objFilePath + (optimizationLevel != "" ? (" -O" + optimizationLevel) : "");
    result = std::system(command.c_str());
    if (result != 0) {
        std::cerr << "Error: Failed to compile " << filePath << " to " << objFilePath << std::endl;
        exit(1);
    } else {
        std::cout << "Compiled " << filePath << " to " << objFilePath << std::endl;
    }

    compiledFilesRecord[filePath] = currentHash;
    json ir_gc_map_json;
    std::ifstream ir_gc_map_file(ir_gc_map);
    if (!ir_gc_map_file.is_open()) { throw std::runtime_error("Failed to open ir_gc_map file: " + ir_gc_map); }
    ir_gc_map_file >> ir_gc_map_json;
    ir_gc_map_file.close();
    ir_gc_map_json["uptodate"] = true;
    std::ofstream ir_gc_map_file_out(ir_gc_map, std::ios::trunc);
    if (!ir_gc_map_file_out.is_open()) { throw std::runtime_error("Failed to open ir_gc_map file for writing: " + ir_gc_map); }
    ir_gc_map_file_out << ir_gc_map_json.dump(4);
    ir_gc_map_file_out.close();
}

void compileRustFile(
    const std::string& filePath, const std::string& outputFilePath, const std::string& objFilePath, const std::string& optimizationLevel, json& compiledFilesRecord, size_t currentHash) {
    std::string irFilePath = outputFilePath + ".ll";
    std::filesystem::create_directories(std::filesystem::path(irFilePath).parent_path());
    std::string rustcCommand = "rustc -emit=llvm-ir " + filePath + " -o " + irFilePath + (optimizationLevel != "" ? (" -C opt-level=" + optimizationLevel) : "");
    int rustcResult = std::system(rustcCommand.c_str());
    if (rustcResult != 0) {
        std::cerr << "Error: Failed to compile " << filePath << " to LLVM IR" << std::endl;
    } else {
        std::cout << "Compiled " << filePath << " to LLVM IR" << std::endl;
        std::filesystem::create_directories(std::filesystem::path(objFilePath).parent_path());
        std::string clangCommand = "clang -c " + irFilePath + " -o " + objFilePath + (optimizationLevel != "" ? (" -O" + optimizationLevel) : "");
        int clangResult = std::system(clangCommand.c_str());
        if (clangResult != 0) {
            std::cerr << "Error: Failed to convert " << irFilePath << " to " << objFilePath << std::endl;
        } else {
            std::cout << "Converted " << irFilePath << " to " << objFilePath << std::endl;
        }
    }
    compiledFilesRecord[filePath] = currentHash;
}

void compileFile(const std::string& filePath,
                 const std::string& outputFilePath,
                 const std::string& ir_gc_map,
                 const std::string& objFilePath,
                 const std::string& buildDir,
                 const std::string& relativePath,
                 json& compiledFilesRecord,
                 const std::string& optimizationLevel) {
    std::string fileContent = readFileToString(filePath);
    std::hash<std::string> hasher;
    size_t currentHash = hasher(fileContent);

    std::cout << "Working on file: " << filePath << std::endl;
    if (filePath.ends_with(".gc")) {
        compileGcFile(filePath, outputFilePath, ir_gc_map, objFilePath, buildDir, relativePath, compiledFilesRecord, optimizationLevel, currentHash);
    } else if (filePath.ends_with(".c")) {
        compileCFile(filePath, outputFilePath, ir_gc_map, objFilePath, optimizationLevel, compiledFilesRecord, currentHash);
    } else if (filePath.ends_with(".rs")) {
        compileRustFile(filePath, outputFilePath, objFilePath, optimizationLevel, compiledFilesRecord, currentHash);
    } else {
        std::cerr << "Error: Unsupported file type for " << filePath << std::endl;
        exit(1);
    }
    std::cout << "Done Working on File: " << filePath << std::endl;
}

void compileDirectory(const std::string& srcDir, const std::string& buildDir, json& compiledFilesRecord, const std::string& optimizationLevel) {

    // update the ir_gc_map file
    for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
        if (entry.is_regular_file() && (entry.path().extension() == ".gc" || entry.path().extension() == ".c")) {
            setIrGcMap(entry.path().string(), buildDir + "/ir_gc_map/" + std::filesystem::relative(entry.path(), srcDir).string() + ".json", compiledFilesRecord);
        }
    }

    // Compile each .gc file in the src director
    for (const auto& entry : std::filesystem::recursive_directory_iterator(srcDir)) {
        if (entry.is_regular_file() && (entry.path().extension() == ".gc" || entry.path().extension() == ".c")) {
            std::string relativePath = std::filesystem::relative(entry.path(), srcDir).string();
            std::string outputFilePath = buildDir + "/ir/" + relativePath + ".ll";
            std::string ir_gc_map = buildDir + "/ir_gc_map/" + relativePath + ".json";
            std::string objFilePath = buildDir + "/obj/" + relativePath + ".o";
            std::filesystem::create_directories(std::filesystem::path(outputFilePath).parent_path());
            if (entry.path().extension() == ".gc") {
                std::vector<std::tuple<std::string, std::string, std::string, std::string, std::string>> filesRecord = {{entry.path().string(), outputFilePath, ir_gc_map, objFilePath, relativePath}};
                while (!filesRecord.empty()) {
                    try {
                        auto& fileTuple = filesRecord.back();
                        compileFile(std::get<0>(fileTuple),
                                    std::get<1>(fileTuple),
                                    std::get<2>(fileTuple),
                                    std::get<3>(fileTuple),
                                    buildDir,
                                    std::get<4>(fileTuple),
                                    compiledFilesRecord,
                                    optimizationLevel);
                        filesRecord.pop_back();
                    } catch (const compiler::NotCompiledError& e) {
                        auto gcFile = e.path;
                        std::string _relativePath = std::filesystem::relative(gcFile, srcDir).string();
                        std::string _outputFilePath = buildDir + "/ir/" + _relativePath + ".ll";
                        std::filesystem::create_directories(std::filesystem::path(_outputFilePath).parent_path());
                        std::string _ir_gc_map = buildDir + "/ir_gc_map/" + _relativePath + ".json";
                        std::string _objFilePath = buildDir + "/obj/" + _relativePath + ".o";
                        filesRecord.push_back({gcFile, _outputFilePath, _ir_gc_map, _objFilePath, _relativePath});
                    }
                }
            } else if (entry.path().extension() == ".c") {
                compileFile(entry.path().string(), outputFilePath, ir_gc_map, objFilePath, buildDir, relativePath, compiledFilesRecord, optimizationLevel);
                return;
            }
        }
    }
}

int checkAndSetEnvVars() {
    const char* gcStdDir = std::getenv("GC_STD_IRGCMAP");
    if (gcStdDir == nullptr) {
        std::cerr << "Warning: GC_STD_DIR environment variable is not set." << std::endl;
        return 1;
    }
    GC_STD_DIR = std::filesystem::path(gcStdDir ? gcStdDir : "");

    const char* gc_Std_IrGcMap = std::getenv("GC_STD_IRGCMAP");
    if (gc_Std_IrGcMap == nullptr) {
        std::cerr << "Warning: GC_STD_IRGCMAP environment variable is not set." << std::endl;
        return 1;
    }
    GC_STD_IRGCMAP = std::filesystem::path(gc_Std_IrGcMap ? gc_Std_IrGcMap : "");

    // const char* gcStdObj = std::getenv("GC_STD_OBJ");
    // if (gcStdObj == nullptr) {
    //     std::cerr << "Warning: GC_STD_OBJ environment variable is not set." << std::endl;
    //     return 1;
    // }

    return 0;
}

void setupCLI(CLI::App& app, std::string& inputFolderPath, std::string& optimizationLevel, std::string& executablePath) {
    app.add_option("input_folder", inputFolderPath, "Input folder path")->required();
    app.add_option("-O,--optimization", optimizationLevel, "Optimization level (O1, O2, O3, Os, Ofast)")->required(false);
    app.add_option("-o,--output", executablePath, "Output executable path")->required();
}

void createDirectories(const std::string& irDir, const std::string& irGcMapDir) {
    std::filesystem::create_directories(irDir);
    std::filesystem::create_directories(irGcMapDir);
}

json loadCompiledFilesRecord(const std::string& recordFilePath) {
    json compiledFilesRecord;
    if (std::filesystem::exists(recordFilePath)) {
        std::ifstream recordFile(recordFilePath);
        if (recordFile.is_open()) {
            recordFile >> compiledFilesRecord;
            recordFile.close();
        }
    }
    return compiledFilesRecord;
}

void saveCompiledFilesRecord(const json& compiledFilesRecord, const std::string& recordFilePath) {
    std::ofstream recordFile(recordFilePath, std::ios::trunc);
    if (recordFile.is_open()) {
        recordFile << compiledFilesRecord.dump(4);
        recordFile.close();
    }
}

int linkObjectFiles(const std::string& buildDir, const std::string& executablePath) {
    std::string objFiles;
    for (const auto& entry : std::filesystem::recursive_directory_iterator(buildDir + "/obj")) {
        if (entry.is_regular_file() && entry.path().extension() == ".o") { objFiles += entry.path().string() + " "; }
    }
    std::cout << objFiles << std::endl;
    std::string linkCommand = "clang " + objFiles + " -o " + executablePath;
    int linkResult = std::system(linkCommand.c_str());
    if (linkResult != 0) {
        std::cerr << "Error: Failed to link object files into executable " << executablePath << std::endl;
        return 1;
    } else {
        std::cout << "Successfully linked object files into executable " << executablePath << std::endl;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    if (checkAndSetEnvVars() != 0) { return 1; }

    CLI::App app{"Folder Compiler"};
    std::string inputFolderPath;
    std::string optimizationLevel;
    std::string executablePath;
    setupCLI(app, inputFolderPath, optimizationLevel, executablePath);
    CLI11_PARSE(app, argc, argv);

    std::string srcDir = inputFolderPath + "/src";
    std::string buildDir = inputFolderPath + "/build";
    std::string irDir = buildDir + "/ir";
    std::string irGcMapDir = buildDir + "/ir_gc_map";
    std::string recordFilePath = buildDir + "/compiled_files_record.json";

    createDirectories(irDir, irGcMapDir);

    json compiledFilesRecord = loadCompiledFilesRecord(recordFilePath);

    compileDirectory(srcDir, buildDir, compiledFilesRecord, optimizationLevel);

    saveCompiledFilesRecord(compiledFilesRecord, recordFilePath);

    return linkObjectFiles(buildDir, executablePath);
}
