#ifndef TOKENS_HPP
#define TOKENS_HPP
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <deque>
#include <string>
#include <vector>

#include "../config.hpp"

struct str {
    const char* string;
    uint32_t len;
    str(const char* string) : string(string), len(strlen(string)) {}
    str(const char* string, uint32_t len) : string(string), len(len) {}
};

namespace token {
enum struct TokenType : char {
    // Special Tokens
    EndOfFile, // End of file token
    Illegal,   // Illegal token
    Coment,    // Coment token `# ...`

    // Comparison Operators
    GreaterThan,        // Greater than '>'
    LessThan,           // Less than '<'
    GreaterThanOrEqual, // Greater than or equal to '>='
    LessThanOrEqual,    // Less than or equal to '<='
    EqualEqual,         // Equal to '=='
    NotEquals,          // Not equal to '!='

    // Data Types
    Identifier, // Identifier token [a-zA-Z_][a-zA-Z0-9_]*
    Integer,    // Integer token [0-9]+
    Float,      // Float token [0-9]+.[0-9]+
    StringSSQ,     // String token `'...'`
    StringDSQ,     // String token `"..."`
    StringSTQ,     // String token `'''...'''`
    StringDTQ,     // String token `"""..."""`

    // Assignment Operators
    PlusEqual,          // Addition assignment '+='
    DashEqual,          // Subtraction assignment '-='
    AsteriskEqual,      // Multiplication assignment '*='
    PercentEqual,       // Modulus assignment '%='
    CaretEqual,         // Exponentiation assignment '^='
    ForwardSlashEqual,  // Division assignment '/='
    BackwardSlashEqual, // Division assignment '\='
    Equals,             // Equals sign '='
    Is,                 // Is sign 'is'

    // Increment and Decrement Operators
    Increment, // Increment '++'
    Decrement, // Decrement '--'

    // Bitwise Operators
    BitwiseAnd, // Bitwise AND '&&'
    BitwiseOr,  // Bitwise OR '||'
    BitwiseXor, // Bitwise XOR '^'
    BitwiseNot, // Bitwise NOT '~'
    LeftShift,  // Left shift '<<'
    RightShift, // Right shift '>>'

    // Arithmetic Operators
    Dot,              // Dot '.'
    Ellipsis,         // Ellipsis '...'
    Plus,             // Addition '+'
    Dash,             // Subtraction '-'
    Asterisk,         // Multiplication '*'
    Percent,          // Modulus '%'
    AsteriskAsterisk, // Exponentiation '**'
    ForwardSlash,     // Division '/'
    BackwardSlash,    // Backward slash '\'
    Refrence,         // Reference '&'

    // Symbols
    LeftParen,    // Left Parenthesis '('
    RightParen,   // Right Parenthesis ')'
    LeftBrace,    // Left Brace '{'
    RightBrace,   // Right Brace '}'
    LeftBracket,  // Left Bracket '['
    RightBracket, // Right Bracket ']'
    Colon,        // Colon ':'
    Semicolon,    // Semicolon ';'
    RightArrow,   // Right Arrow '->'
    Comma,        // Comma ','
    AtTheRate,    // At symbol '@'
    Pipe,         // Pipe symbol '|'

    // Keywords
    And,      // Logical AND 'and'
    Or,       // Logical OR 'or'
    Not,      // Logical NOT 'not'
    Def,      // Function definition 'def'
    Return,   // Return statement 'return'
    If,       // If statement 'if'
    Else,     // Else statement 'else'
    ElIf,     // Else If statement 'elif'
    While,    // While statement 'while'
    For,      // For statement 'for'
    In,       // In statement 'in'
    Break,    // Break statement 'break'
    Continue, // Continue statement 'continue'
    Struct,   // Struct keyword 'struct'
    Enum,     // Enum keyword 'enum'
    Volatile, // Volatile keyword 'volatile'
    Const, // Constant keyword 'const'
    Use,      // Use keyword 'use'
    Import,   // Import keyword 'import'
    As,       // As keyword 'as'
    True,     // Boolean true 'true'
    False,    // Boolean false 'false'
    None,     // None type 'none'
    New,      // New keyword 'new'
    Try,      // Try keyword 'try'
    Catch,    // Catch keyword 'catch'
    Raise,    // Raise keyword 'raise'
    IfBreak,  // IfBreak keyword 'ifbreak'
    NotBreak, // NotBreak keyword 'notbreak'
    Switch,   // Switch keyword 'switch'
    Case,     // Case keyword 'case'
    Other,    // Other keyword 'other'
};

/**
 * @brief Convert TokenType to string for debugging.
 *
 * @param type The TokenType to convert.
 * @return std::string The string representation of the TokenType.
 */
std::string tokenTypeToString(TokenType type);

#define UINT24_MAX 1'67'77'215

struct Token {
    TokenType type = TokenType::Illegal;
    uint32_t pos : 24 = UINT24_MAX;

    inline Token() = default;
    inline Token(TokenType type, uint32_t pos) : type(type), pos(pos) {};

    uint32_t getStLineNo(str source) const;
    uint32_t getEnLineNo(str source) const;
    uint32_t getStColNo(str source) const;
    uint32_t getEnColNo(str source) const;
    uint32_t getEnPos(str source) const;

    const std::string getLiteral(str source) const;
    const std::string getIdentLiteral(str source) const;

    /**
     * @brief Convert the current token to a string.
     *
     * @param color If true, adds color to the string for CLI printing in ANSI format.
     */
    std::string toString(std::string source, bool color = true) const;

    void print(std::string source) const;
  private:
    static bool _isDigit(const char character) {
        return character >= '0' && character <= '9'; /* 0-9 in ansi is lied in one after the another*/
    };

    static bool _isHexDigit(const char character) {
        return (character >= '0' && character <= '9') || (character >= 'a' && character <= 'f') || (character >= 'A' && character <= 'F');
    };

    static bool _isLetter(const char character) {
        return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_'; /* a-z & A-Z in ansi is lied in one after the another*/
    };
};

struct Tokens {
    std::vector<Token> tokens = {};
    std::deque<Token> token_buffer = {};
    uint32_t current_token = 0;
    str source;
    Tokens() = delete;
    Tokens(str source) : source(source) {}

    void append(Token token) {
        tokens.push_back(token);
    };

    void append2buf(Token token) {
        token_buffer.push_back(token);
    };

    Token nextToken() {
        if (!token_buffer.empty()) {
            auto tok = token_buffer.back();
            token_buffer.pop_back();
            return tok;
        }
        if (current_token >= tokens.size()) return token::Token(TokenType::EndOfFile, UINT24_MAX);
        auto tok = tokens[current_token];
        current_token++;
        return tok;
    }
};

} // namespace token
#endif // TOKENS_HPP