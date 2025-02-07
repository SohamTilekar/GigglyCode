#include "lexer.hpp"
#include "../errors/errors.hpp"
#include "token.hpp"

#include <cstring>
#include <sstream>

Lexer::Lexer(const char* source, const std::filesystem::path& file_path, bool tokenize_coment) : tokens(source), source_len(strlen(source)) {
    this->source = source;
    this->file_path = file_path;
    this->tokenize_coment = tokenize_coment;
    // Calling the `_readChar` will incremnt the `pos` & `col_no`
    pos = -1;
    current_char = '\0';
    _readChar();
}

token::TokenType Lexer::_lookupIdent(const char* ident) {
    static const std::unordered_map<std::string, token::TokenType> keywords = {
        {"and", token::TokenType::And},
        {"or", token::TokenType::Or},
        {"not", token::TokenType::Not},
        {"def", token::TokenType::Def},
        {"return", token::TokenType::Return},
        {"if", token::TokenType::If},
        {"else", token::TokenType::Else},
        {"elif", token::TokenType::ElIf},
        {"is", token::TokenType::Is},
        {"while", token::TokenType::While},
        {"for", token::TokenType::For},
        {"in", token::TokenType::In},
        {"break", token::TokenType::Break},
        {"continue", token::TokenType::Continue},
        {"struct", token::TokenType::Struct},
        {"enum", token::TokenType::Enum},
        {"volatile", token::TokenType::Volatile},
        {"const", token::TokenType::Const},
        {"use", token::TokenType::Use},
        {"import", token::TokenType::Import},
        {"as", token::TokenType::As},
        {"True", token::TokenType::True},
        {"False", token::TokenType::False},
        {"None", token::TokenType::None},
        {"new", token::TokenType::New},
        {"try", token::TokenType::Try},
        {"catch", token::TokenType::Catch},
        {"raise", token::TokenType::Raise},
        {"notbreak", token::TokenType::NotBreak},
        {"ifbreak", token::TokenType::IfBreak},
        {"switch", token::TokenType::Switch},
        {"case", token::TokenType::Case},
        {"other", token::TokenType::Other},
    };
    auto it = keywords.find(ident);
    return (it == keywords.end()) ? token::TokenType::Identifier : it->second;
};

token::Tokens Lexer::Tokenize() {
    while (this->current_char != '\0') {
        tokens.append(this->nextToken());
    }
    return this->tokens;
};

