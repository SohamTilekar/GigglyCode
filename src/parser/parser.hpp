/**
 * @file parser.hpp
 * @brief This file contains the definition of the Parser class and related components for parsing tokens into an
 * abstract syntax tree (AST).
 */
#ifndef PARSER_HPP
#define PARSER_HPP

#include <functional>
#include <string>
#include <unordered_map>

#include "../lexer/lexer.hpp"
#include "../lexer/token.hpp"
#include "AST/ast.hpp"

#define LOG
#define LOG_PATH "./dump/dbg.log"

#ifdef LOG
#include <fstream>
#include <iostream>
#include <mutex>

namespace parser {

/**
 * @class Logger
 * @brief Singleton Logger class for handling log messages.
 */
class Logger {
  public:
    /**
     * @brief Get the singleton instance of the Logger.
     *
     * @param path The file path for the log file. Defaults to LOG_PATH.
     * @return Logger& Reference to the singleton Logger instance.
     */
    static Logger& getInstance(const std::string& path = LOG_PATH) {
        static Logger instance(path);
        return instance;
    }

    /**
     * @brief Log a message with detailed meta information.
     *
     * @param file_name The name of the source file.
     * @param function_name The name of the function.
     * @param line_no The line number in the source file.
     * @param message The message to log.
     */
    void log(const std::string& file_name, const std::string& function_name, int line_no, const std::string& message) {
        std::lock_guard<std::mutex> guard(mtx_);
        if (log_stream_.is_open()) { log_stream_ << "[" << file_name << ":" << function_name << ":" << line_no << "] " << message << std::endl; }
    }

  private:
    std::ofstream log_stream_;
    std::mutex mtx_;

    // Private constructor to enforce singleton pattern
    Logger(const std::string& path) : log_stream_(path, std::ios::out | std::ios::app) {
        if (!log_stream_) { std::cerr << "Failed to open log file: " << path << std::endl; }
    }

    // Delete copy constructor and assignment operator
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

} // namespace parser
#endif

namespace parser {
/**
 * @namespace parser
 * @brief Namespace for the parser components.
 */

using token::TokenType;

/**
 * @enum PrecedenceType
 * @brief Enum representing the precedence levels of different token types.
 */
enum class PrecedenceType {
    LOWEST,        ///< Lowest precedence
    ASSIGN,        ///< Assignment operators (=, +=, -=, *=, /=, %=)
    AND,           ///< Logical AND operator (and)
    OR,            ///< Logical OR operator (or)
    COMPARISION,   ///< Comparison operators (>, <, >=, <=, ==, !=)
    BITWISE_AND,   ///< Bitwise AND operator (&)
    BITWISE_XOR,   ///< Bitwise XOR operator (^)
    BITWISE_OR,    ///< Bitwise OR operator (|)
    SUM,           ///< Addition and subtraction (+, -)
    PRODUCT,       ///< Multiplication, division, and modulo (*, /, %)
    Exponent,      ///< Exponentiation (**)
    PREFIX,        ///< Prefix operators (-X, !X)
    CALL,          ///< Function calls (myFunction(X))
    INDEX,         ///< Array indexing (array[index])
    MEMBER_ACCESS, ///< Member access (object.member)
    POSTFIX        ///< Postfix operators (X++)
};

// Token precedence mapping
static const std::unordered_map<TokenType, PrecedenceType> token_precedence = {
    // Lowest precedence
    {TokenType::Illegal, PrecedenceType::LOWEST},
    {TokenType::Ellipsis, PrecedenceType::LOWEST},
    {TokenType::EndOfFile, PrecedenceType::LOWEST},

    // Assignment operators
    {TokenType::PlusEqual, PrecedenceType::ASSIGN},
    {TokenType::DashEqual, PrecedenceType::ASSIGN},
    {TokenType::AsteriskEqual, PrecedenceType::ASSIGN},
    {TokenType::PercentEqual, PrecedenceType::ASSIGN},
    {TokenType::CaretEqual, PrecedenceType::ASSIGN},
    {TokenType::ForwardSlashEqual, PrecedenceType::ASSIGN},
    {TokenType::BackwardSlashEqual, PrecedenceType::ASSIGN},
    {TokenType::Equals, PrecedenceType::ASSIGN},
    {TokenType::Is, PrecedenceType::ASSIGN},

    // Comparison operators
    {TokenType::GreaterThan, PrecedenceType::COMPARISION},
    {TokenType::LessThan, PrecedenceType::COMPARISION},
    {TokenType::GreaterThanOrEqual, PrecedenceType::COMPARISION},
    {TokenType::LessThanOrEqual, PrecedenceType::COMPARISION},
    {TokenType::EqualEqual, PrecedenceType::COMPARISION},
    {TokenType::NotEquals, PrecedenceType::COMPARISION},

    // Bitwise operators
    {TokenType::BitwiseAnd, PrecedenceType::BITWISE_AND},
    {TokenType::BitwiseXor, PrecedenceType::BITWISE_XOR},
    {TokenType::BitwiseOr, PrecedenceType::BITWISE_OR},

    // Logical operators
    {TokenType::And, PrecedenceType::AND},
    {TokenType::Or, PrecedenceType::OR},

    // Addition and subtraction
    {TokenType::Plus, PrecedenceType::SUM},
    {TokenType::Dash, PrecedenceType::SUM},

    // Multiplication and division
    {TokenType::Asterisk, PrecedenceType::PRODUCT},
    {TokenType::Percent, PrecedenceType::PRODUCT},
    {TokenType::ForwardSlash, PrecedenceType::PRODUCT},
    {TokenType::BackwardSlash, PrecedenceType::PRODUCT},

    // Exponentiation
    {TokenType::AsteriskAsterisk, PrecedenceType::Exponent},

    // Prefix operators
    {TokenType::BitwiseNot, PrecedenceType::PREFIX},

    // Postfix operators
    {TokenType::Increment, PrecedenceType::POSTFIX},
    {TokenType::Decrement, PrecedenceType::POSTFIX},

    // Member access
    {TokenType::Dot, PrecedenceType::MEMBER_ACCESS},

    // Function calls
    {TokenType::LeftParen, PrecedenceType::CALL},

    // Array indexing
    {TokenType::LeftBracket, PrecedenceType::INDEX},
};

/**
 * @class Parser
 * @brief Class responsible for parsing tokens into an abstract syntax tree (AST).
 */
class Parser {
  public:
    Lexer* lexer;               ///< The lexer used for tokenizing the input
    token::Token current_token; ///< The current token being parsed
    token::Token peek_token;    ///< The next token to be parsed

