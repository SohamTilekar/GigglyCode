#ifndef LEXER_HPP
#define LEXER_HPP
#include "token.hpp"
#include <filesystem>

struct Lexer {
    const char* source;
    std::filesystem::path filePath;
    const uint32_t srcLen;
    uint32_t pos = -1;
    token::Tokens tokens;
    char currentChar = '\0'; ///< if \0 means EOF
    bool tokenizeComments;

    explicit Lexer(const char* source, const std::filesystem::path& file_path, bool tokenize_Comment = false);

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
    token::TokenType _lookupIdent(const char* ident, uint8_t ident_len);

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
    const char _peekChar(uint32_t offset = 1);

    /**
     * @brief Skip whitespace characters in the source code.
     */
    void _skipWhitespace();

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

    bool _isLetter(const char character);

    token::StringDelimiter _isString();

    std::string _readIdentifier();

    void _readString(token::StringDelimiter quote);
};
#endif