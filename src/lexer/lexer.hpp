/**
 * @file lexer.hpp
 * @brief This file contains the declaration of the Lexer class and related
 * functions.
 *
 * The Lexer class is responsible for tokenizing the source code. It reads the
 * source code character by character and generates tokens that represent the
 * different elements of the code.
 *
 * Functions:
 * - getStringOnLineNumber: Get the string on a specific line number from the
 * input string.
 * - getNumberOfLines: Get the number of lines in the input string.
 *
 * Class Lexer:
 * - Members:
 *   - source: The source code to be lexed.
 *   - pos: The current position in the source code.
 *   - line_no: The current line number in the source code.
 *   - col_no: The current column number in the source code.
 *   - current_char: The current character being processed.
 * - Methods:
 *   - Lexer: Constructor to initialize the Lexer with the source code.
 *   - nextToken: Get the next token from the source code.
 *   - _lookupIdent: Lookup the identifier type.
 *   - _readChar: Read the next character from the source code.
 *   - _peekChar: Peek at the character at the specified offset.
 *   - _skipWhitespace: Skip whitespace characters in the source code.
 *   - _newToken: Create a new token.
 *   - _readNumber: Read a number from the source code.
 *   - _isDigit: Check if the character is a digit.
 *   - _isLetter: Check if the character is a letter.
 *   - _isString: Check if the current character is a string.
 *   - _readIdentifier: Read an identifier from the source code.
 *   - _readString: Read a string from the source code.
 */
#ifndef LEXER_HPP
#define LEXER_HPP
#include "token.hpp"
#include <filesystem>
#include <stack>
#include <vector>

/**
 * @brief Get the string on a specific line number from the input string.
 *
 * @param input_string The input string.
 * @param line_number The line number to retrieve.
 * @return The string on the specified line number.
 */
std::string getStringOnLineNumber(const std::string& input_string, int line_number);

/**
 * @brief Get the number of lines in the input string.
 *
 * @param str The input string.
 * @return The number of lines in the input string.
 */
int getNumberOfLines(const std::string& str);

class Lexer {
  public:
    std::string source; ///< The source code to be lexed.
    std::filesystem::path file_path;
    int pos;                  ///< The current position in the source code.
    unsigned int line_no;     ///< The current line number in the source code.
    int col_no;               ///< The current column number in the source code.
    std::string current_char; ///< The current character being processed.
    bool tokenize_coment;
    std::vector<token::Token> tokenBuffer;

    /**
     * @brief Construct a new Lexer object.
     *
     * @param source The source code to be lexed.
     */
    explicit Lexer(const std::string& source, const std::filesystem::path& file_path, bool tokenize_coment = false);

    /**
     * @brief Get the next token from the source code.
     *
     * @return The next token.
     */
    token::Token nextToken();

  private:
    /**
     * @brief Lookup the identifier type.
     *
     * @param ident The identifier to lookup.
     * @return The token type of the identifier.
     */
    token::TokenType _lookupIdent(const std::string ident);

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
    std::string _peekChar(int offset = 1);

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
    token::Token _newToken(token::TokenType type, std::string current_char);

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
    bool _isDigit(const std::string& character);

    /**
     * @brief Check if the character is a letter.
     *
     * @param character The character to check.
     * @return True if the character is a letter, false otherwise.
     */
    bool _isLetter(const std::string& character);

    /**
     * @brief Check if the current character is a string.
     *
     * @return The string if the current character is a string.
     */
    std::string _isString();

    /**
     * @brief Read an identifier from the source code.
     *
     * @return The identifier.
     */
    std::string _readIdentifier();

    /**
     * @brief Read a string from the source code.
     *
     * @param quote The quote character used to delimit the string.
     * @return The string.
     */
    std::string _readString(const std::string& quote);
    bool _isHexDigit(const std::string& character);
};
#endif