    // Prefix parse functions
    std::unordered_map<TokenType, std::function<AST::Expression*()>> prefix_parse_fns = {
        {TokenType::Integer, std::bind(&Parser::_parseIntegerLiteral, this)},
        {TokenType::Float, std::bind(&Parser::_parseFloatLiteral, this)},
        {TokenType::String, std::bind(&Parser::_parseStringLiteral, this)},
        {TokenType::True, std::bind(&Parser::_parseBooleanLiteral, this)},
        {TokenType::False, std::bind(&Parser::_parseBooleanLiteral, this)},
        {TokenType::Identifier, std::bind(&Parser::_parseIdentifier, this)},
        {TokenType::LeftParen, std::bind(&Parser::_parseGroupedExpression, this)},
        {TokenType::LeftBracket, std::bind(&Parser::_parseArrayLiteral, this)},
        {TokenType::New, std::bind(&Parser::_parseNew, this)},
    };

    // Infix parse functions
    std::unordered_map<TokenType, std::function<AST::Expression*(AST::Expression*)>> infix_parse_fns = {
        {TokenType::Or, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::And, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::Plus, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::Dash, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::Asterisk, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::ForwardSlash, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::Percent, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::AsteriskAsterisk, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::GreaterThan, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::LessThan, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::GreaterThanOrEqual, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::LessThanOrEqual, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::EqualEqual, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::NotEquals, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::Dot, std::bind(&Parser::_parseInfixExpression, this, std::placeholders::_1)},
        {TokenType::LeftBracket, std::bind(&Parser::_parseIndexExpression, this, std::placeholders::_1)},
    };

    std::unordered_map<std::string, AST::MacroStatement*> macros;

    /**
     * @brief Construct a new Parser object
     *
     * Initializes the parser with the provided lexer and reads the first two tokens.
     *
     * @param lexer The lexer to use for tokenizing the input
     */
    Parser(Lexer* lexer);

    ~Parser();

