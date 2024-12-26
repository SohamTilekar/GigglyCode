#include <algorithm>
#include <sstream>

#include "../errors/errors.hpp"
#include "lexer.hpp"
#include "token.hpp"

Lexer::Lexer(const std::string& source) {
    this->source = source;
    // Calling the `_readChar` will incremnt the `pos` & `col_no`
    pos = -1;
    line_no = 1;
    col_no = -1;
    current_char = "";
    _readChar();
}

token::TokenType Lexer::_lookupIdent(const std::string ident) {
    if (ident == "and") {
        return token::TokenType::And;
    } else if (ident == "or") {
        return token::TokenType::Or;
    } else if (ident == "not") {
        return token::TokenType::Not;
    } else if (ident == "def") {
        return token::TokenType::Def;
    } else if (ident == "return") {
        return token::TokenType::Return;
    } else if (ident == "if") {
        return token::TokenType::If;
    } else if (ident == "else") {
        return token::TokenType::Else;
    } else if (ident == "elif") {
        return token::TokenType::ElIf;
    } else if (ident == "is") {
        return token::TokenType::Is;
    } else if (ident == "while") {
        return token::TokenType::While;
    } else if (ident == "for") {
        return token::TokenType::For;
    } else if (ident == "in") {
        return token::TokenType::In;
    } else if (ident == "break") {
        return token::TokenType::Break;
    } else if (ident == "continue") {
        return token::TokenType::Continue;
    } else if (ident == "struct") {
        return token::TokenType::Struct;
    } else if (ident == "enum") {
        return token::TokenType::Enum;
    } else if (ident == "volatile") {
        return token::TokenType::Volatile;
    } else if (ident == "use") {
        return token::TokenType::Use;
    } else if (ident == "import") {
        return token::TokenType::Import;
    } else if (ident == "True") {
        return token::TokenType::True;
    } else if (ident == "False") {
        return token::TokenType::False;
        // } else if(ident == "MayBe") {
        //     return token::TokenType::Maybe;
    } else if (ident == "None") {
        return token::TokenType::None;
    } else if (ident == "new") {
        return token::TokenType::New;
    } else if (ident == "try") {
        return token::TokenType::Try;
    } else if (ident == "catch") {
        return token::TokenType::Catch;
    } else if (ident == "raise") {
        return token::TokenType::Raise;
    } else if (ident == "notbreak") {
        return token::TokenType::NotBreak;
    } else if (ident == "ifbreak") {
        return token::TokenType::IfBreak;
    } else if (ident == "switch") {
        return token::TokenType::Switch;
    } else if (ident == "case") {
        return token::TokenType::Case;
    } else if (ident == "other") {
        return token::TokenType::Other;
    }

    return token::TokenType::Identifier;
};

