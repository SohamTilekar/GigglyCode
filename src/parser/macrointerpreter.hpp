#ifndef MACROINTERPRETER_HPP
#define MACROINTERPRETER_HPP

#include <string>
#include <unordered_map>
#include <memory>
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
    Node,
    NodeVector,
    NodeType
};

class MIObjects {
public:
    MIObjectType Type;
    void* objectpointer = nullptr;
    MIObjects() = default;
    MIObjects(MIObjectType Type, void* objectpointer = nullptr) : Type(Type), objectpointer(objectpointer) {};
    ~MIObjects(){
        if(objectpointer != nullptr){
            switch(Type){
                // case MIObjectType::Int: delete static_cast<int*>(objectpointer); break;
                // case MIObjectType::Float: delete static_cast<float*>(objectpointer); break;
                // case MIObjectType::Str: delete static_cast<std::string*>(objectpointer); break;
                // case MIObjectType::Bool: delete static_cast<bool*>(objectpointer); break;
                // Add cases for other types as needed
                default: break;
            }
            objectpointer = nullptr;
        }
    }
};

class MacroInterpreter {
public:
    MacroInterpreter(Lexer* lexer, parser::Parser* parser) : lexer(lexer), parser(parser) {
        throw std::runtime_error("TODO: Add Suport to the Macros");
    };

    void interpret(AST::MacroStatement* macro) {};
private:
    Lexer* lexer;
    parser::Parser* parser;
    std::unordered_map<std::string, MIObjects*> variabels;
    // void visitStatement(AST::Statement* node);
    // void visitBlockStatement(AST::BlockStatement* node);
    // void visitExpressionStatement(AST::ExpressionStatement* node);
    // void visitVariableAssignmentStatement(AST::VariableAssignmentStatement* node);
    // void visitReturnStatement(AST::ReturnStatement* node);
    // void visitIfElseStatement(AST::IfElseStatement* node);
    // void visitWhileStatement(AST::WhileStatement* node);
    // void visitForStatement(AST::ForStatement* node);
    // void visitSwitchCaseStatement(AST::SwitchCaseStatement* node);
    // MIObjects* visitExpression(AST::Expression* node);
    // MIObjects* visitCallExpression(AST::CallExpression* node);
    // MIObjects* visitInfixedExpression(AST::InfixExpression* node);
};

#endif // MACROINTERPRETER_HPP
