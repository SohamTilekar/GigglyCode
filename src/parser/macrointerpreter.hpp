#ifndef MACROINTERPRETER_HPP
#define MACROINTERPRETER_HPP

#include <string>
#include <unordered_map>
#include <variant>
#include "../lexer/lexer.hpp"
#include "AST/ast.hpp"
#include "parser.hpp"

enum class MIObjectType {
    Int,
    Float,
    Str,
    Bool,
    Void,
    Token,
    TokenVector,
    TokenType,
    TokenTypeVector,
};

using MIObjectVariant = std::variant<int, float, std::string, bool, token::Token, std::vector<token::Token>, token::TokenType, std::vector<token::TokenType>>;

class MIObjects {
public:
    MIObjectType Type;
    MIObjectVariant Value;
    MIObjects() = default;
    MIObjects(MIObjectType Type, MIObjectVariant Value) : Type(Type), Value(Value) {};
};

class MacroInterpreter {
public:
    MacroInterpreter(Lexer* lexer, parser::Parser* parser) : lexer(lexer), parser(parser) {};

    void interpret(AST::MacroStatement* macro) { visitBlockStatement(macro->body); };
private:
    Lexer* lexer;
    parser::Parser* parser;
    std::unordered_map<std::string, MIObjects> variabels;
    void visitStatement(AST::Statement* node);
    void visitBlockStatement(AST::BlockStatement* node);
    void visitExpressionStatement(AST::ExpressionStatement* node);
    void visitVariableAssignmentStatement(AST::VariableAssignmentStatement* node);
    void visitReturnStatement(AST::ReturnStatement* node);
    void visitIfElseStatement(AST::IfElseStatement* node);
    void visitWhileStatement(AST::WhileStatement* node);
    void visitForEachStatement(AST::ForEachStatement* node);
    // void visitSwitchCaseStatement(AST::SwitchCaseStatement* node);

    MIObjects visitExpression(AST::Expression* node);
    MIObjects visitCallExpression(AST::CallExpression* node);
    MIObjects visitInfixedExpression(AST::InfixExpression* node);
    MIObjects visitIndexExpression(AST::IndexExpression* node);
    MIObjects visitArrayLiteral(AST::ArrayLiteral* node);
    MIObjects visitIntegerLiteral(AST::IntegerLiteral* node);
    MIObjects visitFloatLiteral(AST::FloatLiteral* node);
};

#endif // MACROINTERPRETER_HPP
