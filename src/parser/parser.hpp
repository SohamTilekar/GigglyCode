#ifndef PARSER_HPP
#define PARSER_HPP

#include <functional>
#include <string>
#include <unordered_map>

#include "../config.hpp"
#include "../lexer/token.hpp"
#include "AST/ast.hpp"


#ifdef DEBUG_PARSER
#define LOG_PATH "./dump/dbg.log"
#include <fstream>
#include <iostream>
#include <mutex>

namespace parser {

class Logger {
  public:
    static Logger& getInstance(const std::string& path = LOG_PATH) {
        static Logger instance(path);
        return instance;
    }

    void log(const std::string& file_name, const std::string& function_name, uint32_t line_no, const std::string& message) {
        std::lock_guard<std::mutex> guard(mtx_);
        if (log_stream_.is_open()) { log_stream_ << "[" << file_name << ":" << function_name << ":" << line_no << "] " << message << std::endl; }
    }

  private:
    std::ofstream log_stream_;
    std::mutex mtx_;

    Logger(const std::string& path) : log_stream_(path, std::ios::out | std::ios::app) {
        if (!log_stream_) { std::cerr << "Failed to open log file: " << path << std::endl; }
    }

    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;
};

} // namespace parser
#endif

namespace parser {

using token::TokenType;

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

struct Parser {
    token::Tokens tokens;
    token::Token currentToken;
    token::Token peekToken;
    std::string filePath;

    // Prefix parse functions
    const std::unordered_map<TokenType, std::function<AST::Expression*()>> prefix_parse_fns = {
        {TokenType::Integer, std::bind(&Parser::_parseIntegerLiteral, this)},
        {TokenType::Float, std::bind(&Parser::_parseFloatLiteral, this)},
        {TokenType::StringDSQ, std::bind(&Parser::_parseStringLiteral, this)},
        {TokenType::StringDTQ, std::bind(&Parser::_parseStringLiteral, this)},
        {TokenType::StringSTQ, std::bind(&Parser::_parseStringLiteral, this)},
        {TokenType::StringSSQ, std::bind(&Parser::_parseStringLiteral, this)},
        {TokenType::True, std::bind(&Parser::_parseBooleanLiteral, this)},
        {TokenType::False, std::bind(&Parser::_parseBooleanLiteral, this)},
        {TokenType::Identifier, std::bind(&Parser::_InterpretExpresionIdentifier, this)},
        {TokenType::LeftParen, std::bind(&Parser::_parseGroupedExpression, this)},
        {TokenType::LeftBracket, std::bind(&Parser::_parseArrayLiteral, this)},
        {TokenType::New, std::bind(&Parser::_parseNew, this)},
    };

    // Infix parse functions
    const std::unordered_map<TokenType, std::function<AST::Expression*(AST::Expression*)>> infix_parse_fns = {
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

    Parser(token::Tokens tokens, std::string file_path);

    ~Parser();

    AST::Program* parseProgram();

    struct LoopModifiers {
        AST::Statement* ifbreak;
        AST::Statement* notbreak;
    };

    LoopModifiers _parseLoopModifiers();
    void _nextToken();

    bool _currentTokenIs(TokenType type);

    bool _peekTokenIs(TokenType type);

    bool _expectPeek(std::vector<TokenType> types, std::string suggestedFix = "");

    bool _expectPeek(TokenType type, std::string suggestedFix = "");

    [[noreturn]] void _peekTokenError(TokenType type, std::vector<TokenType> expected_types, std::string suggestedFix = "");

    [[noreturn]] void _currentTokenError(TokenType type, std::vector<TokenType> expected_types, std::string suggestedFix = "");

    PrecedenceType _currentPrecedence();

    PrecedenceType _peekPrecedence();

    AST::Statement* _parseStatement();

    AST::Statement* _parseExpressionStatement(AST::Expression* identifier = nullptr);

    AST::VariableDeclarationStatement* _parseVariableDeclaration(AST::Expression* identifier = nullptr, bool is_volatile = false, bool is_const = false);

    AST::VariableAssignmentStatement* _parseVariableAssignment(AST::Expression* identifier = nullptr);

    AST::ReturnStatement* _parseReturnStatement();

    AST::RaiseStatement* _parseRaiseStatement();

    AST::FunctionStatement* _parseFunctionStatement();

    std::vector<AST::FunctionParameter*> _parseFunctionParameters();

    std::vector<AST::FunctionParameter*> _parseClosureParameters();

    AST::Statement* _parseDeco();

    void _parseMacroDecleration();

    AST::BlockStatement* _parseBlockStatement();

    AST::IfElseStatement* _parseIfElseStatement();

    AST::WhileStatement* _parseWhileStatement();

    AST::Statement* _parseForStatement();

    AST::BreakStatement* _parseBreakStatement();

    AST::ContinueStatement* _parseContinueStatement();

    AST::ImportStatement* _parseImportStatement();

    AST::StructStatement* _parseStructStatement();

    AST::EnumStatement* _parseEnumStatement();

    AST::TryCatchStatement* _parseTryCatchStatement();

    AST::SwitchCaseStatement* _parseSwitchCaseStatement();

    AST::CallExpression* _parseFunctionCall(AST::IdentifierLiteral* identifier = nullptr);

    AST::Type* _parseType();

    AST::Expression* _parseExpression(PrecedenceType precedence, AST::Expression* parsed_expression = nullptr);

    AST::IntegerLiteral* _parseIntegerLiteral();

    AST::FloatLiteral* _parseFloatLiteral();

    AST::BooleanLiteral* _parseBooleanLiteral();

    AST::StringLiteral* _parseStringLiteral();
    AST::Expression* _parseGroupedExpression();

    AST::IdentifierLiteral* _parseIdentifier();

    AST::Expression* _InterpretExpresionIdentifier();

    AST::ArrayLiteral* _parseArrayLiteral();

    AST::Expression* _parseNew();

    std::vector<AST::Expression*> _parse_expression_list(TokenType end);

    AST::Expression* _parseInfixExpression(AST::Expression* leftNode);

    AST::Expression* _parseIndexExpression(AST::Expression* leftNode);

    AST::Expression* _parseInfixIdenifier();

    AST::Statement* _interpretIdentifier();

    AST::Statement* _parseGenericDeco();

    AST::Statement* _parseAutocastDeco();
}; // class Parser

} // namespace parser

#endif // PARSER_HPP
