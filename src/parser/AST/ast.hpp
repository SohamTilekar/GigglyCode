#ifndef AST_HPP
#define AST_HPP
#include <any>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../../lexer/token.hpp"

namespace AST {
class Node;
class Statement;
class Type;
class GenericType;
class Program;
class FunctionParameter;
class Expression;
class IdentifierLiteral;
class IntegerLiteral;
class FloatLiteral;
class StringLiteral;
class BooleanLiteral;
class ArrayLiteral;
class InfixExpression;
class IndexExpression;
class CallExpression;
class ExpressionStatement;
class BlockStatement;
class ReturnStatement;
class RaiseStatement;
class FunctionStatement;
class IfElseStatement;
class WhileStatement;
class ForStatement;
class BreakStatement;
class ContinueStatement;
class ImportStatement;
class VariableDeclarationStatement;
class VariableAssignmentStatement;
class TryCatchStatement;
class StructStatement;
class SwitchCaseStatement;

enum class NodeType {
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
    BreakStatement,
    ContinueStatement,
    StructStatement,
    ImportStatement,
    TryCatchStatement,
    RaiseStatement,
    SwitchCaseStatement,

    // Types
    Type,
    GenericType,

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

struct MetaData {
    int st_line_no = -1;
    int st_col_no = -1;
    int end_line_no = -1;
    int end_col_no = -1;
    std::unordered_map<std::string, std::variant<int, std::string, std::tuple<int, int>>> more_data = {};
};

class Node {
  public:
    MetaData meta_data;
    std::unordered_map<std::string, std::any> extra_info;

    inline void set_meta_data(int st_line_num, int st_col_num, int end_line_num, int end_col_num) {
        if (this->type() == NodeType::IdentifierLiteral) return;
        meta_data.st_line_no = st_line_num;
        meta_data.st_col_no = st_col_num;
        meta_data.end_line_no = end_line_num;
        meta_data.end_col_no = end_col_num;
    }

    virtual inline NodeType type() { return NodeType::Unknown; }

    virtual inline std::string toStr() {
        std::string yaml = "type: " + nodeTypeToString(this->type()) + "\n";
        return yaml;
    }

    Expression* castToExpression() { return (Expression*)(this); }
    Statement* castToStatement() { return (Statement*)(this); }
    Type* castToType() { return (Type*)(this); }
    GenericType* castToGenericType() { return (GenericType*)(this); }
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
    BreakStatement* castToBreakStatement() { return (BreakStatement*)(this); }
    ContinueStatement* castToContinueStatement() { return (ContinueStatement*)(this); }
    ImportStatement* castToImportStatement() { return (ImportStatement*)(this); }
    VariableDeclarationStatement* castToVariableDeclarationStatement() { return (VariableDeclarationStatement*)(this); }
    VariableAssignmentStatement* castToVariableAssignmentStatement() { return (VariableAssignmentStatement*)(this); }
    TryCatchStatement* castToTryCatchStatement() { return (TryCatchStatement*)(this); }
    StructStatement* castToStructStatement() { return (StructStatement*)(this); }
    SwitchCaseStatement* castToSwitchCaseStatement() { return (SwitchCaseStatement*)(this); }
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

    // Destructor Declaration
    virtual ~Node() = default;
};

class Statement : public Node {
  public:
    // Destructor Declaration
    virtual ~Statement() = default;
};

class Expression : public Node {
  public:
    // Destructor Declaration
    virtual ~Expression() = default;
};

class Type : public Node {
  public:
    Expression* name;
    std::vector<Type*> generics;
    bool refrence;
    inline Type(Expression* name, const std::vector<Type*>& generics, bool refrence) : name(name), generics(generics), refrence(refrence) {}
    NodeType type() override { return NodeType::Type; };
    std::string toStr() override;

    // Destructor Declaration
    ~Type() override;
};

class GenericType : public Node {
  public:
    Expression* name;
    std::vector<Type*> generic_union;
    inline GenericType(Expression* name, const std::vector<Type*>& generic_union) : name(name), generic_union(generic_union) {}
    inline NodeType type() override { return NodeType::GenericType; };
    std::string toStr() override;

    // Destructor Declaration
    ~GenericType() override;
};

class Program : public Node {
  public:
    std::vector<Statement*> statements;
    inline NodeType type() override { return NodeType::Program; };
    std::string toStr() override;

    // Destructor Declaration
    ~Program() override;
};

class ExpressionStatement : public Statement {
  public:
    Expression* expr;
    inline ExpressionStatement(Expression* expr = nullptr) : expr(expr) {}
    inline NodeType type() override { return NodeType::ExpressionStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~ExpressionStatement() override;
};

class BlockStatement : public Statement {
  public:
    std::vector<Statement*> statements;
    inline NodeType type() override { return NodeType::BlockStatement; };
    inline BlockStatement(const std::vector<Statement*>& statements = {}) : statements(statements) {}
    std::string toStr() override;

