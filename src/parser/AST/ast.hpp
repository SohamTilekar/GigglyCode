#ifndef AST_HPP
#define AST_HPP
#include <any>
#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <variant>
#include <vector>

#include "../../include/json.hpp"
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

using std::shared_ptr;
using NodePtr = shared_ptr<Node>;
using ExpressionPtr = shared_ptr<Expression>;
using StatementPtr = shared_ptr<Statement>;
using Json = nlohmann::json;
using JsonPtr = shared_ptr<Json>;
using std::make_shared;

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

class Node : public std::enable_shared_from_this<Node> {
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

    virtual inline JsonPtr toJSON() {
        auto json = Json();
        json["type"] = nodeTypeToString(this->type());
        return make_shared<Json>(json);
    }

    std::shared_ptr<Expression> castToExpression() { return std::static_pointer_cast<Expression>(shared_from_this()); }
    std::shared_ptr<Statement> castToStatement() { return std::static_pointer_cast<Statement>(shared_from_this()); }
    std::shared_ptr<Type> castToType() { return std::static_pointer_cast<Type>(shared_from_this()); }
    std::shared_ptr<GenericType> castToGenericType() { return std::static_pointer_cast<GenericType>(shared_from_this()); }
    std::shared_ptr<Program> castToProgram() { return std::static_pointer_cast<Program>(shared_from_this()); }
    std::shared_ptr<FunctionParameter> castToFunctionParameter() { return std::static_pointer_cast<FunctionParameter>(shared_from_this()); }
    // Statemnt Casts
    std::shared_ptr<ExpressionStatement> castToExpressionStatement() { return std::static_pointer_cast<ExpressionStatement>(shared_from_this()); }
    std::shared_ptr<BlockStatement> castToBlockStatement() { return std::static_pointer_cast<BlockStatement>(shared_from_this()); }
    std::shared_ptr<ReturnStatement> castToReturnStatement() { return std::static_pointer_cast<ReturnStatement>(shared_from_this()); }
    std::shared_ptr<RaiseStatement> castToRaiseStatement() { return std::static_pointer_cast<RaiseStatement>(shared_from_this()); }
    std::shared_ptr<FunctionStatement> castToFunctionStatement() { return std::static_pointer_cast<FunctionStatement>(shared_from_this()); }
    std::shared_ptr<IfElseStatement> castToIfElseStatement() { return std::static_pointer_cast<IfElseStatement>(shared_from_this()); }
    std::shared_ptr<WhileStatement> castToWhileStatement() { return std::static_pointer_cast<WhileStatement>(shared_from_this()); }
    std::shared_ptr<ForStatement> castToForStatement() { return std::static_pointer_cast<ForStatement>(shared_from_this()); }
    std::shared_ptr<BreakStatement> castToBreakStatement() { return std::static_pointer_cast<BreakStatement>(shared_from_this()); }
    std::shared_ptr<ContinueStatement> castToContinueStatement() { return std::static_pointer_cast<ContinueStatement>(shared_from_this()); }
    std::shared_ptr<ImportStatement> castToImportStatement() { return std::static_pointer_cast<ImportStatement>(shared_from_this()); }
    std::shared_ptr<VariableDeclarationStatement> castToVariableDeclarationStatement() { return std::static_pointer_cast<VariableDeclarationStatement>(shared_from_this()); }
    std::shared_ptr<VariableAssignmentStatement> castToVariableAssignmentStatement() { return std::static_pointer_cast<VariableAssignmentStatement>(shared_from_this()); }
    std::shared_ptr<TryCatchStatement> castToTryCatchStatement() { return std::static_pointer_cast<TryCatchStatement>(shared_from_this()); }
    std::shared_ptr<StructStatement> castToStructStatement() { return std::static_pointer_cast<StructStatement>(shared_from_this()); }
    // Expression Casts
    std::shared_ptr<IdentifierLiteral> castToIdentifierLiteral() { return std::static_pointer_cast<IdentifierLiteral>(shared_from_this()); }
    std::shared_ptr<IntegerLiteral> castToIntegerLiteral() { return std::static_pointer_cast<IntegerLiteral>(shared_from_this()); }
    std::shared_ptr<FloatLiteral> castToFloatLiteral() { return std::static_pointer_cast<FloatLiteral>(shared_from_this()); }
    std::shared_ptr<StringLiteral> castToStringLiteral() { return std::static_pointer_cast<StringLiteral>(shared_from_this()); }
    std::shared_ptr<BooleanLiteral> castToBooleanLiteral() { return std::static_pointer_cast<BooleanLiteral>(shared_from_this()); }
    std::shared_ptr<ArrayLiteral> castToArrayLiteral() { return std::static_pointer_cast<ArrayLiteral>(shared_from_this()); }
    std::shared_ptr<InfixExpression> castToInfixExpression() { return std::static_pointer_cast<InfixExpression>(shared_from_this()); }
    std::shared_ptr<IndexExpression> castToIndexExpression() { return std::static_pointer_cast<IndexExpression>(shared_from_this()); }
    std::shared_ptr<CallExpression> castToCallExpression() { return std::static_pointer_cast<CallExpression>(shared_from_this()); }
};

