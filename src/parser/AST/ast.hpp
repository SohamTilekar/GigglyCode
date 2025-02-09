#ifndef AST_HPP
#define AST_HPP
#include <string>
#include <vector>

#include "../../config.hpp"
#include "../../lexer/token.hpp"

namespace AST {
struct Node;
struct Statement;
struct Type;
struct Program;
struct FunctionParameter;
struct Expression;
struct IdentifierLiteral;
struct IntegerLiteral;
struct FloatLiteral;
struct StringLiteral;
struct BooleanLiteral;
struct ArrayLiteral;
struct InfixExpression;
struct IndexExpression;
struct CallExpression;
struct ExpressionStatement;
struct BlockStatement;
struct ReturnStatement;
struct RaiseStatement;
struct FunctionStatement;
struct IfElseStatement;
struct WhileStatement;
struct ForEachStatement;
struct ForStatement;
struct BreakStatement;
struct ContinueStatement;
struct ImportStatement;
struct VariableDeclarationStatement;
struct VariableAssignmentStatement;
struct TryCatchStatement;
struct StructStatement;
struct EnumStatement;
struct SwitchCaseStatement;
struct MacroStatement;

enum struct NodeType : char {
    Program,
    Unknown,

    // Statements
    ExpressionStatement,
    VariableDeclarationStatement,
    VariableAssignmentStatement,
    FunctionStatement,
    FunctionParameter,
    CallExpression,
    BlockStatement,
    ReturnStatement,
    IfElseStatement,
    WhileStatement,
    ForStatement,
    ForEachStatement,
    BreakStatement,
    ContinueStatement,
    StructStatement,
    EnumStatement,
    ImportStatement,
    TryCatchStatement,
    RaiseStatement,
    SwitchCaseStatement,
    MacroStatement,

    // Types
    Type,

    // Expressions
    InfixedExpression,
    IndexExpression,

    // Literals
    IntegerLiteral,
    FloatLiteral,
    BooleanLiteral,
    StringLiteral,
    IdentifierLiteral,
    ArrayLiteral
};

std::string nodeTypeToString(NodeType type);

#ifdef DEBUG_PARSER
#define DefToStr std::string toStr() override
#else
#define DefToStr
#endif

struct Node {
    token::Token lastToken;
    uint32_t firstToken : 24;
    NodeType type = NodeType::Unknown;

    #ifdef DEBUG_PARSER
        virtual inline std::string toStr() {
            std::string yaml = "type: " + nodeTypeToString(type) + "\n";
            return yaml;
        }
    #endif

    Node(uint32_t firstToken, token::Token lastToken)
        : firstToken(firstToken), lastToken(lastToken) {};
    Node() {};

    uint32_t getStColNo(str source) const {
            return firstToken;
    }

    uint32_t getStLineNo(str source) const {
        uint32_t line_no = 1;
        uint32_t pos = 0;
        while (pos < this->firstToken && pos < source.len) {
            if (source.string[pos] == '\n') {
                line_no++;
            }
            pos++;
        }
        return line_no;
    }

    uint32_t getEnLineNo(str source) const {
        return lastToken.getEnLineNo(source);
    }

    uint32_t getEnColNo(str source) const {
        return lastToken.getEnColNo(source);
    }

    void del();

