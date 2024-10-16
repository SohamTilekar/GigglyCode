#include <iostream>
#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <regex>
#include "include/cli11.hpp"
#include "lexer/lexer.hpp"
#include "parser/parser.hpp"

#define DEBUG_LEXER
#define DEBUG_PARSER
#define DEBUG_LEXER_OUTPUT_PATH "./dump/lexer_output.log"
#define DEBUG_PARSER_OUTPUT_PATH "./dump/parser_output.json"

// Function to read the file content into a string
std::string readFileToString(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::string basicCodeHighlighting(const std::string& code) {
    std::regex keywords("\\b(let|var|int|float)\\b");
    std::regex types("\\b(int|float|double|char|bool|void)\\b");
    std::regex numbers("\\b[0-9]+\\b");
    std::regex strings("\".*?\"");
    std::regex comments("//.*");
    std::regex operators("[+\\-*/=<>]");

    std::string highlightedCode = std::regex_replace(highlightedCode, numbers, "\033[1;32m$&\033[0m");
    highlightedCode = std::regex_replace(code, keywords, "\033[1;31m$&\033[0m");
    highlightedCode = std::regex_replace(highlightedCode, types, "\033[1;36m$&\033[0m");
    highlightedCode = std::regex_replace(highlightedCode, operators, "\033[1;35m$&\033[0m");
    highlightedCode = std::regex_replace(highlightedCode, strings, "\033[1;33m$&\033[0m");
    highlightedCode = std::regex_replace(highlightedCode, comments, "\033[1;34m$&\033[0m");

    return highlightedCode;
}

int main(int argc, char* argv[]) {
    CLI::App app{"File Reader and Writer"};

    std::string inputFilePath;
    std::string outputFilePath;

    app.add_option("input_file", inputFilePath, "Input file path")->required();
    app.add_option("-o,--output", outputFilePath, "Output file path")->required();

    CLI11_PARSE(app, argc, argv);

    // Read the input file content into a string
    std::string fileContent = readFileToString(inputFilePath);

    // Print the file content
    std::cout << "File Content:\n" << basicCodeHighlighting(fileContent) << std::endl;
#ifdef DEBUG_LEXER
    std::cout << "=========== Lexer Debug ===========" << std::endl;
    Lexer debug_lexer(fileContent);
    if (DEBUG_LEXER_OUTPUT_PATH != "") {
        std::ofstream debugOutput(DEBUG_LEXER_OUTPUT_PATH, std::ios::trunc);
        if (!debugOutput.is_open()) {
            std::cerr << "Error: Could not open debug output file " << DEBUG_LEXER_OUTPUT_PATH << std::endl;
            return 1;
        }
        while (debug_lexer.current_char != "") {
            std::shared_ptr<token::Token> token = debug_lexer.nextToken();
            debugOutput << token->toString(false) << std::endl;
        }
        debugOutput.close();
        std::cout << "Debug output written to " << DEBUG_LEXER_OUTPUT_PATH << std::endl;
    }
    else {
        while (debug_lexer.current_char != "") {
            std::shared_ptr<token::Token> token = debug_lexer.nextToken();
            std::cout << token->toString(true) << std::endl;
        }
    }
#endif
    Lexer lexer(fileContent);
#ifdef DEBUG_PARSER
    parser::Parser debug_parser(std::make_shared<Lexer>(lexer));
    auto program = debug_parser.parseProgram();
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
    for(auto& err : debug_parser.errors) {
        err->raise(false);
    }
    if(debug_parser.errors.size() > 0) {
        return 1;
    }
    if(!std::string(DEBUG_PARSER_OUTPUT_PATH).empty()) {
        std::cout << "Parser output dumped to " << DEBUG_PARSER_OUTPUT_PATH << std::endl;
    }
#endif
    // Write the file content to the output file
    std::cout << "Output File: " << outputFilePath << std::endl;
    return 0;
}
