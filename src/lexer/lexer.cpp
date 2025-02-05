#include "lexer.hpp"
#include "../errors/errors.hpp"
#include "token.hpp"

#include <cstring>

using namespace token;

Lexer::Lexer(const char* source, const std::filesystem::path& file_path, bool tokenize_Comment)
    : source(source), filePath(file_path), tokens(source), srcLen(strlen(source)), tokenizeComments(tokenize_Comment) {
    _readChar();
}

struct KW_TYPE {
    const char* str;
    TokenType type;
};

static constexpr KW_TYPE keywords_8[] = {
    {"continue", TokenType::Continue},
    {"volatile", TokenType::Volatile},
    {"notbreak", TokenType::NotBreak},
    {nullptr, TokenType::Illegal}, //add a null terminator
};

static constexpr KW_TYPE keywords_7[] = {
    {"ifbreak", TokenType::IfBreak}
};

static constexpr KW_TYPE keywords_6[] = {
    {"switch", TokenType::Switch},
    {"return", TokenType::Return},
    {"struct", TokenType::Struct},
    {"import", TokenType::Import},
    {nullptr, TokenType::Illegal}, //add a null terminator
};

static constexpr KW_TYPE keywords_5[] = {
    {"while", TokenType::While},
    {"break", TokenType::Break},
    {"const", TokenType::Const},
    {"False", TokenType::False},
    {"catch", TokenType::Catch},
    {"raise", TokenType::Raise},
    {"other", TokenType::Other},
};

static constexpr KW_TYPE keywords_4[] = {
    {"else", TokenType::Else},
    {"elif", TokenType::ElIf},
    {"enum", TokenType::Enum},
    {"True", TokenType::True},
    {"None", TokenType::None},
    {"case", TokenType::Case},
};

static constexpr KW_TYPE keywords_3[] = {
    {"and", TokenType::And},
    {"not", TokenType::Not},
    {"def", TokenType::Def},
    {"for", TokenType::For},
    {"use", TokenType::Use},
    {"new", TokenType::New},
    {"try", TokenType::Try},
};

static constexpr KW_TYPE keywords_2[] = {
    {"or", TokenType::Or},
    {"if", TokenType::If},
    {"is", TokenType::Is},
    {"in", TokenType::In},
    {"as", TokenType::As},
};

template <size_t kw_len, size_t len>
TokenType lookupKeyword(const char* ident, const KW_TYPE(&kw_types)[kw_len]) {
    for (size_t i = 0; i < kw_len; ++i) {
        [[unlikely]]
        if (strncmp(ident, kw_types[i].str, len) == 0) {
            return kw_types[i].type;
        }
    }
    return TokenType::Identifier;
}

TokenType Lexer::_lookupIdent(const char* ident, uint8_t ident_len) {
    switch (ident_len) {
        case (2):
            return lookupKeyword<5, 2>(ident, keywords_2);
        case (3):
            return lookupKeyword<7, 3>(ident, keywords_3);
        case (4):
            return lookupKeyword<6, 4>(ident, keywords_4);
        case (5):
            return lookupKeyword<7, 5>(ident, keywords_5);
        case (6):
            return lookupKeyword<5, 6>(ident, keywords_6);
        case (7):
            return lookupKeyword<1, 7>(ident, keywords_7);
        case (8):
            return lookupKeyword<4, 8>(ident, keywords_8);
        default:
            return TokenType::Identifier;
    }
};

Tokens Lexer::Tokenize() {
    [[likely]]
    while (currentChar != '\0') { this->tokens.append(nextToken()); }
    return this->tokens;
};