    Expression* castToExpression() { return (Expression*)(this); }
    Statement* castToStatement() { return (Statement*)(this); }
    Type* castToType() { return (Type*)(this); }
    Program* castToProgram() { return (Program*)(this); }
    FunctionParameter* castToFunctionParameter() { return (FunctionParameter*)(this); }
    // Statement Casts
    ExpressionStatement* castToExpressionStatement() { return (ExpressionStatement*)(this); }
    BlockStatement* castToBlockStatement() { return (BlockStatement*)(this); }
    ReturnStatement* castToReturnStatement() { return (ReturnStatement*)(this); }
    RaiseStatement* castToRaiseStatement() { return (RaiseStatement*)(this); }
    FunctionStatement* castToFunctionStatement() { return (FunctionStatement*)(this); }
    IfElseStatement* castToIfElseStatement() { return (IfElseStatement*)(this); }
    WhileStatement* castToWhileStatement() { return (WhileStatement*)(this); }
    ForStatement* castToForStatement() { return (ForStatement*)(this); }
    ForEachStatement* castToForEachStatement() { return (ForEachStatement*)(this); }
    BreakStatement* castToBreakStatement() { return (BreakStatement*)(this); }
    ContinueStatement* castToContinueStatement() { return (ContinueStatement*)(this); }
    ImportStatement* castToImportStatement() { return (ImportStatement*)(this); }
    VariableDeclarationStatement* castToVariableDeclarationStatement() { return (VariableDeclarationStatement*)(this); }
    VariableAssignmentStatement* castToVariableAssignmentStatement() { return (VariableAssignmentStatement*)(this); }
    TryCatchStatement* castToTryCatchStatement() { return (TryCatchStatement*)(this); }
    StructStatement* castToStructStatement() { return (StructStatement*)(this); }
    EnumStatement* castToEnumStatement() { return (EnumStatement*)(this); }
    SwitchCaseStatement* castToSwitchCaseStatement() { return (SwitchCaseStatement*)(this); }
    MacroStatement* castToMacroStatement() { return (MacroStatement*)(this); }
    // Expression Casts
    IdentifierLiteral* castToIdentifierLiteral() { return (IdentifierLiteral*)(this); }
    IntegerLiteral* castToIntegerLiteral() { return (IntegerLiteral*)(this); }
    FloatLiteral* castToFloatLiteral() { return (FloatLiteral*)(this); }
    StringLiteral* castToStringLiteral() { return (StringLiteral*)(this); }
    BooleanLiteral* castToBooleanLiteral() { return (BooleanLiteral*)(this); }
    ArrayLiteral* castToArrayLiteral() { return (ArrayLiteral*)(this); }
    InfixExpression* castToInfixExpression() { return (InfixExpression*)(this); }
    IndexExpression* castToIndexExpression() { return (IndexExpression*)(this); }
    CallExpression* castToCallExpression() { return (CallExpression*)(this); }
};

struct Statement : public Node { using Node::Node; };

struct Expression : public Node { using Node::Node; };

struct Type : public Node {
    Expression* name;
    std::vector<Type*> generics;
    bool refrence;
    inline Type(uint32_t firstToken, token::Token lastToken, Expression* name, const std::vector<Type*>& generics, bool refrence)
        : Node(firstToken, lastToken), name(name), generics(generics), refrence(refrence) { type = NodeType::Type; }
    DefToStr;

    ~Type();
};

struct Program : public Node {
    std::vector<Statement*> statements;
    Program() { type = NodeType::Program; };
    DefToStr;

    ~Program();
};

struct ExpressionStatement : public Statement {
    Expression* expr;
    inline ExpressionStatement(uint32_t firstToken, token::Token lastToken, Expression* expr = nullptr)
        : Statement(firstToken, lastToken), expr(expr) { type = NodeType::ExpressionStatement; }
    DefToStr;

    ~ExpressionStatement();
};

struct BlockStatement : public Statement {
    std::vector<Statement*> statements;
    inline BlockStatement(uint32_t firstToken, token::Token lastToken, const std::vector<Statement*>& statements = {})
        : Statement(firstToken, lastToken), statements(statements) { type = NodeType::BlockStatement; }
    DefToStr;

    ~BlockStatement();
};

struct ReturnStatement : public Statement {
    Expression* value;
    inline ReturnStatement(uint32_t firstToken, token::Token lastToken, Expression* exp = nullptr)
        : Statement(firstToken, lastToken), value(exp) { type = NodeType::ReturnStatement; }
    DefToStr;

    ~ReturnStatement();
};

struct RaiseStatement : public Statement {
    Expression* value;
    inline RaiseStatement(uint32_t firstToken, token::Token lastToken, Expression* exp = nullptr)
        : Statement(firstToken, lastToken), value(exp) { type = NodeType::RaiseStatement; }
    DefToStr;

    ~RaiseStatement();
};

struct FunctionParameter : public Node {
    Expression* name;
    Type* value_type;
    bool constant;
    inline FunctionParameter(uint32_t firstToken, token::Token lastToken, Expression* name, Type* type, bool constant)
        : Node(firstToken, lastToken), name(name), value_type(type), constant(constant) { this->type = NodeType::FunctionParameter; }
    DefToStr;

    ~FunctionParameter();
};

struct FunctionStatement : public Statement {
    Expression* name;
    std::vector<FunctionParameter*> parameters;
    std::vector<FunctionParameter*> closure_parameters;
    Type* return_type;
    bool return_const;
    BlockStatement* body;
    std::vector<Type*> generic;
    bool autocast = false;
    inline FunctionStatement(
        uint32_t firstToken, token::Token lastToken, Expression* name, std::vector<FunctionParameter*> parameters, std::vector<FunctionParameter*> closure_parameters, Type* return_type, bool return_const, BlockStatement* body, const std::vector<Type*>& generic)
        : Statement(firstToken, lastToken), name(name), parameters(parameters), closure_parameters(closure_parameters), return_type(return_type), return_const(return_const), body(body), generic(generic) {
        this->type = NodeType::FunctionStatement;
    }
    DefToStr;

