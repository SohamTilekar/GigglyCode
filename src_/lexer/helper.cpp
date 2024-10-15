#include "lexer.hpp"
#include <algorithm>
#include <sstream>

void Lexer::_readChar() {
    this->pos++;
    if(this->pos >= static_cast<int>(this->source.length())) {
        this->current_char = "";
    } else {
        this->current_char = this->source[this->pos];
    }
    this->col_no++;
};

std::shared_ptr<std::string> Lexer::_peekChar(int offset) {
    if((this->pos + offset) >= static_cast<int>(this->source.length())) {
        return std::make_shared<std::string>("");
    } else {
        return std::make_shared<std::string>(1, this->source[this->pos + offset]);
    }
}

std::shared_ptr<token::Token> Lexer::_newToken(token::TokenType type, std::string current_char) {
    auto x = std::make_shared<token::Token>(type, current_char, this->line_no, this->col_no);
    return x;
}

std::shared_ptr<token::Token> Lexer::_readNumber() {
    int dot_count = 0;
    std::string number = "";
    while(this->_isDigit(this->current_char) || this->current_char == ".") {
        if(this->current_char == ".") {
            dot_count++;
            if(dot_count > 1) {
                printf("Invalid number at line %u, column %i\n", this->line_no, this->col_no);
                return this->_newToken(token::TokenType::Illegal, this->current_char);
            }
        }
        number += this->current_char;
        this->_readChar();
        if(this->current_char == "")
            break;
    }
    if(dot_count == 0) {
        return this->_newToken(token::TokenType::Integer, number);
    }
    return this->_newToken(token::TokenType::Float, number);
};

std::shared_ptr<std::string> Lexer::_readIdentifier() {
    std::string identifier = "";
    while(this->_isLetter(this->current_char) || this->_isDigit(this->current_char)) {
        identifier += this->current_char;
        this->_readChar();
    };
    return std::make_shared<std::string>(identifier);
}

void Lexer::_skipWhitespace() {
    while(this->current_char == " " || this->current_char == "\t" || this->current_char == "\n" || this->current_char == "\r") {
        if(this->current_char == "\n") {
            this->line_no++;
            this->col_no = 0;
        }

        this->_readChar();
    }
    if(this->current_char == "#") {
        this->_readChar();
        while(this->current_char != "\n" && this->current_char != "") {
            this->_readChar();
        }
        this->_skipWhitespace();
    }
}

bool Lexer::_isDigit(const std::string& character) { return character >= "0" && character <= "9"; };


bool Lexer::_isLetter(const std::string& character) {
    return (character >= "a" && character <= "z") || (character >= "A" && character <= "Z") || character == "_";
};

std::string getStringOnLineNumber(const std::string& input_string, int line_number) {
    std::istringstream input(input_string);
    std::string line;
    for(int i = 0; std::getline(input, line); ++i) {
        if(i == line_number - 1) {
            return line; // Found the line
        } else if(i > line_number - 1) {
            break; // Line number not found
        }
    }
    return ""; // Line number not found
}

int getNumberOfLines(const std::string& str) { return std::count(str.begin(), str.end(), '\n') + 1; }