    // Destructor Declaration
    ~BlockStatement() override;
};

class ReturnStatement : public Statement {
  public:
    Expression* value;
    inline ReturnStatement(Expression* exp = nullptr) : value(exp) {}
    inline NodeType type() override { return NodeType::ReturnStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~ReturnStatement() override;
};

class RaiseStatement : public Statement {
  public:
    Expression* value;
    inline RaiseStatement(Expression* exp = nullptr) : value(exp) {}
    inline NodeType type() override { return NodeType::RaiseStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~RaiseStatement() override;
};

class FunctionParameter : public Node {
  public:
    Expression* name;
    Type* value_type;
    inline FunctionParameter(Expression* name, Type* type) : name(name), value_type(type) {}
    inline NodeType type() override { return NodeType::FunctionParameter; };
    std::string toStr() override;

    // Destructor Declaration
    ~FunctionParameter() override;
};

class FunctionStatement : public Statement {
  public:
    Expression* name;
    std::vector<FunctionParameter*> parameters;
    std::vector<FunctionParameter*> closure_parameters;
    Type* return_type;
    BlockStatement* body;
    std::vector<GenericType*> generic;
    inline FunctionStatement(Expression* name,
                             std::vector<FunctionParameter*> parameters,
                             std::vector<FunctionParameter*> closure_parameters,
                             Type* return_type,
                             BlockStatement* body,
                             const std::vector<GenericType*>& generic)
        : name(name), parameters(parameters), closure_parameters(closure_parameters), return_type(return_type), body(body), generic(generic) {
        this->extra_info["autocast"] = false;
    }
    inline NodeType type() override { return NodeType::FunctionStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~FunctionStatement() override;
};

class CallExpression : public Expression {
  public:
    Expression* name;
    std::vector<Expression*> arguments;
    std::vector<Expression*> generics;
    bool _new;
    inline CallExpression(Expression* name, const std::vector<Expression*>& arguments = {}) : name(name), arguments(arguments), _new(false) {}
    inline NodeType type() override { return NodeType::CallExpression; };
    std::string toStr() override;

    // Destructor Declaration
    ~CallExpression() override;
};

class IfElseStatement : public Statement {
  public:
    Expression* condition;
    Statement* consequence;
    Statement* alternative;
    inline IfElseStatement(Expression* condition, Statement* consequence, Statement* alternative = nullptr) : condition(condition), consequence(consequence), alternative(alternative) {}
    inline NodeType type() override { return NodeType::IfElseStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~IfElseStatement() override;
};

class WhileStatement : public Statement {
  public:
    Expression* condition;
    Statement* body;
    Statement* ifbreak;
    Statement* notbreak;
    inline WhileStatement(Expression* condition, Statement* body, Statement* ifbreak = nullptr, Statement* notbreak = nullptr)
        : condition(condition), body(body), ifbreak(ifbreak), notbreak(notbreak) {}
    inline NodeType type() override { return NodeType::WhileStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~WhileStatement() override;
};

class ForStatement : public Statement {
  public:
    Expression* from;
    IdentifierLiteral* get;
    Statement* body;
    Statement* ifbreak;
    Statement* notbreak;
    inline ForStatement(IdentifierLiteral* get, Expression* from, Statement* body, Statement* ifbreak = nullptr, Statement* notbreak = nullptr)
        : get(get), from(from), body(body), ifbreak(ifbreak), notbreak(notbreak) {}
    inline NodeType type() override { return NodeType::ForStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~ForStatement() override;
};

class BreakStatement : public Statement {
  public:
    unsigned short loopIdx = 0;
    inline NodeType type() override { return NodeType::BreakStatement; };
    BreakStatement(int loopNum) : loopIdx(loopNum) {};
    std::string toStr() override;
};

class ContinueStatement : public Statement {
  public:
    unsigned short loopIdx = 0;
    inline NodeType type() override { return NodeType::ContinueStatement; };
    ContinueStatement(int loopNum) : loopIdx(loopNum) {};
    std::string toStr() override;
};

class ImportStatement : public Statement {
  public:
    std::string relativePath;
    inline NodeType type() override { return NodeType::ImportStatement; }
    ImportStatement(const std::string& relativePath) : relativePath(relativePath) {}
    std::string toStr() override;
};

class VariableDeclarationStatement : public Statement {
  public:
    Expression* name;
    Type* value_type;
    Expression* value;
    bool is_volatile = false;
    inline VariableDeclarationStatement(Expression* name, Type* type, Expression* value = nullptr, bool is_volatile = true) : name(name), value_type(type), value(value), is_volatile(is_volatile) {}
    inline NodeType type() override { return NodeType::VariableDeclarationStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~VariableDeclarationStatement() override;
};

class VariableAssignmentStatement : public Statement {
  public:
    Expression* name;
    Expression* value;
    inline VariableAssignmentStatement(Expression* name, Expression* value) : name(name), value(value) {}
    inline NodeType type() override { return NodeType::VariableAssignmentStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~VariableAssignmentStatement() override;
};

class TryCatchStatement : public Statement {
  public:
    Statement* try_block;
    std::vector<std::tuple<Type*, IdentifierLiteral*, Statement*>> catch_blocks;
    inline TryCatchStatement(Statement* try_block, std::vector<std::tuple<Type*, IdentifierLiteral*, Statement*>> catch_blocks) : try_block(try_block), catch_blocks(catch_blocks) {}
    inline NodeType type() override { return NodeType::TryCatchStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~TryCatchStatement() override;
};

class SwitchCaseStatement : public Statement {
  public:
    Expression* condition;
    std::vector<std::tuple<Expression*, Statement*>> cases;
    Statement* other;
    inline SwitchCaseStatement(Expression* condition, std::vector<std::tuple<Expression*, Statement*>> cases, Statement* other = nullptr) : condition(condition), cases(cases), other(other) {};
    inline NodeType type() override { return NodeType::SwitchCaseStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~SwitchCaseStatement() override;
};

class InfixExpression : public Expression {
  public:
    Expression* left;
    Expression* right;
    token::TokenType op;
    inline InfixExpression(Expression* left, token::TokenType op, const std::string& literal, Expression* right = nullptr) : left(left), right(right), op(op) {
        this->meta_data.more_data["operator_literal"] = literal;
    }
    inline NodeType type() override { return NodeType::InfixedExpression; };
    std::string toStr() override;