    ~FunctionStatement();
};

struct CallExpression : public Expression {
    IdentifierLiteral* name;
    std::vector<Expression*> arguments;
    std::vector<Expression*> generics;
    bool _new;
    inline CallExpression(uint32_t firstToken, token::Token lastToken, IdentifierLiteral* name, const std::vector<Expression*>& arguments = {})
        : Expression(firstToken, lastToken), name(name), arguments(arguments), _new(false) { type = NodeType::CallExpression; }
    DefToStr;

    ~CallExpression();
};

struct IfElseStatement : public Statement {
    Expression* condition;
    Statement* consequence;
    Statement* alternative;
    inline IfElseStatement(uint32_t firstToken, token::Token lastToken, Expression* condition, Statement* consequence, Statement* alternative = nullptr)
        : Statement(firstToken, lastToken), condition(condition), consequence(consequence), alternative(alternative) { type = NodeType::IfElseStatement; }
    DefToStr;

    ~IfElseStatement();
};

struct WhileStatement : public Statement {
    Expression* condition;
    Statement* body;
    Statement* ifbreak;
    Statement* notbreak;
    inline WhileStatement(uint32_t firstToken, token::Token lastToken, Expression* condition, Statement* body, Statement* ifbreak = nullptr, Statement* notbreak = nullptr)
        : Statement(firstToken, lastToken), condition(condition), body(body), ifbreak(ifbreak), notbreak(notbreak) { type = NodeType::IfElseStatement; }
    DefToStr;

    ~WhileStatement();
};

struct ForStatement : public Statement {
    Statement* init;
    Expression* condition;
    Statement* update;
    Statement* body;
    Statement* ifbreak;
    Statement* notbreak;
    inline ForStatement(uint32_t firstToken, token::Token lastToken, Statement* init, Expression* condition, Statement* update, Statement* body, Statement* ifbreak = nullptr, Statement* notbreak = nullptr)
        : Statement(firstToken, lastToken), init(init), condition(condition), update(update), body(body), ifbreak(ifbreak), notbreak(notbreak) { type = NodeType::ForStatement; }
    DefToStr;

    ~ForStatement();
};

struct ForEachStatement : public Statement {
    Expression* from;
    IdentifierLiteral* get;
    Statement* body;
    Statement* ifbreak;
    Statement* notbreak;
    inline ForEachStatement(uint32_t firstToken, token::Token lastToken, IdentifierLiteral* get, Expression* from, Statement* body, Statement* ifbreak = nullptr, Statement* notbreak = nullptr)
        : Statement(firstToken, lastToken), get(get), from(from), body(body), ifbreak(ifbreak), notbreak(notbreak) { type = NodeType::ForEachStatement; };
    DefToStr;

    ~ForEachStatement();
};

struct BreakStatement : public Statement {
    uint8_t loopIdx;
    uint32_t pos : 24;
    BreakStatement(uint32_t firstToken, token::Token lastToken, uint8_t loopNum)
        : Statement(firstToken, lastToken), loopIdx(loopNum) { type = NodeType::BreakStatement; };
    DefToStr;
};

struct ContinueStatement : public Statement {
  uint8_t loopIdx;
  uint32_t pos : 24;
    ContinueStatement(uint32_t firstToken, token::Token lastToken, int loopNum)
        : Statement(firstToken, lastToken), loopIdx(loopNum) { type = NodeType::ContinueStatement; };
    DefToStr;
};

struct ImportStatement : public Statement {
    std::string relativePath;
    std::string as;
    ImportStatement(uint32_t firstToken, token::Token lastToken, const std::string& relativePath, const std::string& as)
        : Statement(firstToken, lastToken), relativePath(relativePath), as(as) { type = NodeType::ImportStatement; }
    DefToStr;
};

struct VariableDeclarationStatement : public Statement {
    Expression* name;
    Type* value_type;
    Expression* value;
    bool is_volatile = false;
    bool is_const = false;
    inline VariableDeclarationStatement(uint32_t firstToken, token::Token lastToken, Expression* name, Type* type, Expression* value = nullptr, bool is_volatile = false, bool is_const = false)
        : Statement(firstToken, lastToken), name(name), value_type(type), value(value), is_volatile(is_volatile), is_const(is_const) { this->type = NodeType::VariableDeclarationStatement; };
    DefToStr;

    ~VariableDeclarationStatement();
};

struct VariableAssignmentStatement : public Statement {
    Expression* name;
    Expression* value;
    inline VariableAssignmentStatement(uint32_t firstToken, token::Token lastToken, Expression* name, Expression* value)
        : Statement(firstToken, lastToken), name(name), value(value) { type = NodeType::VariableAssignmentStatement; };
    DefToStr;

