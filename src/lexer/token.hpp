#ifndef TOKENS_HPP
#define TOKENS_HPP
#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace token {

enum struct TokenType : char {
    EndOfFile, // End of file
    Illegal,   // Illegal token
    Comment,    // `# ...`

    // Comparison Operators
    GreaterThan,        // `>`
    LessThan,           // `<`
    GreaterThanOrEqual, // `>=`
    LessThanOrEqual,    // `<=`
    EqualEqual,         // `==`
    NotEquals,          // `!=`

    // Data Types
    Identifier, // [a-zA-Z_][a-zA-Z0-9_]*
    Integer,    // [0-9]*
    Float,      // [0-9]*.[0-9]*
    StringSSQ,  // `'...'`
    StringDSQ,  // `"..."`
    StringSTQ,  // `'''...'''`
    StringDTQ,  // `"""..."""`

    // Assignment Operators
    PlusEqual,          // `+=`
    MinusEqual,          // `-=`
    AsteriskEqual,      // `*=`
    ModuloEqual,       // `%=`
    CaretEqual,         // `^=`
    ForwardSlashEqual,  // `/=`
    BackSlashEqual, // `\=`
    Equals,             // `=`
    Is,                 // `is`

    // post fix operators
    Increment, // Increment `++`
    Decrement, // Decrement `--`

    // Bitwise Operators
    BitwiseAnd, // Bitwise AND `&&`
    BitwiseOr,  // Bitwise OR  `||`
    BitwiseXor, // Bitwise XOR `^`
    BitwiseNot, // Bitwise NOT `~`
    LeftShift,  // Left shift  `<<`
    RightShift, // Right shift `>>`

    // Arithmetic Operators
    Dot,              // `.`
    Ellipsis,         // `...`
    Plus,             // `+`
    Minus,             // `-`
    Asterisk,         // `*`
    Modulo,          // `%`
    Exponent, // `**`
    ForwardSlash,     // `/`
    BackSlash,    // `\`
    Ampersand,         // `&`

    // Symbols
    LeftParen,    // `(`
    RightParen,   // `)`
    LeftBrace,    // `{`
    RightBrace,   // `}`
    LeftBracket,  // `[`
    RightBracket, // `]`
    Colon,        // `:`
    Semicolon,    // `;`
    RightArrow,   // `->`
    Comma,        // `,`
    At,    // `@`
    Pipe,         // `|`

    // Keywords
    And,      // 'and'
    Or,       // 'or'
    Not,      // 'not'
    Def,      // 'def'
    Return,   // 'return'
    If,       // 'if'
    Else,     // 'else'
    ElIf,     // 'elif'
    While,    // 'while'
    For,      // 'for'
    In,       // 'in'
    Break,    // 'break'
    Continue, // 'continue'
    Struct,   // 'struct'
    Enum,     // 'enum'
    Volatile, // 'volatile'
    Const,    // 'const'
    Use,      // 'use'
    Import,   // 'import'
    As,       // 'as'
    True,     // 'true'
    False,    // 'false'
    None,     // 'none'
    New,      // 'new'
    Try,      // 'try'
    Catch,    // 'catch'
    Raise,    // 'raise'
    IfBreak,  // 'ifbreak'
    NotBreak, // 'notbreak'
    Switch,   // 'switch'
    Case,     // 'case'
    Other,    // 'other'
};

enum struct StringDelimiter : char {
    None,
    SingleQuote = int(TokenType::StringSSQ),
    DoubleQuote,
    TripleSingleQuote,
    TripleDoubleQuote,
};

/**
 * @brief Convert TokenType to string for debugging.
 *
 * @param type The TokenType to convert.
 * @return std::string The string representation of the TokenType.
 */
const std::string tokenTypeToString(TokenType type);

#define UINT24_MAX 1'67'77'215

struct Token {
    TokenType type = TokenType::Illegal;
    uint32_t pos : 24 = UINT24_MAX; //https://en.cppreference.com/w/cpp/language/bit_field https://en.cppreference.com/w/cpp/language/bit_field

    inline Token() = default;
    inline Token(TokenType type, uint32_t pos) : type(type), pos(pos) {};

    uint32_t getStLineNo(const char* source) const;
    uint32_t getEnLineNo(const char* source) const;
    uint32_t getStColNo(const char* source) const;
    uint32_t getEnColNo(const char* source) const;
    uint32_t getEnPos(const char* source) const;

    const std::string getLiteral(const char* source) const;

    /**
     * @brief Convert the current token to a string.
     *
     * @param color If true, adds color to the string for CLI printing in ANSI format.
     */
    const std::string toString(std::string source, bool color = true) const;

    void print(std::string source) const;

  private:
    static bool _isDigit(const char character) {
        return character >= '0' && character <= '9'; // 0-9 in ansi is lied in one after the another
    };

    static bool _isLetter(const char character) {
        return (character >= 'a' && character <= 'z') || (character >= 'A' && character <= 'Z') || character == '_'; // a-z & A-Z in ansi is lie in one after the another
    };
};

struct Tokens {
    std::vector<Token> tokens = {};
    std::deque<Token> tokenBuffer = {};
    const char* source;
    uint32_t currentTokenIDX = 0;

    explicit Tokens(const char* source) : source(source) {}

    void append(Token token) { this->tokens.push_back(token); };

    void append2buf(Token token) { tokenBuffer.push_back(token); };

    Token nextToken();
};

} // namespace token
#endif // TOKENS_HPP