    /**
     * @brief Parse the entire program
     *
     * Iteratively parses statements until the EndOfFile token is encountered, constructing the AST.
     *
     * @return AST::Program* The parsed program
     */
    AST::Program* parseProgram();

    /**
     * @brief Struct to hold loop modifier statements.
     */
    struct LoopModifiers {
        AST::Statement* ifbreak;
        AST::Statement* notbreak;
    };

    /**
     * @brief Parse loop modifiers (e.g., notbreak, ifbreak) after the loop body.
     *
     * @return LoopModifiers The parsed loop modifiers.
     */
    LoopModifiers _parseLoopModifiers();
    // Token management methods

    /**
     * @brief Advance to the next token
     *
     * Updates the current and peek tokens by reading the next token from the lexer.
     */
    void _nextToken();

    /**
     * @brief Check if the current token matches the specified type
     *
     * @param type The token type to check against the current token
     * @return true If the current token matches the specified type
     * @return false Otherwise
     */
    bool _currentTokenIs(TokenType type);

    /**
     * @brief Check if the next token matches the specified type
     *
     * @param type The token type to check against the peek token
     * @return true If the peek token matches the specified type
     * @return false Otherwise
     */
    bool _peekTokenIs(TokenType type);

    /**
     * @brief Assert that the next token is one of the expected types and advance to it
     *
     * @param types A vector of acceptable token types
     * @param suggestedFix An optional suggestion for correcting the error
     * @return true If the peek token matches one of the expected types
     * @return false Otherwise
     */
    bool _expectPeek(std::vector<TokenType> types, std::string suggestedFix = "");

    /**
     * @brief Assert that the next token is a specific type and advance to it
     *
     * @param type The expected token type
     * @param suggestedFix An optional suggestion for correcting the error
     * @return true If the peek token matches the expected type
     * @return false Otherwise
     */
    bool _expectPeek(TokenType type, std::string suggestedFix = "");

    /**
     * @brief Handle a peek token error by reporting unexpected token and expected types
     *
     * @param type The actual token type encountered
     * @param expected_types A vector of expected token types
     * @param suggestedFix An optional suggestion for correcting the error
     */
    [[noreturn]] void _peekTokenError(TokenType type, std::vector<TokenType> expected_types, std::string suggestedFix = "");

    /**
     * @brief Handle a current token error by reporting unexpected token and expected types
     *
     * @param type The actual token type encountered
     * @param expected_types A vector of expected token types
     * @param suggestedFix An optional suggestion for correcting the error
     */
    [[noreturn]] void _currentTokenError(TokenType type, std::vector<TokenType> expected_types, std::string suggestedFix = "");

    /**
     * @brief Get the precedence of the current token
     *
     * @return PrecedenceType The precedence level of the current token
     */
    PrecedenceType _currentPrecedence();

    /**
     * @brief Get the precedence of the peek token
     *
     * @return PrecedenceType The precedence level of the peek token
     */
    PrecedenceType _peekPrecedence();

    // Statement parsing methods

    /**
     * @brief Parse a single statement based on the current token
     *
     * Dispatches to specific parsing functions for different statement types.
     *
     * @return AST::Statement* The parsed statement
     */
    AST::Statement* _parseStatement();

    /**
     * @brief Parse an expression statement, optionally starting with an identifier
     *
     * Handles expressions that may represent variable assignments or general expressions.
     *
     * @param identifier Optional identifier expression to start with
     * @param st_line_no Optional start line number for metadata
     * @param st_col_no Optional start column number for metadata
     * @return AST::Statement* The parsed expression statement
     */
    AST::Statement* _parseExpressionStatement(AST::Expression* identifier = nullptr, int st_line_no = -1, int st_col_no = -1);

    /**
     * @brief Parse a variable declaration statement
     *
     * Handles both initialized and uninitialized variable declarations, with optional volatility.
     *
     * @param identifier Optional identifier expression
     * @param st_line_no Optional start line number for metadata
     * @param st_col_no Optional start column number for metadata
     * @param is_volatile Optional flag indicating if the variable is volatile
     * @return AST::Statement* The parsed variable declaration statement
     */
    AST::Statement* _parseVariableDeclaration(AST::Expression* identifier = nullptr, int st_line_no = -1, int st_col_no = -1, bool is_volatile = false);

