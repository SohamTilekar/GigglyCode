#include "../errors/errors.hpp"
#include "lexer.hpp"

std::shared_ptr<std::string> Lexer::_isString() {
    if(this->current_char == "\"") {
        if(*this->_peekChar(1) == "\"") {
            if(*this->_peekChar(2) == "\"") {
                return std::make_shared<std::string>("\"\"\"");
            } else {
                return std::make_shared<std::string>("\"");
            }
        } else {
            return std::make_shared<std::string>("\"");
        }
    } else if(this->current_char == "'") {
        if(*this->_peekChar(1) == "'") {
            if(*this->_peekChar(2) == "'") {
                return std::make_shared<std::string>("'''");
            } else {
                return std::make_shared<std::string>("'");
            }
        } else {
            return std::make_shared<std::string>("'");
        }
    }
    return std::make_shared<std::string>("");
}

std::shared_ptr<std::string> Lexer::_readString(const std::string& quote) {
    std::string str = "";
    std::string literal = quote;
    if(quote == "\"\"\"" || quote == "'''") {
        this->_readChar();
        this->_readChar();
    }
    while(true) {
        this->_readChar();
        if(this->current_char == "") {
            errors::raiseSyntaxError(this->source, token::Token(token::TokenType::String, literal, this->line_no, this->col_no),
                                     "Unterminated string literal", "Add a closing " + quote + " to terminate the string literal");
        } else if(this->current_char == "\n" && quote != "\"\"\"" && quote != "'''") {
            errors::raiseSyntaxError(this->source, token::Token(token::TokenType::String, literal, this->line_no, this->col_no),
                                     "Unterminated string literal", "Add a closing " + quote + " to terminate the string literal");
        } else if(this->current_char == "\\") {
            this->_readChar();
            if(this->current_char == "\"") {
                str += "\"";
                literal += "\\\"";
            } else if(this->current_char == "'") {
                str += "'";
                literal += "\\'";
            } else if(this->current_char == "n") {
                str += "\n";
                literal += "\\n";
            } else if(this->current_char == "t") {
                str += "\t";
                literal += "\\t";
            } else if(this->current_char == "r") {
                str += "\r";
                literal += "\\r";
            } else if(this->current_char == "b") {
                str += "\b";
                literal += "\\b";
            } else if(this->current_char == "f") {
                str += "\f";
                literal += "\\f";
            } else if(this->current_char == "v") {
                str += "\v";
                literal += "\\v";
            } else if(this->current_char == "\\") {
                str += "\\";
                literal += "\\\\";
            } else {
                str += "\\" + this->current_char;
                literal += "\\" + this->current_char;
            }
        } else if((this->current_char == quote)) {
            this->_readChar();
            break;
        } else if(this->current_char + *this->_peekChar() + *this->_peekChar(2) == quote) {
            this->_readChar();
            this->_readChar();
            this->_readChar();
            break;
        } else {
            str += this->current_char;
            literal += this->current_char;
        }
    }
    return std::make_shared<std::string>(str);
}