class Statement : public Node {};

class Expression : public Node {};

class Type : public Node {
  public:
    ExpressionPtr name;
    std::vector<shared_ptr<Type>> generics;
    bool refrence;
    inline Type(ExpressionPtr name, const std::vector<shared_ptr<Type>>& generics, bool refrence) : name(name), generics(generics), refrence(refrence) {}
    inline NodeType type() override { return NodeType::Type; };
    JsonPtr toJSON() override;
};

class GenericType : public Node {
  public:
    ExpressionPtr name;
    std::vector<shared_ptr<Type>> generic_union;
    inline GenericType(ExpressionPtr name, const std::vector<shared_ptr<Type>>& generic_union) : name(name), generic_union(generic_union) {}
    inline NodeType type() override { return NodeType::GenericType; };
    JsonPtr toJSON() override;
};

class Program : public Node {
  public:
    std::vector<StatementPtr> statements;
    inline NodeType type() override { return NodeType::Program; };
    JsonPtr toJSON() override;
};

class ExpressionStatement : public Statement {
  public:
    ExpressionPtr expr;
    inline ExpressionStatement(ExpressionPtr expr = nullptr) : expr(expr) {}
    inline NodeType type() override { return NodeType::ExpressionStatement; };
    JsonPtr toJSON() override;
};

class BlockStatement : public Statement {
  public:
    std::vector<StatementPtr> statements;
    inline NodeType type() override { return NodeType::BlockStatement; };
    inline BlockStatement(const std::vector<StatementPtr>& statements = {}) : statements(statements) {}
    JsonPtr toJSON() override;
};

class ReturnStatement : public Statement {
  public:
    ExpressionPtr value;
    inline ReturnStatement(ExpressionPtr exp = nullptr) : value(exp) {}
    inline NodeType type() override { return NodeType::ReturnStatement; };
    JsonPtr toJSON() override;
};

class RaiseStatement : public Statement {
  public:
    ExpressionPtr value;
    inline RaiseStatement(ExpressionPtr exp = nullptr) : value(exp) {}
    inline NodeType type() override { return NodeType::RaiseStatement; };
    JsonPtr toJSON() override;
};

class FunctionParameter : public Node {
  public:
    ExpressionPtr name;
    shared_ptr<Type> value_type;
    inline FunctionParameter(ExpressionPtr name, shared_ptr<Type> type) : name(name), value_type(type) {}
    inline NodeType type() override { return NodeType::FunctionParameter; };
    JsonPtr toJSON() override;
};