Token Lexer::nextToken() {
    Token token;
    this->_skipWhitespace();

    switch (currentChar) {
        case '+':
            if (_peekChar() == '+') {
                token = Token(TokenType::Increment, this->pos);
                this->_readChar();
            } else if (_peekChar() == '=') {
                token = Token(TokenType::PlusEqual, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::Plus, this->pos);
            break;
        case '.':
            [[unlikely]]
            if (_peekChar() == '.' && _peekChar(2) == '.') {
                token = Token(TokenType::Ellipsis, this->pos);
                this->pos += 1;
                this->_readChar();
            } else
                token = Token(TokenType::Dot, this->pos);
            break;
        case '-':
            if (_peekChar() == '>') {
                token = Token(TokenType::RightArrow, this->pos);
                this->_readChar();
            } else if (_peekChar() == '-') {
                token = Token(TokenType::Decrement, this->pos);
                this->_readChar();
            } else if (this->_isDigit(_peekChar())) {
                uint32_t st_pos = this->pos;
                this->_readChar();
                token = this->_readNumber();
                token.pos = st_pos;
                return token;
            } else if (_peekChar() == '=') {
                token = Token(TokenType::MinusEqual, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::Minus, this->pos);
            break;
        case '*':
            if (_peekChar() == '=') {
                token = Token(TokenType::AsteriskEqual, this->pos);
                this->_readChar();
            } else [[unlikely]] if (_peekChar() == '*') {
                token = Token(TokenType::Exponent, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::Asterisk, this->pos);
            break;
        case '/':
            if (_peekChar() == '=') {
                token = Token(TokenType::ForwardSlashEqual, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::ForwardSlash, this->pos);
            break;
        case '%':
            if (_peekChar() == '=') {
                token = Token(TokenType::ModuloEqual, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::Modulo, this->pos);
            break;
        case '^':
            if (_peekChar() == '=') {
                token = Token(TokenType::CaretEqual, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::BitwiseXor, this->pos);
            break;
        case '=':
            if (_peekChar() == '=') {
                token = Token(TokenType::EqualEqual, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::Equals, this->pos);
            break;
        case '>':
            if (_peekChar() == '=') {
                token = Token(TokenType::GreaterThanOrEqual, this->pos);
                this->_readChar();
            } else [[unlikely]] if (_peekChar() == '>') {
                token = Token(TokenType::RightShift, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::GreaterThan, this->pos);
            break;
        case '<':
            if (_peekChar() == '=') {
                token = Token(TokenType::LessThanOrEqual, this->pos);
                this->_readChar();
            } else [[unlikely]] if (_peekChar() == '<') {
                token = Token(TokenType::LeftShift, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::LessThan, this->pos);
            break;
        case '!':
            [[likely]]
            if (_peekChar() == '=') {
                token = Token(TokenType::NotEquals, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::Illegal, this->pos);
            break;
        case '{':
            token = Token(TokenType::LeftBrace, this->pos);
            break;
        case '}':
            token = Token(TokenType::RightBrace, this->pos);
            break;
        case '(':
            token = Token(TokenType::LeftParen, this->pos);
            break;
        case ')':
            token = Token(TokenType::RightParen, this->pos);
            break;
        case '[':
            token = Token(TokenType::LeftBracket, this->pos);
            break;
        case ']':
            token = Token(TokenType::RightBracket, this->pos);
            break;
        case ':':
            token = Token(TokenType::Colon, this->pos);
            break;
        case ';':
            token = Token(TokenType::Semicolon, this->pos);
            break;
        case '&':
            [[unlikely]]
            if (_peekChar() == '&') {
                token = Token(TokenType::BitwiseAnd, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::Ampersand, this->pos);
            break;
        case '|':
            [[likely]]
            if (_peekChar() == '|') {
                token = Token(TokenType::BitwiseOr, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::Pipe, this->pos);
            break;
        case '~':
            token = Token(TokenType::BitwiseNot, this->pos);
            break;
        case ',':
            token = Token(TokenType::Comma, this->pos);
            break;
        case '@':
            token = Token(TokenType::At, this->pos);
            break;
        case '\0':
            token = Token(TokenType::EndOfFile, this->pos);
            break;
        default:
            [[unlikely]]
            if (auto quote = _isString(); quote != StringDelimiter::None) {
                uint32_t st_pos = this->pos + (quote == StringDelimiter::TripleDoubleQuote || quote == StringDelimiter::TripleSingleQuote ? 3 : 1);
                this->_readString(quote);
                token = Token(TokenType(quote), st_pos);
                return token;
            } else [[likely]] if (this->_isLetter(this->currentChar)) {
                auto st_pos = this->pos;
                char ident_buffer[64];
                char* ident = ident_buffer;
                size_t ident_len = 0;
                [[likely]]
                while (this->_isLetter(this->currentChar) || this->_isDigit(this->currentChar)) {
                    ident[ident_len++] = this->currentChar;
                    this->_readChar();
                    [[unlikely]]
                    if (ident_len == sizeof(ident_buffer)) {
                        errors::raiseSyntaxError(this->filePath, Token(TokenType::Identifier, st_pos), this->source, "Identifier too long", "Identifiers cannot exceed 64 characters.");
                    }
                };
                ident[ident_len] = '\0';
                token = Token(_lookupIdent(ident, ident_len), st_pos);
                return token;
            } else [[likely]] if (this->_isDigit(this->currentChar)) {
                token = this->_readNumber();
                return token;
            } else
                token = Token(TokenType::Illegal, this->pos);
    }
    this->_readChar();
    return token;
}

void Lexer::_readChar() {
    this->pos++;
    [[unlikely]]
    if (this->pos >= this->srcLen)
        this->currentChar = '\0';
    else this->currentChar = this->source[this->pos];
}

const char Lexer::_peekChar(uint32_t offset) {
    uint32_t peek_pos = this->pos + offset;
    [[unlikely]]
    if (peek_pos >= this->srcLen)
        return '\0';
    else
        return source[peek_pos];
}

Token Lexer::_readNumber() {
    bool dot_count = false;
    uint32_t pos = this->pos;
    [[likely]]
    while (this->_isDigit(this->currentChar) || this->currentChar == '.') {
        [[unlikely]]
        if (this->currentChar == '.') dot_count = true;
        this->_readChar();
    }
    if (dot_count) return Token(TokenType::Float, pos);
    else return Token(TokenType::Integer, pos);
};

void Lexer::_skipWhitespace() {
    [[likely]]
    while (
        this->currentChar == ' '
        || this->currentChar == '\t'
        || this->currentChar == '\n'
        || this->currentChar == '\r'
    ) { _readChar(); }

    [[unlikely]]
    if (this->currentChar == '#' && !tokenizeComments) {
        this->_readChar();
        [[likely]]
        while (this->currentChar != '\n' && this->currentChar != '\0') { this->_readChar(); }
        this->_skipWhitespace(); // Recursive call to also skip the white space after the coment
    }
}

bool Lexer::_isDigit(const char character) {
    return character >= '0' && character <= '9'; // 0-9 in ansi is lied in one after the another
};

bool Lexer::_isLetter(const char character) {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_'; // a-z & A-Z in ansi is lied in one after the another
};

StringDelimiter Lexer::_isString() {
    [[unlikely]]
    if (this->currentChar == '"') {
        [[unlikely]]
        if (_peekChar(1) == '"') {
            [[unlikely]]
            if (_peekChar(2) == '"') {
                return StringDelimiter::TripleDoubleQuote;
            } else {
                return StringDelimiter::DoubleQuote;
            }
        } else {
            return StringDelimiter::DoubleQuote;
        }
    } else [[unlikely]] if (this->currentChar == '\'') {
        [[unlikely]]
        if (_peekChar(1) == '\'') {
            [[unlikely]]
            if (_peekChar(2) == '\'') {
                return StringDelimiter::TripleDoubleQuote;
            } else {
                return StringDelimiter::DoubleQuote;
            }
        } else {
            return StringDelimiter::DoubleQuote;
        }
    }
    return StringDelimiter::None;
}

void Lexer::_readString(StringDelimiter quote) {
    [[unlikely]]
    if (quote == StringDelimiter::TripleDoubleQuote || quote == StringDelimiter::TripleSingleQuote) {
        this->pos++;
        this->_readChar();
    }
    const char* quote_literal = quote == StringDelimiter::TripleDoubleQuote ? "\"\"\""
        : quote == StringDelimiter::TripleSingleQuote                       ? "'''"
        : quote == StringDelimiter::DoubleQuote                             ? "\""
        : quote == StringDelimiter::SingleQuote                             ? "'"
        : "";

    uint32_t st_pos = this->pos;
    [[likely]]
    while (true) {
        this->_readChar();
        [[unlikely]]
        if (this->currentChar == '"') {
            [[likely]]
            if (quote == StringDelimiter::DoubleQuote) {
                this->_readChar();
                break;
            } else if (quote == StringDelimiter::TripleDoubleQuote && _peekChar() == '"' && _peekChar(2) == '"') {
                this->pos += 2;
                _readChar();
                break;
            }
        }
        [[unlikely]]
        if (this->currentChar == '\'') {
            [[likely]]
            if (quote == StringDelimiter::SingleQuote) {
                this->_readChar();
                break;
            } else if (quote == StringDelimiter::TripleSingleQuote && _peekChar() == '"' && _peekChar(2) == '"') {
                this->pos += 2;
                _readChar();
                break;
            }
        }
        [[unlikely]]
        if ((this->currentChar == '\0' || this->currentChar == '\n') && (quote == StringDelimiter::DoubleQuote || quote == StringDelimiter::SingleQuote)) {
            errors::raiseSyntaxError(this->filePath,
                                     Token(TokenType(quote), st_pos),
                                     this->source,
                                     "Unterminated string literal",
                                     std::string("Add a closing ") + quote_literal + " to terminate the string literal");
        }
    }
}