token::Token Lexer::nextToken() {
    token::Token token;
    this->_skipWhitespace();

    if (this->current_char == '+') {
        if (this->_peekChar() == '+') {
            token = this->_newToken(token::TokenType::Increment, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '++'
        } else if (this->_peekChar() == '=') {
            token = this->_newToken(token::TokenType::PlusEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '+='
        } else {
            token = this->_newToken(token::TokenType::Plus, this->current_char);
        }
    } else if (this->current_char == '.') {
        if (this->_peekChar() == '.' && this->_peekChar(2) == '.') {
            token = this->_newToken(token::TokenType::Ellipsis, this->current_char + this->_peekChar() + this->_peekChar(2));
            this->_readChar(); // Move to next character after '..'
            this->_readChar(); // Move to next character after '...'
        } else {
            token = this->_newToken(token::TokenType::Dot, this->current_char);
        }
    } else if (this->current_char == '-') {
        if (this->_peekChar() == '>') {
            token = this->_newToken(token::TokenType::RightArrow, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '->'
        } else if (this->_peekChar() == '-') {
            token = this->_newToken(token::TokenType::Decrement, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '--'
        } else if (this->_isDigit(this->_peekChar())) {
            int st_pos = this->pos;
            this->_readChar(); // Move to next character after '-'
            token = this->_readNumber();
            token.pos = st_pos;
            return token;
        } else if (this->_peekChar() == '=') {
            token = this->_newToken(token::TokenType::DashEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '-='
        } else {
            token = this->_newToken(token::TokenType::Dash, this->current_char);
        }
    } else if (this->current_char == '*') {
        if (this->_peekChar() == '=') {
            token = this->_newToken(token::TokenType::AsteriskEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '*='
        } else if (this->_peekChar() == '*') {
            token = this->_newToken(token::TokenType::AsteriskAsterisk, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '**'
        } else {
            token = this->_newToken(token::TokenType::Asterisk, this->current_char);
        }
    } else if (this->current_char == '/') {
        if (this->_peekChar() == '=') {
            token = this->_newToken(token::TokenType::ForwardSlashEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '/='
        } else {
            token = this->_newToken(token::TokenType::ForwardSlash, this->current_char);
        }
    } else if (this->current_char == '%') {
        if (this->_peekChar() == '=') {
            token = this->_newToken(token::TokenType::PercentEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '%='
        } else {
            token = this->_newToken(token::TokenType::Percent, this->current_char);
        }
    } else if (this->current_char == '^') {
        if (this->_peekChar() == '=') {
            token = this->_newToken(token::TokenType::CaretEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '^='
        } else {
            token = this->_newToken(token::TokenType::BitwiseXor, this->current_char);
        }
    } else if (this->current_char == '=') {
        if (this->_peekChar() == '=') {
            token = this->_newToken(token::TokenType::EqualEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '=='
        } else {
            token = this->_newToken(token::TokenType::Equals, this->current_char);
        }
    } else if (this->current_char == '>') {
        if (this->_peekChar() == '=') {
            token = this->_newToken(token::TokenType::GreaterThanOrEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '>='
        } else if (this->_peekChar() == '>') {
            token = this->_newToken(token::TokenType::RightShift, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '>>'
        } else {
            token = this->_newToken(token::TokenType::GreaterThan, this->current_char);
        }
    } else if (this->current_char == '<') {
        if (this->_peekChar() == '=') {
            token = this->_newToken(token::TokenType::LessThanOrEqual, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '<='
        } else if (this->_peekChar() == '<') {
            token = this->_newToken(token::TokenType::LeftShift, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '<<'
        } else {
            token = this->_newToken(token::TokenType::LessThan, this->current_char);
        }
    } else if (this->current_char == '!') {
        if (this->_peekChar() == '=') {
            token = this->_newToken(token::TokenType::NotEquals, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '!='
        } else {
            token = this->_newToken(token::TokenType::Illegal, this->current_char);
        }
    } else if (this->current_char == '{') {
        token = this->_newToken(token::TokenType::LeftBrace, this->current_char);
    } else if (this->current_char == '}') {
        token = this->_newToken(token::TokenType::RightBrace, this->current_char);
    } else if (this->current_char == '(') {
        token = this->_newToken(token::TokenType::LeftParen, this->current_char);
    } else if (this->current_char == ')') {
        token = this->_newToken(token::TokenType::RightParen, this->current_char);
    } else if (this->current_char == '[') {
        token = this->_newToken(token::TokenType::LeftBracket, this->current_char);
    } else if (this->current_char == ']') {
        token = this->_newToken(token::TokenType::RightBracket, this->current_char);
    } else if (this->current_char == ':') {
        token = this->_newToken(token::TokenType::Colon, this->current_char);
    } else if (this->current_char == ';') {
        token = this->_newToken(token::TokenType::Semicolon, this->current_char);
    } else if (this->current_char == '&') {
        if (this->_peekChar() == '&') {
            token = this->_newToken(token::TokenType::BitwiseAnd, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '&&'
        } else {
            token = this->_newToken(token::TokenType::Refrence, this->current_char);
        }
    } else if (this->current_char == '|') {
        if (this->_peekChar() == '|') {
            token = this->_newToken(token::TokenType::BitwiseOr, this->current_char + this->_peekChar());
            this->_readChar(); // Move to next character after '||'
        } else {
            token = this->_newToken(token::TokenType::Pipe, this->current_char);
        }
    } else if (this->current_char == '~') {
        token = this->_newToken(token::TokenType::BitwiseNot, this->current_char);
    } else if (this->current_char == ',') {
        token = this->_newToken(token::TokenType::Comma, this->current_char);
    } else if (this->current_char == '@') {
        token = this->_newToken(token::TokenType::AtTheRate, this->current_char);
    } else if (this->current_char == '\0') {
        token = this->_newToken(token::TokenType::EndOfFile, '\0');
    } else if (this->current_char == '#' && tokenize_coment) {
        int st_pos = this->pos;
        this->_readChar();
        while (this->current_char != '\n' && this->current_char != '\0') {
            this->_readChar();
        }
        return token::Token(token::TokenType::Coment, st_pos);
    } else {
        if (auto quote = this->_isString(); quote != QuoteType::None) {
            int st_pos = this->pos + (quote == QuoteType::DoubleTripleQuote || quote == QuoteType::SingleTripleQuote ? 3 : 1);
            this->_readString(quote);
            token = token::Token(token::TokenType(quote), st_pos);
            return token;
        } else if (this->_isLetter(this->current_char)) {
            auto st_pos = this->pos;
            auto ident = this->_readIdentifier();
            token = this->_newToken(this->_lookupIdent(ident.c_str()), st_pos);
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
    if (this->pos >= this->source_len) {
        // Set current_char to empty string to tell EOF
        this->current_char = '\0';
    } else {
        this->current_char = this->source[this->pos];
    }
}

const char Lexer::_peekChar(int offset) {
    int peek_pos = this->pos + offset;
    if (peek_pos >= this->source_len) {
        return '\0';
    } else {
        return source[peek_pos];
    }
}

token::Token Lexer::_newToken(token::TokenType type, uint32_t st_pos) {
    return token::Token(type, st_pos);
}

token::Token Lexer::_readNumber() {
    bool dot_count = false;
    uint32_t pos = this->pos;
    while (this->_isDigit(this->current_char) || this->current_char == '.') {
        if (this->current_char == '.') dot_count = true;
        this->_readChar();
    }
    if (dot_count) return this->_newToken(token::TokenType::Float, pos);
    else return this->_newToken(token::TokenType::Integer, pos);
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
    while (this->current_char == ' ' || this->current_char == '\t' || this->current_char == '\n' || this->current_char == '\r') {
        this->_readChar();
    }
    // Skip comments starting with #
    if (this->current_char == '#' && !tokenize_coment) {
        this->_readChar();
        while (this->current_char != '\n' && this->current_char != '\0') { this->_readChar(); }
        this->_skipWhitespace(); // Recursive call to also skip the white space
    }
}

bool Lexer::_isDigit(const char character) {
    return character >= '0' && character <= '9'; /* 0-9 in ansi is lied in one after the another*/
};

bool Lexer::_isLetter(const char character) {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_'; /* a-z & A-Z in ansi is lied in one after the another*/
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

QuoteType Lexer::_isString() {
    if (this->current_char == '\"') {
        if (this->_peekChar(1) == '\"') {
            if (this->_peekChar(2) == '\"') {
                return QuoteType::DoubleTripleQuote;
            } else {
                return QuoteType::DoubleSingleQuote;
            }
        } else {
            return QuoteType::DoubleSingleQuote;
        }
    } else if (this->current_char == '\'') {
        if (this->_peekChar(1) == '\'') {
            if (this->_peekChar(2) == '\'') {
                return QuoteType::DoubleTripleQuote;
            } else {
                return QuoteType::DoubleSingleQuote;
            }
        } else {
            return QuoteType::DoubleSingleQuote;
        }
    }
    return QuoteType::None;
}

void Lexer::_readString(QuoteType quote) {
    if (quote == QuoteType::DoubleTripleQuote || quote == QuoteType::SingleTripleQuote) {
        this->_readChar();
        this->_readChar();
    }
    const char* quote_literal =
        quote == QuoteType::DoubleTripleQuote ? "\"\"\""
            : quote == QuoteType::SingleTripleQuote
                ? "'''" : quote == QuoteType::DoubleSingleQuote
                    ? "\"" : quote == QuoteType::SingleSingleQuote
                        ? "'" : "";
    auto st_pos = this->pos;
    while (true) {
        this->_readChar();
        if (this->current_char == '"') {
            if (quote == QuoteType::DoubleSingleQuote) {
                this->_readChar();
                break;
            } else if(quote == QuoteType::DoubleTripleQuote && _peekChar() == '"' && _peekChar(2) == '"') {
                this->_readChar();
                this->_readChar();
                this->_readChar();
                break;
            }
        } else if (this->current_char == '\'') {
            if (quote == QuoteType::SingleSingleQuote) {
                this->_readChar();
                break;
            } else if(quote == QuoteType::SingleTripleQuote && _peekChar() == '"' && _peekChar(2) == '"') {
                this->_readChar();
                this->_readChar();
                this->_readChar();
                break;
            }
        } else if (this->current_char == '\\')
            this->_readChar();
        if ((this->current_char == '\0' || this->current_char == '\n') && (quote == QuoteType::DoubleSingleQuote || quote == QuoteType::SingleSingleQuote)) {
            errors::raiseSyntaxError(this->file_path,
                                     token::Token(token::TokenType(quote), st_pos),
                                     this->source,
                                     "Unterminated string literal",
                                     std::string("Add a closing ") + quote_literal + " to terminate the string literal");
        }
    }
}