    // Destructor Declaration
    ~InfixExpression() override;
};

class IndexExpression : public Expression {
  public:
    Expression* left;
    Expression* index;
    inline IndexExpression(Expression* left, Expression* index) : left(left), index(index) {}
    inline IndexExpression(Expression* left) : left(left), index(nullptr) {}
    inline NodeType type() override { return NodeType::IndexExpression; };
    std::string toStr() override;

    // Destructor Declaration
    ~IndexExpression() override;
};

class IntegerLiteral : public Expression {
  public:
    long long int value;
    inline IntegerLiteral(long long int value) : value(value) {}
    inline NodeType type() override { return NodeType::IntegerLiteral; };
    std::string toStr() override;

    // Destructor Declaration
    ~IntegerLiteral() override;
};

class FloatLiteral : public Expression {
  public:
    double value;
    inline FloatLiteral(double value) : value(value) {}
    inline NodeType type() override { return NodeType::FloatLiteral; };
    std::string toStr() override;

    // Destructor Declaration
    ~FloatLiteral() override;
};

class StringLiteral : public Expression {
  public:
    std::string value;
    inline StringLiteral(const std::string& value) : value(value) { this->meta_data.more_data["length"] = int(value.length()); }
    inline NodeType type() override { return NodeType::StringLiteral; };
    std::string toStr() override;

    // Destructor Declaration
    ~StringLiteral() override;
};

class IdentifierLiteral : public Expression {
  public:
    std::string value;
    inline IdentifierLiteral(token::Token value) {
        this->value = value.literal;
        this->meta_data.st_line_no = value.line_no;
        this->meta_data.end_line_no = value.line_no;
        this->meta_data.st_col_no = value.col_no;
        this->meta_data.end_col_no = value.end_col_no;
    }
    inline NodeType type() override { return NodeType::IdentifierLiteral; };
    std::string toStr() override;

    // Destructor Declaration
    ~IdentifierLiteral() override;
};

class BooleanLiteral : public Expression {
  public:
    bool value;
    inline BooleanLiteral(bool value) : value(value) {}
    inline NodeType type() override { return NodeType::BooleanLiteral; };
    std::string toStr() override;

    // Destructor Declaration
    ~BooleanLiteral() override;
};

class StructStatement : public Statement {
  public:
    Expression* name = nullptr;
    std::vector<Statement*> fields = {};
    std::vector<GenericType*> generics = {};
    inline StructStatement(Expression* name, const std::vector<Statement*>& fields) : name(name), fields(fields) {}
    inline NodeType type() override { return NodeType::StructStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~StructStatement() override;
};

class ArrayLiteral : public Expression {
  public:
    std::vector<Expression*> elements;
    bool _new;
    inline ArrayLiteral(const std::vector<Expression*>& elements, bool _new = false) : elements(elements), _new(_new) {}
    inline NodeType type() override { return NodeType::ArrayLiteral; };
    std::string toStr() override;

    // Destructor Declaration
    ~ArrayLiteral() override;
};

} // namespace AST

#endif // AST_HPP