#ifndef TOKENS_HPP
#define TOKENS_HPP
#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>

namespace token {
enum class TokenType {
    // Special Tokens
    EndOfFile,
    Illegal,

    // Comparison Operators
    GreaterThan,        // Greater than >
    LessThan,           // Less than <
    GreaterThanOrEqual, // Greater than or equal to >=
    LessThanOrEqual,    // Less than or equal to <=
    EqualEqual,         // Equal to ==
    NotEquals,          // Not equal to !=

    // Data Types
    Identifier, // Identifier token [a-zA-Z_][a-zA-Z0-9_]*
    Integer,    // Integer token [0-9]+
    Float,      // Float token [0-9]+.[0-9]+
    String,     // String token "[\"'].*[\"']" | "'''[^']*'''" | '"""[^"]*"""'
    RawString,  // Raw String token "r[\"'].*[\"']" | "R[\"'].*[\"']" |
                // "r'''[^']*'''" | "R'''[^']*'''" | '"""[^"]*"""' |
                // 'R"""[^"]*"""'

    // Assignment Operators
    PlusEqual,          // Addition assignment +=
    DashEqual,          // Subtraction assignment -=
    AsteriskEqual,      // Multiplication assignment *=
    PercentEqual,       // Modulus assignment %=
    CaretEqual,         // Exponentiation assignment ^=
    ForwardSlashEqual,  // Division assignment /=
    BackwardSlashEqual, // Division assignment \=
    Equals,             // Equals sign =
    Is,                 // Is sign is

    // Increment and Decrement Operators
    Increment, // Increment ++
    Decrement, // Decrement --

    // Bitwise Operators
    BitwiseAnd, // Bitwise AND &&
    BitwiseOr,  // Bitwise OR ||
    BitwiseXor, // Bitwise XOR ^
    BitwiseNot, // Bitwise NOT ~
    LeftShift,  // Left shift <<
    RightShift, // Right shift >>

    // Arithmetic Operators
    Dot,              // Dot .
    Ellipsis,         // Ellipsis ...
    Plus,             // Addition +
    Dash,             // Subtraction -
    Asterisk,         // Multiplication *
    Percent,          // Modulus %
    AsteriskAsterisk, // Exponentiation **
    ForwardSlash,     // Division /
    BackwardSlash,    //

    // Symbols
    LeftParen,    // Left Parenthesis (
    RightParen,   // Right Parenthesis )
    LeftBrace,    // Left Brace {
    RightBrace,   // Right Brace }
    LeftBracket,  // Left Bracket [
    RightBracket, // Right Bracket ]
    Colon,        // Colon :
    Semicolon,    // Semicolon ;
    RightArrow,   // Right Arrow ->
    Comma,        // Comma ,

    // Keywords
    And,      // Logical AND and
    Or,       // Logical OR or
    Not,      // Logical NOT not
    Def,      // Function definition def
    Return,   // Return Statement return
    If,       // If Statement if
    Else,     // Else Statement else
    ElIf,     // Else If Statement elif
    While,    // While Statement while
    For,      // For Statement for
    In,       // In Statement in
    Break,    // Break Statement break
    Continue, // Continue Statement continue
    Struct,   // Struct
    Enum,     // Enum
    Volatile, // volatile
    Use,
    Import,   // import
    True,     // Boolean true true
    False,    // Boolean false false
    // Maybe,    // Maybe type maybe
    None, // None type none
};

std::shared_ptr<std::string> tokenTypeString(TokenType type);

class Token {
  public:
    TokenType type;
    std::string literal;
    int line_no;
    int end_col_no;
    int col_no;
    inline Token() {};
    inline Token(TokenType type, int line_no, int col_no) : type(type), line_no(line_no), end_col_no(col_no - 1), col_no(col_no - 1) {};
    inline Token(TokenType type, std::string literal, int line_no, int col_no)
        : type(type), literal(literal), line_no(line_no), end_col_no(col_no - 1), col_no(col_no - literal.length() - 1) {};
    std::string toString(bool color = true);
    void print();
};
} // namespace token
#endif // TOKENS_HPP
