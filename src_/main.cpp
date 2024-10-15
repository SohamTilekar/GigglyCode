#include "compiler/compiler.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"
#include <fstream>
#include <iostream>
#include <llvm/IR/LegacyPassManager.h>
#include <string>


#define DEBUG_LEXER false
#define DEBUG_PARSER false
#define DEBUG_COMPILER true
#define DEBUG_LEXER_OUTPUT_PATH "./dump/lexer_output"
#define DEBUG_PARSER_OUTPUT_PATH "./dump/parser_output.json"
#define DEBUG_COMPILER_OUTPUT_PATH "./dump/compiler_output.ll"

int main(int argc, char* argv[]) {
    // Reading code
    if(argc != 5) {
        std::cout << "Invalid number of arguments. Usage: exe -f <file> -o <output>" << std::endl;
        return 1;
    }

    std::string input_file;
    std::string output_file;

    for(int i = 1; i < argc; i += 2) {
        std::string arg = argv[i];
        if(arg == "-f") {
            input_file = argv[i + 1];
        } else if(arg == "-o") {
            output_file = argv[i + 1];
        } else {
            std::cout << "Invalid argument: " << arg << std::endl;
            return 1;
        }
    }

    if(input_file.empty() || output_file.empty()) {
        std::cout << "Input file or output file not provided." << std::endl;
        return 1;
    }

    std::ifstream file(input_file);
    if(!file) {
        std::cout << "Unable to open input file: " << input_file << std::endl;
        return 1;
    }

    std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    // Lexer
    if(DEBUG_LEXER) {
        std::cout << "=========== Lexer Debug ===========" << std::endl;
        Lexer debug_lexer(file_content);
        if(!std::string(DEBUG_LEXER_OUTPUT_PATH).empty()) {
            std::ofstream file_c(DEBUG_LEXER_OUTPUT_PATH,
                                 std::ios::trunc); // Open file_c in truncate mode
            if(file_c.is_open()) {
                file_c << ""; // Clear file_c
                file_c.close();
            } else {
                std::cout << "Unable to open file_c";
            }
        }
        while(debug_lexer.current_char != "") {
            std::shared_ptr<token::Token> token = debug_lexer.nextToken();
            if(!std::string(DEBUG_LEXER_OUTPUT_PATH).empty()) {
                std::ofstream file(DEBUG_LEXER_OUTPUT_PATH,
                                   std::ios::app); // Open file in append mode
                if(file.is_open()) {
                    file << token->toString(false) << std::endl;
                    file.close();
                } else {
                    std::cout << "Unable to open file";
                }
            } else {
                std::cout << token->toString() << std::endl;
            }
        }
        if(!std::string(DEBUG_LEXER_OUTPUT_PATH).empty()) {
            std::cout << "Lexer output dumped to " << DEBUG_LEXER_OUTPUT_PATH << std::endl;
        }
    }
    Lexer l(file_content);
    parser::Parser p(std::make_shared<Lexer>(l));
    auto program = p.parseProgram();
    if(DEBUG_PARSER) {
        std::cout << "=========== Parser Debug ===========" << std::endl;
        if(!std::string(DEBUG_PARSER_OUTPUT_PATH).empty()) {
            std::ofstream file(DEBUG_PARSER_OUTPUT_PATH,
                               std::ios::trunc); // Open file in append mode
            if(file.is_open()) {
                file << program->toJSON()->dump(4) << std::endl;
                file.close();
            } else {
                std::cout << "Unable to open file";
            }
        } else {
            std::cout << program->toJSON()->dump(4, ' ', true, nlohmann::json::error_handler_t::replace);
        }
        for(auto& err : p.errors) {
            err->raise(false);
        }
        if(p.errors.size() > 0) {
            return 1;
        }
        if(!std::string(DEBUG_PARSER_OUTPUT_PATH).empty()) {
            std::cout << "Parser output dumped to " << DEBUG_PARSER_OUTPUT_PATH << std::endl;
        }
    } else {
        for(auto& err : p.errors) {
            err->raise(false);
        }
        if(p.errors.size() > 0) {
            return 1;
        }
    }

    compiler::Compiler c(file_content);
    // // Setting Up all The Target Triple and Data Layout
    // auto TargetTriple = llvm::sys::getDefaultTargetTriple();
    // // auto TargetTriple = "x86-pc-windows-msvc";
    // llvm::InitializeAllTargetInfos();
    // llvm::InitializeAllTargets();
    // llvm::InitializeAllTargetMCs();
    // llvm::InitializeAllAsmParsers();
    // llvm::InitializeAllAsmPrinters();
    // std::string Error;
    // auto Target = llvm::TargetRegistry::lookupTarget(TargetTriple, Error);
    // auto CPU = "generic";
    // auto Features = "";

    // llvm::TargetOptions opt;
    // auto TargetMachine = Target->createTargetMachine(TargetTriple, CPU, Features, opt, llvm::Reloc::PIC_);

    // // Print an error and exit if we couldn't find the requested target.
    // // This generally occurs if we've forgotten to initialise the
    // // TargetRegistry or we have a bogus target triple.
    // if(!Target) {
    //     llvm::errs() << Error;
    //     return 1;
    // }
    // c.llvm_module->setDataLayout(TargetMachine->createDataLayout());
    // c.llvm_module->setTargetTriple(TargetTriple);

    if(DEBUG_COMPILER) {
        std::cout << "=========== Compiler Debug ===========" << std::endl;
        c.compile(program);
        std::error_code EC;
        llvm::raw_fd_ostream file(DEBUG_COMPILER_OUTPUT_PATH, EC, llvm::sys::fs::OF_None);
        if(EC) {
            std::cerr << "Could not open file: " << EC.message() << std::endl;
            return 1;
        }
        c.llvm_module->print(file, nullptr);
        file.close();
        std::cout << "Compiler output dumped to " << DEBUG_COMPILER_OUTPUT_PATH << std::endl;
    }

    // std::error_code EC;
    // llvm::raw_fd_ostream dest(output_file, EC, llvm::sys::fs::OF_None);

    // if(EC) {
    //     llvm::errs() << "Could not open file: " << EC.message();
    //     return 1;
    // }

    // llvm::legacy::PassManager pass;
    // auto FileType = llvm::CodeGenFileType::ObjectFile;

    // if(TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
    //     llvm::errs() << "TargetMachine can't emit a file of this type";
    //     return 1;
    // }
    // pass.run(*c.llvm_module);
    // dest.flush();
    return 0;
}
