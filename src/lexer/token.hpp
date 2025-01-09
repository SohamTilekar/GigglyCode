/**
 * @file token.hpp
 * @brief This file contains the definition of tokens used in the lexer.
 *
 * It includes the TokenType enum class which represents different types of tokens,
 * and the Token class which represents a token in the source code.
 * Additionally, it provides utility functions for converting TokenType to string
 * and printing tokens.
 */
#ifndef TOKENS_HPP
#define TOKENS_HPP
#include <string>

namespace token {

/**
 * @brief Enum class representing different types of tokens.
 */
enum class TokenType {
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
    String,     // String token "[\"'].*[\"']" | "'''[^']*'''" | '"""[^"]*"""'
    RawString,  // Raw String token "r[\"'].*[\"']" | "R[\"'].*[\"']" |
                // "r'''[^']*'''" | "R'''[^']*'''" | '"""[^"]*"""' |
                // 'R"""[^"]*"""'

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
std::string tokenTypeString(TokenType type);

/**
 * @brief Class representing a token in the source code.
 */
class Token {
  public:
    TokenType type;      ///< The type of the token.
    std::string literal; ///< The literal value of the token.
    int st_line_no;         ///< The line number where the token is located.
    int end_line_no;     ///< The line number where the token is located.
    int end_col_no;      ///< The ending column number of the token.
    int col_no;          ///< The starting column number of the token.

    /**
     * @brief Default constructor for Token.
     */
    inline Token() {};

    /**
     * @brief Constructor for Token with type, line number, and column number.
     *
     * @param type The type of the token.
     * @param lineNo The line number where the token is located.
     * @param colNo The column number where the token starts.
     */
    inline Token(TokenType type, int lineNo, int colNo, int endColNo) : type(type), st_line_no(lineNo), end_line_no(lineNo), end_col_no(endColNo), col_no(colNo) {};

    /**
     * @brief Constructor for Token with type, literal, line number, and column number.
     *
     * @param type The type of the token.
     * @param literal The literal value of the token.
     * @param lineNo The line number where the token is located.
     * @param colNo The column number where the token starts.
     */
    inline Token(TokenType type, std::string literal, int stLineNo, int lineNo, int colNo, int endColNo) : type(type), literal(literal), st_line_no(stLineNo), end_line_no(lineNo), end_col_no(endColNo), col_no(colNo) {};

    /**
     * @brief Convert the current token to a string.
     *
     * @param color If true, adds color to the string for CLI printing.
     * @return std::string The string representation of the token.
     */
    std::string toString(bool color = true);

    /**
     * @brief Print the current token to the CLI.
     */
    void print();
};

} // namespace token
#endif // TOKENS_HPP