token::Token Lexer::nextToken() {
    token::Token token;

    this->_skipWhitespace();

    if (this->current_char == "+") {
        if (this->_peekChar() == "+") {
            token = this->_newToken(token::TokenType::Increment, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '++'
        } else if (this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::PlusEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '+='
        } else {
            token = this->_newToken(token::TokenType::Plus, this->current_char);
        }
    } else if (this->current_char == ".") {
        if (this->_peekChar() == "." && this->_peekChar(2) == ".") {
            token = this->_newToken(token::TokenType::Ellipsis, this->current_char + this->_peekChar() + this->_peekChar(2));
            this->_readChar(); // Move to next character after '..'
            this->_readChar(); // Move to next character after '...'
        } else {
            token = this->_newToken(token::TokenType::Dot, this->current_char);
        }
    } else if (this->current_char == "-") {
        if (this->_peekChar() == ">") {
            token = this->_newToken(token::TokenType::RightArrow, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '->'
        } else if (this->_peekChar() == "-") {
            token = this->_newToken(token::TokenType::Decrement, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '--'
        } else if (this->_isDigit(this->_peekChar())) {
            this->_readChar(); // Move to next character after '-'
            token = this->_readNumber();
            token.literal = "-" + token.literal;
            return token;
        } else if (this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::DashEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '-='
        } else {
            token = this->_newToken(token::TokenType::Dash, this->current_char);
        }
    } else if (this->current_char == "*") {
        if (this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::AsteriskEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '*='
        } else if (this->_peekChar() == "*") {
            token = this->_newToken(token::TokenType::AsteriskAsterisk, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '**'
        } else {
            token = this->_newToken(token::TokenType::Asterisk, this->current_char);
        }
    } else if (this->current_char == "/") {
        if (this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::ForwardSlashEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '/='
        } else {
            token = this->_newToken(token::TokenType::ForwardSlash, this->current_char);
        }
    } else if (this->current_char == "%") {
        if (this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::PercentEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '%='
        } else {
            token = this->_newToken(token::TokenType::Percent, this->current_char);
        }
    } else if (this->current_char == "^") {
        if (this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::CaretEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '^='
        } else {
            token = this->_newToken(token::TokenType::BitwiseXor, this->current_char);
        }
    } else if (this->current_char == "=") {
        if (this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::EqualEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '=='
        } else {
            token = this->_newToken(token::TokenType::Equals, this->current_char);
        }
    } else if (this->current_char == ">") {
        if (this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::GreaterThanOrEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '>='
        } else if (this->_peekChar() == ">") {
            token = this->_newToken(token::TokenType::RightShift, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '>>'
        } else {
            token = this->_newToken(token::TokenType::GreaterThan, this->current_char);
        }
    } else if (this->current_char == "<") {
        if (this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::LessThanOrEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '<='
        } else if (this->_peekChar() == "<") {
            token = this->_newToken(token::TokenType::LeftShift, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '<<'
        } else {
            token = this->_newToken(token::TokenType::LessThan, this->current_char);
        }
    } else if (this->current_char == "!") {
        if (this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::NotEquals, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '!='
        } else {
            token = this->_newToken(token::TokenType::Illegal, this->current_char);
        }
    } else if (this->current_char == "{") {
        token = this->_newToken(token::TokenType::LeftBrace, this->current_char);
    } else if (this->current_char == "}") {
        token = this->_newToken(token::TokenType::RightBrace, this->current_char);
    } else if (this->current_char == "(") {
        token = this->_newToken(token::TokenType::LeftParen, this->current_char);
    } else if (this->current_char == ")") {
        token = this->_newToken(token::TokenType::RightParen, this->current_char);
    } else if (this->current_char == "[") {
        token = this->_newToken(token::TokenType::LeftBracket, this->current_char);
    } else if (this->current_char == "]") {
        token = this->_newToken(token::TokenType::RightBracket, this->current_char);
    } else if (this->current_char == ":") {
        token = this->_newToken(token::TokenType::Colon, this->current_char);
    } else if (this->current_char == ";") {
        token = this->_newToken(token::TokenType::Semicolon, this->current_char);
    } else if (this->current_char == "&") {
        if (this->_peekChar() == "&") {
            token = this->_newToken(token::TokenType::BitwiseAnd, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '&&'
        } else {
            token = this->_newToken(token::TokenType::Refrence, this->current_char);
        }
    } else if (this->current_char == "|") {
        if (this->_peekChar() == "|") {
            token = this->_newToken(token::TokenType::BitwiseOr, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '||'
        } else {
            token = this->_newToken(token::TokenType::Pipe, this->current_char);
        }
    } else if (this->current_char == "~") {
        token = this->_newToken(token::TokenType::BitwiseNot, this->current_char);
    } else if (this->current_char == ",") {
        token = this->_newToken(token::TokenType::Comma, this->current_char);
    } else if (this->current_char == "@") {
        token = this->_newToken(token::TokenType::AtTheRate, this->current_char);
    } else if (this->current_char == "") {
        token = this->_newToken(token::TokenType::EndOfFile, "");
    } else {
        if (this->_isString() != "") {
            std::string str = this->_readString(this->_isString());
            token = this->_newToken(token::TokenType::String, str);
            return token;
        } else if (this->_isLetter(this->current_char)) {
            std::string ident = this->_readIdentifier();
            token = this->_newToken(this->_lookupIdent(ident), ident);
            return token;
        } else if (this->_isDigit(this->current_char)) {
            token = this->_readNumber();
            return token;
        } else {
            token = this->_newToken(token::TokenType::Illegal, this->current_char);
        }
    }

    this->_readChar(); // Move to next character after processing current token
    return token;
}

void Lexer::_readChar() {
    this->pos++;
    if (this->pos >= static_cast<int>(this->source.length())) {
        // Set current_char to empty string to tell EOF
        this->current_char = "";
    } else {
        this->current_char = this->source[this->pos];
    }
    this->col_no++;
}

std::string Lexer::_peekChar(int offset) {
    int peek_pos = this->pos + offset;
    if (peek_pos >= static_cast<int>(this->source.length())) {
        // Return an empty string to tell EOF
        return "";
    } else {
        return std::string(1, source[peek_pos]);
    }
}

token::Token Lexer::_newToken(token::TokenType type, std::string currentChar) {
    return token::Token(type, currentChar, line_no, col_no);
}

token::Token Lexer::_readNumber() {
    int dot_count = 0;
    std::string number = "";
    while (this->_isDigit(this->current_char) || this->current_char == ".") {
        if (this->current_char == ".") {
            dot_count++;
            // Check for multiple dots in the number
            if (dot_count > 1) {
                printf("Invalid number at line %u, column %i\n", this->line_no, this->col_no);
                return this->_newToken(token::TokenType::Illegal, this->current_char);
            }
        }
        number += this->current_char;
        this->_readChar();
        if (this->current_char == "") break;
    }
    if (dot_count == 0) { return this->_newToken(token::TokenType::Integer, number); }
    return this->_newToken(token::TokenType::Float, number);
};

std::string Lexer::_readIdentifier() {
    std::string identifier = "";
    while (this->_isLetter(this->current_char) || this->_isDigit(this->current_char)) {
        identifier += this->current_char;
        this->_readChar();
    };
    return identifier;
}

void Lexer::_skipWhitespace() {
    // Skip whitespace characters
    while (this->current_char == " " || this->current_char == "\t" || this->current_char == "\n" || this->current_char == "\r") {
        if (this->current_char == "\n") {
            this->line_no++;
            this->col_no = 0;
        }
        this->_readChar();
    }
    // Skip comments starting with #
    if (this->current_char == "#") {
        this->_readChar();
        while (this->current_char != "\n" && this->current_char != "") { this->_readChar(); }
        this->_skipWhitespace(); // Recursive call to also skip the white space
    }
}

bool Lexer::_isDigit(const std::string& character) {
    return character >= "0" && character <= "9"; /* 0-9 in ansi is lied in one after the another*/
};

bool Lexer::_isLetter(const std::string& character) {
    return (character >= "a" && character <= "z") || (character >= "A" && character <= "Z") || character == "_"; /* a-z & A-Z in ansi is lied in one after the another*/
};

std::string getStringOnLineNumber(const std::string& input_string, int line_number) {
    std::istringstream input(input_string);
    std::string line;
    // Iterate through the lines to find the specified line number
    for (int i = 0; std::getline(input, line); ++i) {
        if (i == line_number - 1) {
            return line; // Found the line
        } else if (i > line_number - 1) {
            return ""; // Line not found
        }
    }
    return ""; // Line not found
}

int getNumberOfLines(const std::string& str) {
    return std::count(str.begin(), str.end(), '\n') + 1;
}

std::string Lexer::_isString() {
    // Check for double or single quotes to identify string literals
    if (this->current_char == "\"") {
        if (this->_peekChar(1) == "\"") {
            if (this->_peekChar(2) == "\"") {
                return "\"\"\"";
            } else {
                return "\"";
            }
        } else {
            return "\"";
        }
    } else if (this->current_char == "'") {
        if (this->_peekChar(1) == "'") {
            if (this->_peekChar(2) == "'") {
                return "'''";
            } else {
                return "'";
            }
        } else {
            return "'";
        }
    }
    return "";
}

std::string Lexer::_readString(const std::string& quote) {
    std::string str = "";
    std::string literal = quote;
    // Handle triple quotes
    if (quote == "\"\"\"" || quote == "'''") {
        this->_readChar();
        this->_readChar();
    }
    while (true) {
        this->_readChar();
        // Handle unterminated string literals
        if (this->current_char == "") {
            errors::raiseSyntaxError("Invalid Str",
                                     this->source,
                                     token::Token(token::TokenType::String, literal, this->line_no, this->col_no),
                                     "Unterminated string literal",
                                     "Add a closing " + quote + " to terminate the string literal");
        } else if (this->current_char == "\n" && quote != "\"\"\"" && quote != "'''") {
            errors::raiseSyntaxError("Invalid Str",
                                     this->source,
                                     token::Token(token::TokenType::String, literal, this->line_no, this->col_no),
                                     "Unterminated string literal",
                                     "Add a closing " + quote + " to terminate the string literal");
        } else if (this->current_char == "\\") {
            this->_readChar();
            // Handle escape sequences
            if (this->current_char == "\"") {
                str += "\"";
                literal += "\\\"";
            } else if (this->current_char == "'") {
                str += "'";
                literal += "\\'";
            } else if (this->current_char == "n") {
                str += "\n";
                literal += "\\n";
            } else if (this->current_char == "t") {
                str += "\t";
                literal += "\\t";
            } else if (this->current_char == "r") {
                str += "\r";
                literal += "\\r";
            } else if (this->current_char == "b") {
                str += "\b";
                literal += "\\b";
            } else if (this->current_char == "f") {
                str += "\f";
                literal += "\\f";
            } else if (this->current_char == "v") {
                str += "\v";
                literal += "\\v";
            } else if (this->current_char == "\\") {
                str += "\\";
                literal += "\\\\";
            } else {
                str += "\\" + this->current_char;
                literal += "\\" + this->current_char;
            }
        } else if ((this->current_char == quote)) {
            this->_readChar();
            break;
        } else if (this->current_char + this->_peekChar() + this->_peekChar(2) == quote) {
            this->_readChar();
            this->_readChar();
            this->_readChar();
            break;
        } else {
            str += this->current_char;
            literal += this->current_char;
        }
    }
    return str;
}