    /**
     * @brief Parse a variable assignment statement
     *
     * Handles assignments where a variable is assigned a new value.
     *
     * @param identifier Optional identifier expression to assign to
     * @param st_line_no Optional start line number for metadata
     * @param st_col_no Optional start column number for metadata
     * @return AST::Statement* The parsed variable assignment statement
     */
    AST::Statement* _parseVariableAssignment(AST::Expression* identifier = nullptr, int st_line_no = -1, int st_col_no = -1);

    /**
     * @brief Parse a return statement
     *
     * Handles both bare returns and returns with expressions.
     *
     * @return AST::ReturnStatement* The parsed return statement
     */
    AST::ReturnStatement* _parseReturnStatement();

    /**
     * @brief Parse a raise statement
     *
     * Handles statements that raise exceptions or errors.
     *
     * @return AST::RaiseStatement* The parsed raise statement
     */
    AST::RaiseStatement* _parseRaiseStatement();

    /**
     * @brief Parse a function statement
     *
     * Handles the declaration of functions, including parameters, return types, and function bodies.
     *
     * @return AST::FunctionStatement* The parsed function statement
     */
    AST::FunctionStatement* _parseFunctionStatement();

    /**
     * @brief Parse function parameters.
     *
     * @return std::vector<AST::FunctionParameter*> The list of parsed function parameters.
     */
    std::vector<AST::FunctionParameter*> _parseFunctionParameters();

    /**
     * @brief Parse closure parameters in function definitions.
     *
     * @return std::vector<AST::FunctionParameter*> The list of parsed closure parameters.
     */
    std::vector<AST::FunctionParameter*> _parseClosureParameters();

    /**
     * @brief Parse a decorator statement
     *
     * Handles decorators applied to functions or structs.
     *
     * @return AST::Statement* The parsed decorator statement
     */
    AST::Statement* _parseDeco();

    void _parseMacroDecleration();

    /**
     * @brief Parse a block statement
     *
     * Handles a block of statements enclosed within braces `{}`.
     *
     * @return AST::BlockStatement* The parsed block statement
     */
    AST::BlockStatement* _parseBlockStatement();

    /**
     * @brief Parse an if-else statement
     *
     * Handles conditional statements with optional else blocks.
     *
     * @return AST::Statement* The parsed if-else statement
     */
    AST::Statement* _parseIfElseStatement();

    /**
     * @brief Parse a while statement
     *
     * Handles loop constructs that execute based on a condition.
     *
     * @return AST::WhileStatement* The parsed while statement
     */
    AST::WhileStatement* _parseWhileStatement();

    /**
     * @brief Parse a for statement
     *
     * Handles loop constructs that iterate over a range or collection.
     *
     * @return AST::ForStatement* The parsed for statement
     */
    AST::Statement* _parseForStatement();

    /**
     * @brief Parse a break statement
     *
     * Handles statements that exit a loop prematurely.
     *
     * @return AST::BreakStatement* The parsed break statement
     */
    AST::BreakStatement* _parseBreakStatement();

    /**
     * @brief Parse a continue statement
     *
     * Handles statements that skip the current loop iteration.
     *
     * @return AST::ContinueStatement* The parsed continue statement
     */
    AST::ContinueStatement* _parseContinueStatement();

    /**
     * @brief Parse an import statement
     *
     * Handles statements that import modules or libraries.
     *
     * @return AST::ImportStatement* The parsed import statement
     */
    AST::ImportStatement* _parseImportStatement();

    /**
     * @brief Parse a struct statement
     *
     * Handles the declaration of structs, including their members and methods.
     *
     * @return AST::StructStatement* The parsed struct statement
     */
    AST::StructStatement* _parseStructStatement();

    /**
     * @brief Parse a try-catch statement
     *
     * Handles error handling constructs with try and catch blocks.
     *
     * @return AST::TryCatchStatement* The parsed try-catch statement
     */
    AST::TryCatchStatement* _parseTryCatchStatement();

    /**
     * @brief Parse a switch-case statement
     *
     * @return AST::SwitchCaseStatement* The parsed switch-case statement
     */
    AST::SwitchCaseStatement* _parseSwitchCaseStatement();