    ~VariableAssignmentStatement();
};

struct TryCatchStatement : public Statement {
    Statement* try_block;
    std::vector<std::tuple<Type*, IdentifierLiteral*, Statement*>> catch_blocks;
    inline TryCatchStatement(uint32_t firstToken, token::Token lastToken, Statement* try_block, std::vector<std::tuple<Type*, IdentifierLiteral*, Statement*>> catch_blocks)
        : Statement(firstToken, lastToken), try_block(try_block), catch_blocks(catch_blocks) { type = NodeType::TryCatchStatement; };
    DefToStr;

    ~TryCatchStatement();
};

struct SwitchCaseStatement : public Statement {
    Expression* condition;
    std::vector<std::tuple<Expression*, Statement*>> cases;
    Statement* other;
    inline SwitchCaseStatement(uint32_t firstToken, token::Token lastToken, Expression* condition, std::vector<std::tuple<Expression*, Statement*>> cases, Statement* other = nullptr)
        : Statement(firstToken, lastToken), condition(condition), cases(cases), other(other) { type = NodeType::SwitchCaseStatement; };
    DefToStr;

    ~SwitchCaseStatement();
};

struct InfixExpression : public Expression {
    Expression* left;
    Expression* right;
    token::TokenType op;
    inline InfixExpression(uint32_t firstToken, token::Token lastToken, Expression* left, token::TokenType op, const std::string& literal, Expression* right = nullptr)
        : Expression(firstToken, lastToken), left(left), right(right), op(op) { type = NodeType::InfixedExpression; };
    DefToStr;

    ~InfixExpression();
};

struct IndexExpression : public Expression {
    Expression* left;
    Expression* index;
    inline IndexExpression(uint32_t firstToken, token::Token lastToken, Expression* left, Expression* index)
        : Expression(firstToken, lastToken), left(left), index(index) { type = NodeType::IndexExpression; }
    DefToStr;

    ~IndexExpression();
};

struct IntegerLiteral : public Expression {
    int64_t value;
    inline IntegerLiteral(uint32_t firstToken, token::Token lastToken, int64_t value)
        : Expression(firstToken, lastToken), value(value) { type = NodeType::IntegerLiteral; };
    DefToStr;
};

struct FloatLiteral : public Expression {
    double value;
    inline FloatLiteral(uint32_t firstToken, token::Token lastToken, double value)
        : Expression(firstToken, lastToken), value(value) { type = NodeType::FloatLiteral; };
    DefToStr;
};

struct StringLiteral : public Expression {
    std::string value;
    inline StringLiteral(uint32_t firstToken, token::Token lastToken, const std::string& value)
        : Expression(firstToken, lastToken), value(value) { type = NodeType::StringLiteral; };
    DefToStr;
};

struct IdentifierLiteral : public Expression {
    std::string value;
    inline IdentifierLiteral(uint32_t firstToken, token::Token lastToken, std::string value)
        : Expression(firstToken, lastToken), value(value) { type = NodeType::IdentifierLiteral; };
    DefToStr;
};

struct BooleanLiteral : public Expression {
    bool value;
    inline BooleanLiteral(uint32_t firstToken, token::Token lastToken, bool value)
        : Expression(firstToken, lastToken), value(value) { type = NodeType::BooleanLiteral; };
    DefToStr;
};

struct StructStatement : public Statement {
    Expression* name = nullptr;
    std::vector<Statement*> fields = {};
    std::vector<Type*> generics = {};
    inline StructStatement(uint32_t firstToken, token::Token lastToken, Expression* name, const std::vector<Statement*>& fields)
        : Statement(firstToken, lastToken), name(name), fields(fields) { type = NodeType::StructStatement; };
    DefToStr;

    ~StructStatement();
};

struct EnumStatement : public Statement {
    Expression* name = nullptr;
    std::vector<std::string> fields = {};
    inline EnumStatement(uint32_t firstToken, token::Token lastToken, Expression* name, const std::vector<std::string>& fields)
        : Statement(firstToken, lastToken), name(name), fields(fields) { type = NodeType::EnumStatement; }
    ~EnumStatement();
    DefToStr;
};

struct MacroStatement : public Statement {
    std::string name;
    BlockStatement* body;
    inline MacroStatement(std::string name, BlockStatement* body)
        : name(name), body(body) { type = NodeType::MacroStatement; };
    DefToStr;

    ~MacroStatement();
};

struct ArrayLiteral : public Expression {
    std::vector<Expression*> elements;
    bool _new;
    inline ArrayLiteral(uint32_t firstToken, token::Token lastToken, const std::vector<Expression*>& elements, bool _new = false)
        : Expression(firstToken, lastToken), elements(elements), _new(_new) { type = NodeType::ArrayLiteral; };
    DefToStr;

    ~ArrayLiteral();
};

} // namespace AST

#endif // AST_HPP
