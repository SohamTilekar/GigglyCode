#include "lexer.hpp"
#include "../errors/errors.hpp"
#include "token.hpp"

#include <cstdint>
#include <cstring>

using namespace token;

Lexer::Lexer(str source, const std::filesystem::path& file_path, bool tokenize_coment)
    : tokens(source), source(source), file_path(file_path), tokenize_coment(tokenize_coment) {
    pos = -1;
    current_char = '\0';
    _readChar();
}

using KW_TYPE = std::pair<const char*, TokenType>;
static constexpr KW_TYPE keywords_8[] = {
    {"continue", TokenType::Continue},
    {"volatile", TokenType::Volatile},
    {"notbreak", TokenType::NotBreak},
};
static constexpr KW_TYPE keywords_7[] = {
    {"ifbreak", TokenType::IfBreak}
};
static constexpr KW_TYPE keywords_6[] = {
    {"switch", TokenType::Switch},
    {"return", TokenType::Return},
    {"struct", TokenType::Struct},
    {"import", TokenType::Import},
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
TokenType lookupKeyword(str ident, const KW_TYPE(&kw_types)[kw_len]) {
    for (size_t i = 0; i < kw_len; ++i) {
        [[unlikely]]
        if (memcmp(ident.string, kw_types[i].first, len * sizeof(char)) == 0) {
            return kw_types[i].second;
        }
    }
    return TokenType::Identifier;
}

TokenType Lexer::_lookupIdent(str ident) {
    switch (ident.len) {
        case (2):
            return lookupKeyword<sizeof(keywords_2)/sizeof(KW_TYPE), 2>(ident, keywords_2);
        case (3):
            return lookupKeyword<sizeof(keywords_3)/sizeof(KW_TYPE), 3>(ident, keywords_3);
        case (4):
            return lookupKeyword<sizeof(keywords_4)/sizeof(KW_TYPE), 4>(ident, keywords_4);
        case (5):
            return lookupKeyword<sizeof(keywords_5)/sizeof(KW_TYPE), 5>(ident, keywords_5);
        case (6):
            return lookupKeyword<sizeof(keywords_6)/sizeof(KW_TYPE), 6>(ident, keywords_6);
        case (7):
            return lookupKeyword<sizeof(keywords_7)/sizeof(KW_TYPE), 7>(ident, keywords_7);
        case (8):
            return lookupKeyword<sizeof(keywords_8)/sizeof(KW_TYPE), 8>(ident, keywords_8);
        default:
            return TokenType::Identifier;
    }
};

Tokens Lexer::Tokenize() {
    while (this->current_char != '\0') {
        tokens.append(this->nextToken());
    }
    return this->tokens;
};

Token Lexer::nextToken() {
    Token token;
    this->_skipWhitespace();
    switch (current_char) {
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
                token = Token(TokenType::DashEqual, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::Dash, this->pos);
            break;
        case '*':
            if (_peekChar() == '=') {
                token = Token(TokenType::AsteriskEqual, this->pos);
                this->_readChar();
            } else [[unlikely]] if (_peekChar() == '*') {
                token = Token(TokenType::AsteriskAsterisk, this->pos);
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
                token = Token(TokenType::PercentEqual, this->pos);
                this->_readChar();
            } else
                token = Token(TokenType::Percent, this->pos);
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
                token = Token(TokenType::Refrence, this->pos);
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
            token = Token(TokenType::AtTheRate, this->pos);
            break;
        case '\0':
            token = Token(TokenType::EndOfFile, this->pos);
            break;
        default:
            [[unlikely]]
            if (auto quote = _isString(); quote != QuoteType::None) {
                uint32_t st_pos = this->pos + (quote == QuoteType::DoubleTripleQuote || quote == QuoteType::SingleTripleQuote ? 3 : 1);
                this->_readString(quote);
                token = Token(TokenType(quote), st_pos);
                return token;
            } else [[likely]] if (this->_isLetter(this->current_char)) {
                auto st_pos = this->pos;
                char ident_buffer[64];
                char* ident = ident_buffer;
                size_t ident_len = 0;
                [[likely]]
                while (this->_isLetter(this->current_char) || this->_isDigit(this->current_char)) {
                    ident[ident_len++] = this->current_char;
                    this->_readChar();
                    [[unlikely]]
                    if (ident_len == sizeof(ident_buffer)) {
                        errors::raiseSyntaxError(this->file_path, Token(TokenType::Identifier, st_pos), this->source.string, "Identifier too long", "Identifiers cannot exceed 64 characters.");
                    }
                };
                ident[ident_len] = '\0';
                token = Token(_lookupIdent(str(ident, ident_len)), st_pos);
                return token;
            } else [[likely]] if (this->_isDigit(this->current_char)) {
                token = this->_readNumber();
                return token;
            } else
                token = Token(TokenType::Illegal, this->pos);
    }
    this->_readChar(); // Move to next character after processing current token
    return token;
}

void Lexer::_readChar() {
    this->pos++;
    [[unlikely]]
    if (this->pos >= this->source.len)
        this->current_char = '\0';
    else
        this->current_char = this->source.string[this->pos];
}

const char Lexer::_peekChar(int offset) {
    uint32_t peek_pos = this->pos + offset;
    [[unlikely]]
    if (peek_pos >= this->source.len)
        return '\0';
    else
        return source.string[peek_pos];
}

Token Lexer::_newToken(TokenType type, uint32_t st_pos) {
    return Token(type, st_pos);
}

Token Lexer::_readNumber() {
    bool dot_count = false;
    uint32_t pos = this->pos;
    while (this->_isDigit(this->current_char) || this->current_char == '.') {
        if (this->current_char == '.') dot_count = true;
        this->_readChar();
    }
    if (dot_count) return this->_newToken(TokenType::Float, pos);
    else return this->_newToken(TokenType::Integer, pos);
};

void Lexer::_skipWhitespace() {
    [[likely]]
    while (
        this->current_char == ' '
        || this->current_char == '\t'
        || this->current_char == '\n'
        || this->current_char == '\r'
    ) { _readChar(); }

    [[unlikely]]
    if (this->current_char == '#' && !this->tokenize_coment) {
        this->_readChar();
        [[likely]]
        while (this->current_char != '\n' && this->current_char != '\0') { this->_readChar(); }
        this->_skipWhitespace(); // Recursive call to also skip the white space after the coment
    }
}

bool Lexer::_isDigit(const char character) {
    return character >= '0' && character <= '9'; /* 0-9 in ansi is lied in one after the another*/
};

bool Lexer::_isLetter(const char character) {
    return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_'; /* a-z & A-Z in ansi is lied in one after the another*/
};

QuoteType Lexer::_isString() {
    [[unlikely]]
    if (this->current_char == '\"') {
        [[unlikely]]
        if (this->_peekChar(1) == '\"') {
            [[unlikely]]
            if (this->_peekChar(2) == '\"') {
                return QuoteType::DoubleTripleQuote;
            } else {
                return QuoteType::DoubleSingleQuote;
            }
        } else {
            return QuoteType::DoubleSingleQuote;
        }
    } else [[unlikely]] if (this->current_char == '\'') {
        [[unlikely]]
        if (this->_peekChar(1) == '\'') {
            [[unlikely]]
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
    [[unlikely]]
    if (quote == QuoteType::DoubleTripleQuote || quote == QuoteType::SingleTripleQuote) {
        this->pos++;
        this->_readChar();
    }
    const char* quote_literal = quote == QuoteType::DoubleTripleQuote ? "\"\"\""
        : quote == QuoteType::SingleTripleQuote                       ? "'''"
        : quote == QuoteType::DoubleSingleQuote                             ? "\""
        : quote == QuoteType::SingleSingleQuote                             ? "'"
        : "";

    uint32_t st_pos = this->pos;
    [[likely]]
    while (true) {
        this->_readChar();
        [[unlikely]]
        if (this->current_char == '"') {
            [[likely]]
            if (quote == QuoteType::DoubleSingleQuote) {
                this->_readChar();
                break;
            } else if (quote == QuoteType::DoubleTripleQuote && _peekChar() == '"' && _peekChar(2) == '"') {
                this->pos += 2;
                _readChar();

                break;
            }
        }
        [[unlikely]]
        if (this->current_char == '\'') {
            [[likely]]
            if (quote == QuoteType::SingleSingleQuote) {
                this->_readChar();
                break;
            } else if (quote == QuoteType::SingleTripleQuote && _peekChar() == '"' && _peekChar(2) == '"') {
                this->pos += 2;
                _readChar();

                break;
            }
        }
        [[unlikely]]
        if ((this->current_char == '\0' || this->current_char == '\n') && (quote == QuoteType::DoubleSingleQuote || quote == QuoteType::SingleSingleQuote)) {
            errors::raiseSyntaxError(this->file_path,
                                     Token(TokenType(quote), st_pos),
                                     this->source.string,
                                     "Unterminated string literal",
                                     std::string("Add a closing ") + quote_literal + " to terminate the string literal");
        }

    }
}