class FunctionStatement : public Statement {
  public:
    ExpressionPtr name;
    std::vector<shared_ptr<FunctionParameter>> parameters;
    std::vector<shared_ptr<FunctionParameter>> closure_parameters;
    shared_ptr<Type> return_type;
    shared_ptr<BlockStatement> body;
    std::vector<shared_ptr<GenericType>> generic;
    inline FunctionStatement(ExpressionPtr name,
                             std::vector<shared_ptr<FunctionParameter>> parameters,
                             std::vector<shared_ptr<FunctionParameter>> closure_parameters,
                             shared_ptr<Type> return_type,
                             shared_ptr<BlockStatement> body,
                             const std::vector<shared_ptr<GenericType>>& generic)
        : name(name), parameters(parameters), closure_parameters(closure_parameters), return_type(return_type), body(body), generic(generic) {
        this->extra_info["autocast"] = false;
    }
    inline NodeType type() override { return NodeType::FunctionStatement; };
    JsonPtr toJSON() override;
};

class CallExpression : public Expression {
  public:
    ExpressionPtr name;
    std::vector<ExpressionPtr> arguments;
    std::vector<ExpressionPtr> generics;
    bool _new;
    inline CallExpression(ExpressionPtr name, const std::vector<ExpressionPtr>& arguments = {}) : name(name), arguments(arguments) {}
    inline NodeType type() override { return NodeType::CallExpression; };
    JsonPtr toJSON() override;
};

class IfElseStatement : public Statement {
  public:
    ExpressionPtr condition;
    StatementPtr consequence;
    StatementPtr alternative;
    inline IfElseStatement(ExpressionPtr condition, StatementPtr consequence, StatementPtr alternative = nullptr) : condition(condition), consequence(consequence), alternative(alternative) {}
    inline NodeType type() override { return NodeType::IfElseStatement; };
    JsonPtr toJSON() override;
};

class WhileStatement : public Statement {
  public:
    ExpressionPtr condition;
    StatementPtr body;
    StatementPtr ifbreak;
    StatementPtr notbreak;
    inline WhileStatement(ExpressionPtr condition, StatementPtr body, StatementPtr ifbreak = nullptr, StatementPtr notbreak = nullptr)
        : condition(condition), body(body), ifbreak(ifbreak), notbreak(notbreak) {}
    inline NodeType type() override { return NodeType::WhileStatement; };
    JsonPtr toJSON() override;
};

class ForStatement : public Statement {
  public:
    ExpressionPtr from;
    shared_ptr<IdentifierLiteral> get;
    StatementPtr body;
    StatementPtr ifbreak;
    StatementPtr notbreak;
    inline ForStatement(shared_ptr<IdentifierLiteral> get, ExpressionPtr from, StatementPtr body, StatementPtr ifbreak = nullptr, StatementPtr notbreak = nullptr)
        : get(get), from(from), body(body), ifbreak(ifbreak), notbreak(notbreak) {}
    inline NodeType type() override { return NodeType::ForStatement; };
    JsonPtr toJSON() override;
};

class BreakStatement : public Statement {
  public:
    unsigned short loopIdx = 0;
    inline NodeType type() override { return NodeType::BreakStatement; };
    BreakStatement(int loopNum) : loopIdx(loopNum) {};
    JsonPtr toJSON() override;
};

class ContinueStatement : public Statement {
  public:
    unsigned short loopIdx = 0;
    inline NodeType type() override { return NodeType::ContinueStatement; };
    ContinueStatement(int loopNum) : loopIdx(loopNum) {};
    JsonPtr toJSON() override;
};

class ImportStatement : public Statement {
  public:
    std::string relativePath;
    inline NodeType type() override { return NodeType::ImportStatement; }
    ImportStatement(const std::string& relativePath) : relativePath(relativePath) {}
    JsonPtr toJSON() override;
};

class VariableDeclarationStatement : public Statement {
  public:
    ExpressionPtr name;
    shared_ptr<Type> value_type;
    ExpressionPtr value;
    bool is_volatile = false;
    inline VariableDeclarationStatement(ExpressionPtr name, shared_ptr<Type> type, ExpressionPtr value = nullptr, bool is_volatile = true)
        : name(name), value_type(type), value(value), is_volatile(is_volatile) {}
    inline NodeType type() override { return NodeType::VariableDeclarationStatement; };
    JsonPtr toJSON() override;
};

