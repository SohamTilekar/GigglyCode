#include "lexer.hpp"

#include "../errors/errors.hpp"
#include "token.hpp"

Lexer::Lexer(const std::string& source) {
    this->source = source;
    this->pos = -1;
    this->line_no = 1;
    this->col_no = -1;
    this->current_char = "";
    this->_readChar();
}

token::TokenType Lexer::_lookupIdent(std::shared_ptr<std::string> ident) {
    if(*ident == "and") {
        return token::TokenType::And;
    } else if(*ident == "or") {
        return token::TokenType::Or;
    } else if(*ident == "not") {
        return token::TokenType::Not;
    } else if(*ident == "def") {
        return token::TokenType::Def;
    } else if(*ident == "return") {
        return token::TokenType::Return;
    } else if(*ident == "if") {
        return token::TokenType::If;
    } else if(*ident == "else") {
        return token::TokenType::Else;
    } else if(*ident == "elif") {
        return token::TokenType::ElIf;
    } else if(*ident == "is") {
        return token::TokenType::Is;
    } else if(*ident == "while") {
        return token::TokenType::While;
    } else if(*ident == "for") {
        return token::TokenType::For;
    } else if(*ident == "in") {
        return token::TokenType::In;
    } else if(*ident == "break") {
        return token::TokenType::Break;
    } else if(*ident == "continue") {
        return token::TokenType::Continue;
    } else if(*ident == "struct") {
        return token::TokenType::Struct;
    } else if(*ident == "enum") {
        return token::TokenType::Enum;
    } else if(*ident == "volatile") {
        return token::TokenType::Volatile;
    } else if(*ident == "use") {
        return token::TokenType::Use;
    } else if(*ident == "import") {
        return token::TokenType::Import;
    } else if(*ident == "True") {
        return token::TokenType::True;
    } else if(*ident == "False") {
        return token::TokenType::False;
        // } else if(*ident == "MayBe") {
        //     return token::TokenType::Maybe;
    } else if(*ident == "None") {
        return token::TokenType::None;
    }
    return token::TokenType::Identifier;
};

std::shared_ptr<token::Token> Lexer::nextToken() {
    std::shared_ptr<token::Token> token;

    this->_skipWhitespace();

    if(this->current_char == "+") {
        if(*this->_peekChar() == "+") {
            token = this->_newToken(token::TokenType::Increment, this->current_char + *this->_peekChar());
            this->_readChar();
        } else if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::PlusEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Plus, this->current_char);
        }
    } else if(this->current_char == ".") {
        if(*this->_peekChar() == "." && *this->_peekChar(2) == ".") {
            token = this->_newToken(token::TokenType::Ellipsis, this->current_char + *this->_peekChar() + *(this->_peekChar(2)));
            this->_readChar();
            this->_readChar();
        } else
            token = this->_newToken(token::TokenType::Dot, this->current_char);
    } else if(this->current_char == "-") {
        if(*this->_peekChar() == ">") {
            token = this->_newToken(token::TokenType::RightArrow, this->current_char + *this->_peekChar());
            this->_readChar();
        } else if(*this->_peekChar() == "-") {
            token = this->_newToken(token::TokenType::Decrement, this->current_char + *this->_peekChar());
            this->_readChar();
        } else if(this->_isDigit(*(this->_peekChar()))) {
            this->_readChar();
            token = this->_readNumber();
            token->literal = "-" + token->literal;
            return token;
        } else if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::DashEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Dash, this->current_char);
        }
    } else if(this->current_char == "*") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::AsteriskEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else if(*this->_peekChar() == "*") {
            token = this->_newToken(token::TokenType::AsteriskAsterisk, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Asterisk, this->current_char);
        }
    } else if(this->current_char == "/") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::ForwardSlashEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::ForwardSlash, this->current_char);
        }
    } else if(this->current_char == "%") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::PercentEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Percent, this->current_char);
        }
    } else if(this->current_char == "^") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::CaretEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::BitwiseXor, this->current_char);
        }
    } else if(this->current_char == "=") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::EqualEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Equals, this->current_char);
        }
    } else if(this->current_char == ">") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::GreaterThanOrEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else if(*this->_peekChar() == ">") {
            token = this->_newToken(token::TokenType::RightShift, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::GreaterThan, this->current_char);
        }
    } else if(this->current_char == "<") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::LessThanOrEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else if(*this->_peekChar() == "<") {
            token = this->_newToken(token::TokenType::LeftShift, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::LessThan, this->current_char);
        }
    } else if(this->current_char == "!") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::NotEquals, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Illegal, this->current_char);
        }
    } else if(this->current_char == "{") {
        token = this->_newToken(token::TokenType::LeftBrace, this->current_char);
    } else if(this->current_char == "}") {
        token = this->_newToken(token::TokenType::RightBrace, this->current_char);
    } else if(this->current_char == "(") {
        token = this->_newToken(token::TokenType::LeftParen, this->current_char);
    } else if(this->current_char == ")") {
        token = this->_newToken(token::TokenType::RightParen, this->current_char);
    } else if(this->current_char == "[") {
        token = this->_newToken(token::TokenType::LeftBracket, this->current_char);
    } else if(this->current_char == "]") {
        token = this->_newToken(token::TokenType::RightBracket, this->current_char);
    } else if(this->current_char == ":") {
        token = this->_newToken(token::TokenType::Colon, this->current_char);
    } else if(this->current_char == ";") {
        token = this->_newToken(token::TokenType::Semicolon, this->current_char);
    } else if(this->current_char == "&") {
        if(*this->_peekChar() == "&") {
            token = this->_newToken(token::TokenType::BitwiseAnd, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Illegal, this->current_char);
        }
    } else if(this->current_char == "|") {
        if(*this->_peekChar() == "|") {
            token = this->_newToken(token::TokenType::BitwiseOr, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Illegal, this->current_char);
        }
    } else if(this->current_char == "~") {
        token = this->_newToken(token::TokenType::BitwiseNot, this->current_char);
    } else if(this->current_char == ",") {
        token = this->_newToken(token::TokenType::Comma, this->current_char);
    } else if(this->current_char == "") {
        token = this->_newToken(token::TokenType::EndOfFile, "");
    } else {
        if(*this->_isString() != "") {
            std::shared_ptr<std::string> str = this->_readString(*this->_isString());
            token = this->_newToken(token::TokenType::String, *str);
            return token;
        } else if(this->_isLetter(this->current_char)) {
            std::shared_ptr<std::string> ident = this->_readIdentifier();
            token = this->_newToken(this->_lookupIdent(ident), *ident);
            return token;
        } else if(this->_isDigit(this->current_char)) {
            token = this->_readNumber();
            return token;
        } else {
            token = this->_newToken(token::TokenType::Illegal, this->current_char);
        }
    }
    this->_readChar();
    return token;
}

#include <algorithm>
#include <sstream>

#include "lexer.hpp"

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


bool Lexer::_isLetter(const std::string& character) { return (character >= "a" && character <= "z") || (character >= "A" && character <= "Z") || character == "_"; };

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
            errors::raiseSyntaxError("Invalid Str", this->source, token::Token(token::TokenType::String, literal, this->line_no, this->col_no), "Unterminated string literal",
                                     "Add a closing " + quote + " to terminate the string literal");
        } else if(this->current_char == "\n" && quote != "\"\"\"" && quote != "'''") {
            errors::raiseSyntaxError("Invalid Str", this->source, token::Token(token::TokenType::String, literal, this->line_no, this->col_no), "Unterminated string literal",
                                     "Add a closing " + quote + " to terminate the string literal");
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