    /**
     * @brief Parse a function call expression
     *
     * Handles the invocation of functions with arguments.
     *
     * @param identifier Optional identifier expression representing the function name
     * @param st_line_no Optional start line number for metadata
     * @param st_col_no Optional start column number for metadata
     * @return AST::Expression* The parsed function call expression
     */
    AST::Expression* _parseFunctionCall(AST::Expression* identifier = nullptr, int st_line_no = -1, int st_col_no = -1);

    // Expression parsing methods

    /**
     * @brief Parse a type
     *
     * Handles type declarations, including generic types and references.
     *
     * @return AST::Type* The parsed type
     */
    AST::Type* _parseType();

    /**
     * @brief Parse an expression
     *
     * Parses expressions based on precedence and operator precedence rules.
     *
     * @param precedence The current precedence level
     * @param parsed_expression Optional already parsed expression to extend
     * @param st_line_no Optional start line number for metadata
     * @param st_col_no Optional start column number for metadata
     * @return AST::Expression* The parsed expression
     */
    AST::Expression* _parseExpression(PrecedenceType precedence, AST::Expression* parsed_expression = nullptr, int st_line_no = -1, int st_col_no = -1);

    /**
     * @brief Parse an integer literal expression
     *
     * @return AST::Expression* The parsed integer literal
     */
    AST::Expression* _parseIntegerLiteral();

    /**
     * @brief Parse a float literal expression
     *
     * @return AST::Expression* The parsed float literal
     */
    AST::Expression* _parseFloatLiteral();

    /**
     * @brief Parse a boolean literal expression
     *
     * @return AST::Expression* The parsed boolean literal
     */
    AST::Expression* _parseBooleanLiteral();

    /**
     * @brief Parse a string literal expression
     *
     * @return AST::Expression* The parsed string literal
     */
    AST::Expression* _parseStringLiteral();

    /**
     * @brief Parse a grouped expression (expressions within parentheses)
     *
     * @return AST::Expression* The parsed grouped expression
     */
    AST::Expression* _parseGroupedExpression();

    /**
     * @brief Parse an identifier expression
     *
     * @return AST::Expression* The parsed identifier
     */
    AST::Expression* _parseIdentifier();

    /**
     * @brief Parse an array literal expression
     *
     * Handles array definitions with multiple elements.
     *
     * @return AST::Expression* The parsed array literal
     */
    AST::Expression* _parseArrayLiteral();

    /**
     * @brief Parse a 'new' expression for object or array creation
     *
     * Identifies and handles 'new' object or array creation expressions.
     *
     * @return AST::Expression* The parsed 'new' expression
     */
    AST::Expression* _parseNew();

    /**
     * @brief Parse a list of expressions separated by commas
     *
     * @param end The token type that signifies the end of the expression list
     * @return std::vector<AST::Expression*> The list of parsed expressions
     */
    std::vector<AST::Expression*> _parse_expression_list(TokenType end);

    /**
     * @brief Parse an infix expression based on the left-hand side expression
     *
     * @param leftNode The left-hand side expression
     * @return AST::Expression* The parsed infix expression
     */
    AST::Expression* _parseInfixExpression(AST::Expression* leftNode);

    /**
     * @brief Parse an index expression (e.g., array[index])
     *
     * @param leftNode The array expression being indexed
     * @return AST::Expression* The parsed index expression
     */
    AST::Expression* _parseIndexExpression(AST::Expression* leftNode);

    /**
     * @brief Parse an infix identifier recursively
     *
     * Handles chained member accesses like object.property.subproperty.
     *
     * @return AST::Expression* The parsed infix identifier expression
     */
    AST::Expression* _parseInfixIdenifier();

    // Helpers

    /**
     * @brief Parses an identifier and determines its context (declaration, assignment, function call, etc.)
     *
     * @return AST::Statement* The parsed statement based on the identifier context
     */
    AST::Statement* _interpretIdentifier();

    /**
     * @brief Parse a generic decorator
     *
     * Handles decorators that specify generic types.
     *
     * @return AST::Statement* The parsed generic decorator statement
     */
    AST::Statement* _parseGenericDeco();

    /**
     * @brief Parse an autocast decorator
     *
     * Handles decorators that enable autocasting functionality.
     *
     * @return AST::Statement* The parsed autocast decorator statement
     */
    AST::Statement* _parseAutocastDeco();
}; // class Parser

} // namespace parser

#endif // PARSER_HPP