class VariableAssignmentStatement : public Statement {
  public:
    ExpressionPtr name;
    ExpressionPtr value;
    inline VariableAssignmentStatement(ExpressionPtr name, ExpressionPtr value) : name(name), value(value) {}
    inline NodeType type() override { return NodeType::VariableAssignmentStatement; };
    JsonPtr toJSON() override;
};

class TryCatchStatement : public Statement {
  public:
    StatementPtr try_block;
    std::vector<std::tuple<shared_ptr<Type>, shared_ptr<IdentifierLiteral>, StatementPtr>> catch_blocks;
    inline TryCatchStatement(StatementPtr try_block, std::vector<std::tuple<shared_ptr<Type>, shared_ptr<IdentifierLiteral>, StatementPtr>> catch_blocks)
        : try_block(try_block), catch_blocks(catch_blocks) {}
    inline NodeType type() override { return NodeType::TryCatchStatement; };
    JsonPtr toJSON() override;
};

class InfixExpression : public Expression {
  public:
    ExpressionPtr left;
    ExpressionPtr right;
    token::TokenType op;
    inline InfixExpression(ExpressionPtr left, token::TokenType op, const std::string& literal, ExpressionPtr right = nullptr) : left(left), right(right), op(op) {
        this->meta_data.more_data["operator_literal"] = literal;
    }
    inline NodeType type() override { return NodeType::InfixedExpression; };
    JsonPtr toJSON() override;
};

class IndexExpression : public Expression {
  public:
    ExpressionPtr left;
    ExpressionPtr index;
    inline IndexExpression(ExpressionPtr left, ExpressionPtr index) : left(left), index(index) {}
    inline IndexExpression(ExpressionPtr left) : left(left), index(nullptr) {}
    inline NodeType type() override { return NodeType::IndexExpression; };
    JsonPtr toJSON() override;
};

class IntegerLiteral : public Expression {
  public:
    long long int value;
    inline IntegerLiteral(long long int value) : value(value) {}
    inline NodeType type() override { return NodeType::IntegerLiteral; };
    JsonPtr toJSON() override;
};

class FloatLiteral : public Expression {
  public:
    double value;
    inline FloatLiteral(double value) : value(value) {}
    inline NodeType type() override { return NodeType::FloatLiteral; };
    JsonPtr toJSON() override;
};

class StringLiteral : public Expression {
  public:
    std::string value;
    inline StringLiteral(const std::string& value) : value(value) { this->meta_data.more_data["length"] = int(value.length()); }
    inline NodeType type() override { return NodeType::StringLiteral; };
    JsonPtr toJSON() override;
};

class IdentifierLiteral : public Expression {
  public:
    std::string value;
    inline IdentifierLiteral(shared_ptr<token::Token> value) {
        this->value = value->literal;
        this->meta_data.st_line_no = value->line_no;
        this->meta_data.end_line_no = value->line_no;
        this->meta_data.st_col_no = value->col_no;
        this->meta_data.end_col_no = value->end_col_no;
    }
    inline NodeType type() override { return NodeType::IdentifierLiteral; };
    JsonPtr toJSON() override;
};

class BooleanLiteral : public Expression {
  public:
    bool value;
    inline BooleanLiteral(bool value) : value(value) {}
    inline NodeType type() override { return NodeType::BooleanLiteral; };
    JsonPtr toJSON() override;
};

class StructStatement : public Statement {
  public:
    ExpressionPtr name = nullptr;
    std::vector<StatementPtr> fields = {};
    std::vector<shared_ptr<GenericType>> generics = {};
    inline StructStatement(ExpressionPtr name, const std::vector<StatementPtr>& fields) : name(name), fields(fields) {}
    inline NodeType type() override { return NodeType::StructStatement; };
    JsonPtr toJSON() override;
};

class ArrayLiteral : public Expression {
  public:
    std::vector<ExpressionPtr> elements;
    bool _new;
    inline ArrayLiteral(const std::vector<ExpressionPtr>& elements, bool _new = false) : elements(elements), _new(_new) {}
    inline NodeType type() override { return NodeType::ArrayLiteral; };
    JsonPtr toJSON() override;
};

} // namespace AST
#endif // AST_HPP
