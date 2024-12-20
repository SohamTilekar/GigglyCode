#ifndef TOKENS_HPP
#define TOKENS_HPP
#include <string>

namespace token {
enum class TokenType {
  // Special Tokens
  EndOfFile, // End of file token
  Illegal,   // Illegal token

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
  True,     // Boolean true 'true'
  False,    // Boolean false 'false'
  None,     // None type 'none'
  New,      // New keyword 'new'
  Try,      // Try keyword 'try'
  Catch,    // Catch keyword 'catch'
  Raise,    // Raise keyword 'raise'
  IfBreak,  // IfBreak keyword 'ifbreak'
  NotBreak, // NotBreak keyword 'notbreak'
};

// Convert TokenType to string for debugging
std::string tokenTypeString(TokenType type);

class Token {
  public:
  TokenType type;
  std::string literal;
  int line_no;
  int end_col_no;
  int col_no;

  inline Token() {};
  inline Token(TokenType type, int lineNo, int colNo) : type(type), line_no(lineNo), end_col_no(colNo - 1), col_no(colNo - 1) {};
  inline Token(TokenType type, std::string literal, int lineNo, int colNo) : type(type), literal(literal), line_no(lineNo), end_col_no(colNo - 1), col_no(colNo - literal.length() - 1) {};
  // Convert current token to string in the format:
  // "[type: Identifier, literal: float, line_no: 30, col_no: 11, end_col_no: 16]"
  // The color argument adds color to the string for CLI printing
  std::string toString(bool color = true);
  // Print the current Token to the CLI
  void print();
};
} // namespace token
#endif // TOKENS_HPP
