#ifndef LEXER_HPP
#define LEXER_HPP
#include "../config.hpp"
#include "token.hpp"
#include <filesystem>

enum struct QuoteType : char {
    None,
    SingleSingleQuote = int(token::TokenType::StringSSQ),
    DoubleSingleQuote = int(token::TokenType::StringDSQ),
    SingleTripleQuote = int(token::TokenType::StringSTQ),
    DoubleTripleQuote = int(token::TokenType::StringDTQ),
};

struct Lexer {
    str source;
    std::filesystem::path file_path;
    uint32_t pos = -1;
    char current_char = '\0';
    token::Tokens tokens;
    bool tokenize_coment;

    explicit Lexer(str source, const std::filesystem::path& file_path, bool tokenize_coment = false);

    token::Tokens Tokenize();

  private:
    token::Token nextToken();

    token::TokenType _lookupIdent(str ident);

    void _readChar();

    const char _peekChar(int offset = 1);

    void _skipWhitespace();

    token::Token _newToken(token::TokenType type, uint32_t st_pos);

    token::Token _readNumber();

    bool _isDigit(const char character);

    bool _isLetter(const char character);

    QuoteType _isString();

    void _readString(QuoteType quote);
};
#endif