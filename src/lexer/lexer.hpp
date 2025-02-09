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

    /**
     * @brief Construct a new Lexer object.
     *
     * @param source The source code to be lexed.
     */
    explicit Lexer(str source, const std::filesystem::path& file_path, bool tokenize_coment = false);

    token::Tokens Tokenize();

  private:
    /**
     * @brief Get the next token from the source code.
     *
     * @return The next token.
     */
    token::Token nextToken();

    /**
     * @brief Lookup the identifier type.
     *
     * @param ident The identifier to lookup.
     * @return The token type of the identifier.
     */
    token::TokenType _lookupIdent(str ident);

    /**
     * @brief Read the next character from the source code.
     */
    void _readChar();

    /**
     * @brief Peek at the character at the specified offset.
     *
     * @param offset The offset to peek at.
     * @return The character at the specified offset.
     */
    const char _peekChar(int offset = 1);

    /**
     * @brief Skip whitespace characters in the source code.
     */
    void _skipWhitespace();

    /**
     * @brief Create a new token.
     *
     * @param type The type of the token.
     * @param current_char The current character being processed.
     * @return The new token.
     */
    token::Token _newToken(token::TokenType type, uint32_t st_pos);

    /**
     * @brief Read a number from the source code.
     *
     * @return The number token.
     */
    token::Token _readNumber();

    /**
     * @brief Check if the character is a digit.
     *
     * @param character The character to check.
     * @return True if the character is a digit, false otherwise.
     */
    bool _isDigit(const char character);

    /**
     * @brief Check if the character is a letter.
     *
     * @param character The character to check.
     * @return True if the character is a letter, false otherwise.
     */
    bool _isLetter(const char character);

    QuoteType _isString();

    /**
     * @brief Read a string from the source code.
     *
     * @param quote The quote character used to delimit the string.
     * @return The string.
     */
    void _readString(QuoteType quote);
    // bool _isHexDigit(const char* character);
};
#endif