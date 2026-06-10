#ifndef AST_HPP
#define AST_HPP
#include <string>
#include <unordered_map>
#include <vector>
#include <memory>

#include "../../lexer/token.hpp"

namespace AST {
class Node;
class Statement;
class Type;
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
class ForEachStatement;
class ForStatement;
class BreakStatement;
class ContinueStatement;
class ImportStatement;
class VariableDeclarationStatement;
class VariableAssignmentStatement;
class TryCatchStatement;
class StructStatement;
class EnumStatement;
class SwitchCaseStatement;
class MacroStatement;

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

template <typename T>
class ASTUniquePtr {
  private:
    std::shared_ptr<T> ptr;

  public:
    inline ASTUniquePtr() = default;
    inline ASTUniquePtr(T* p) : ptr(p) {}
    inline ASTUniquePtr(std::shared_ptr<T>&& p) : ptr(std::move(p)) {}
    inline ASTUniquePtr(const ASTUniquePtr& other) = default;
    inline ASTUniquePtr& operator=(const ASTUniquePtr& other) = default;
    inline ASTUniquePtr(ASTUniquePtr&& other) noexcept = default;
    inline ASTUniquePtr& operator=(ASTUniquePtr&& other) noexcept = default;
    inline ASTUniquePtr& operator=(T* p) {
        ptr.reset(p);
        return *this;
    }
    inline ASTUniquePtr& operator=(std::nullptr_t) {
        ptr.reset();
        return *this;
    }

    // Implicit conversion to raw pointer
    inline operator T*() const { return ptr.get(); }
    inline T* operator->() const { return ptr.get(); }
    inline T& operator*() const { return *ptr; }
    inline T* get() const { return ptr.get(); }
    inline void reset(T* p = nullptr) { ptr.reset(p); }
    inline explicit operator bool() const { return static_cast<bool>(ptr); }

    inline bool operator==(std::nullptr_t) const { return ptr == nullptr; }
    inline bool operator!=(std::nullptr_t) const { return ptr != nullptr; }
};

struct MoreData {
    std::unordered_map<std::string, int> int_map = {};
    std::unordered_map<std::string, std::string> str_map = {};
    std::unordered_map<std::string, std::tuple<int, int>> pos_map = {};
    std::unordered_map<std::string, bool> bool_map = {};

    MoreData() = default;
    MoreData(std::unordered_map<std::string, int> int_map) : int_map(int_map) {}
    MoreData(std::unordered_map<std::string, std::string> str_map) : str_map(str_map) {}
    MoreData(std::unordered_map<std::string, bool> bool_map) : bool_map(bool_map) {}
    MoreData(std::unordered_map<std::string, std::tuple<int, int>> pos_map) : pos_map(pos_map) {}

    template <typename T> void insert(const std::string& key, const T& value) {
        if constexpr (std::is_same_v<T, int>) {
            int_map[key] = value;
        } else if constexpr (std::is_same_v<T, std::string>) {
            str_map[key] = value;
        } else if constexpr (std::is_same_v<T, std::tuple<int, int>>) {
            pos_map[key] = value;
        } else if constexpr (std::is_same_v<T, bool>) {
            bool_map[key] = value;
        } else {
            // Handle other types or throw an error if needed
            static_assert(false, "Unsupported type for MoreData::insert");
        }
    }

    // Overload for std::pair<int, int> to avoid ambiguity with std::tuple
    void insert(const std::string& key, const std::pair<int, int>& value) { pos_map[key] = value; }
};

struct MetaData {
    int st_line_no = -1;
    int st_col_no = -1;
    int end_line_no = -1;
    int end_col_no = -1;
    MoreData more_data;
};

class Node {
  public:
    MetaData meta_data;
    MoreData extra_info;

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

    Expression* castToExpression();
    Statement* castToStatement();
    Type* castToType();
    Program* castToProgram();
    FunctionParameter* castToFunctionParameter();
    // Statement Casts
    ExpressionStatement* castToExpressionStatement();
    BlockStatement* castToBlockStatement();
    ReturnStatement* castToReturnStatement();
    RaiseStatement* castToRaiseStatement();
    FunctionStatement* castToFunctionStatement();
    IfElseStatement* castToIfElseStatement();
    WhileStatement* castToWhileStatement();
    ForStatement* castToForStatement();
    ForEachStatement* castToForEachStatement();
    BreakStatement* castToBreakStatement();
    ContinueStatement* castToContinueStatement();
    ImportStatement* castToImportStatement();
    VariableDeclarationStatement* castToVariableDeclarationStatement();
    VariableAssignmentStatement* castToVariableAssignmentStatement();
    TryCatchStatement* castToTryCatchStatement();
    StructStatement* castToStructStatement();
    EnumStatement* castToEnumStatement();
    SwitchCaseStatement* castToSwitchCaseStatement();
    MacroStatement* castToMacroStatement();
    // Expression Casts
    IdentifierLiteral* castToIdentifierLiteral();
    IntegerLiteral* castToIntegerLiteral();
    FloatLiteral* castToFloatLiteral();
    StringLiteral* castToStringLiteral();
    BooleanLiteral* castToBooleanLiteral();
    ArrayLiteral* castToArrayLiteral();
    InfixExpression* castToInfixExpression();
    IndexExpression* castToIndexExpression();
    CallExpression* castToCallExpression();

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
    ASTUniquePtr<Expression> name;
    std::vector<ASTUniquePtr<Type>> generics;
    bool refrence;
    inline Type(Expression* name, const std::vector<Type*>& generics, bool refrence) : name(name), refrence(refrence) {
        for (auto gen : generics) { this->generics.push_back(gen); }
    }
    NodeType type() override { return NodeType::Type; };
    std::string toStr() override;

    // Destructor Declaration
    ~Type() override;
};

class Program : public Node {
  public:
    std::vector<ASTUniquePtr<Statement>> statements;
    inline NodeType type() override { return NodeType::Program; };
    std::string toStr() override;

    // Destructor Declaration
    ~Program() override;
};

class ExpressionStatement : public Statement {
  public:
    ASTUniquePtr<Expression> expr;
    inline ExpressionStatement(Expression* expr = nullptr) : expr(expr) {}
    inline NodeType type() override { return NodeType::ExpressionStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~ExpressionStatement() override;
};

class BlockStatement : public Statement {
  public:
    std::vector<ASTUniquePtr<Statement>> statements;
    inline NodeType type() override { return NodeType::BlockStatement; };
    inline BlockStatement(const std::vector<Statement*>& statements = {}) {
        for (auto stmt : statements) { this->statements.push_back(stmt); }
    }
    std::string toStr() override;

    // Destructor Declaration
    ~BlockStatement() override;
};

class ReturnStatement : public Statement {
  public:
    ASTUniquePtr<Expression> value;
    inline ReturnStatement(Expression* exp = nullptr) : value(exp) {}
    inline NodeType type() override { return NodeType::ReturnStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~ReturnStatement() override;
};

class RaiseStatement : public Statement {
  public:
    ASTUniquePtr<Expression> value;
    inline RaiseStatement(Expression* exp = nullptr) : value(exp) {}
    inline NodeType type() override { return NodeType::RaiseStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~RaiseStatement() override;
};

class FunctionParameter : public Node {
  public:
    ASTUniquePtr<Expression> name;
    ASTUniquePtr<Type> value_type;
    bool constant;
    inline FunctionParameter(Expression* name, Type* type, bool constant) : name(name), value_type(type), constant(constant) {}
    inline NodeType type() override { return NodeType::FunctionParameter; };
    std::string toStr() override;

    // Destructor Declaration
    ~FunctionParameter() override;
};

class FunctionStatement : public Statement {
  public:
    ASTUniquePtr<Expression> name;
    std::vector<ASTUniquePtr<FunctionParameter>> parameters;
    std::vector<ASTUniquePtr<FunctionParameter>> closure_parameters;
    ASTUniquePtr<Type> return_type;
    bool return_const;
    ASTUniquePtr<BlockStatement> body;
    std::vector<ASTUniquePtr<Type>> generic;
    inline FunctionStatement(Expression* name,
                             std::vector<FunctionParameter*> parameters,
                             std::vector<FunctionParameter*> closure_parameters,
                             Type* return_type,
                             bool return_const,
                             BlockStatement* body,
                             const std::vector<Type*>& generic)
        : name(name), return_type(return_type), return_const(return_const), body(body) {
        for (auto param : parameters) { this->parameters.push_back(param); }
        for (auto param : closure_parameters) { this->closure_parameters.push_back(param); }
        for (auto gen : generic) { this->generic.push_back(gen); }
        this->extra_info.insert("autocast", false);
    }
    inline NodeType type() override { return NodeType::FunctionStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~FunctionStatement() override;
};

class CallExpression : public Expression {
  public:
    ASTUniquePtr<Expression> name;
    std::vector<ASTUniquePtr<Expression>> arguments;
    std::vector<ASTUniquePtr<Expression>> generics;
    bool _new;
    inline CallExpression(Expression* name, const std::vector<Expression*>& arguments = {}) : name(name), _new(false) {
        for (auto arg : arguments) { this->arguments.push_back(arg); }
    }
    inline NodeType type() override { return NodeType::CallExpression; };
    std::string toStr() override;

    // Destructor Declaration
    ~CallExpression() override;
};

class IfElseStatement : public Statement {
  public:
    ASTUniquePtr<Expression> condition;
    ASTUniquePtr<Statement> consequence;
    ASTUniquePtr<Statement> alternative;
    inline IfElseStatement(Expression* condition, Statement* consequence, Statement* alternative = nullptr) : condition(condition), consequence(consequence), alternative(alternative) {}
    inline NodeType type() override { return NodeType::IfElseStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~IfElseStatement() override;
};

class WhileStatement : public Statement {
  public:
    ASTUniquePtr<Expression> condition;
    ASTUniquePtr<Statement> body;
    ASTUniquePtr<Statement> ifbreak;
    ASTUniquePtr<Statement> notbreak;
    inline WhileStatement(Expression* condition, Statement* body, Statement* ifbreak = nullptr, Statement* notbreak = nullptr)
        : condition(condition), body(body), ifbreak(ifbreak), notbreak(notbreak) {}
    inline NodeType type() override { return NodeType::WhileStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~WhileStatement() override;
};

class ForStatement : public Statement {
  public:
    ASTUniquePtr<Statement> init;
    ASTUniquePtr<Expression> condition;
    ASTUniquePtr<Statement> update;
    ASTUniquePtr<Statement> body;
    ASTUniquePtr<Statement> ifbreak;
    ASTUniquePtr<Statement> notbreak;
    inline ForStatement(Statement* init, Expression* condition, Statement* update, Statement* body, Statement* ifbreak = nullptr, Statement* notbreak = nullptr)
        : init(init), condition(condition), update(update), body(body), ifbreak(ifbreak), notbreak(notbreak) {}
    inline NodeType type() override { return NodeType::ForStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~ForStatement() override;
};

class ForEachStatement : public Statement {
  public:
    ASTUniquePtr<Expression> from;
    ASTUniquePtr<IdentifierLiteral> get;
    ASTUniquePtr<Statement> body;
    ASTUniquePtr<Statement> ifbreak;
    ASTUniquePtr<Statement> notbreak;
    inline ForEachStatement(IdentifierLiteral* get, Expression* from, Statement* body, Statement* ifbreak = nullptr, Statement* notbreak = nullptr)
        : get(get), from(from), body(body), ifbreak(ifbreak), notbreak(notbreak) {}
    inline NodeType type() override { return NodeType::ForEachStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~ForEachStatement() override;
};

class BreakStatement : public Statement {
  public:
    int loopIdx;
    inline NodeType type() override { return NodeType::BreakStatement; };
    BreakStatement(int loopNum = 0) : loopIdx(loopNum){};
    std::string toStr() override;
};

class ContinueStatement : public Statement {
  public:
    unsigned short loopIdx = 0;
    inline NodeType type() override { return NodeType::ContinueStatement; };
    ContinueStatement(int loopNum) : loopIdx(loopNum){};
    std::string toStr() override;
};

class ImportStatement : public Statement {
  public:
    std::string relativePath;
    std::string as;
    inline NodeType type() override { return NodeType::ImportStatement; }
    ImportStatement(const std::string& relativePath, const std::string& as) : relativePath(relativePath), as(as) {}
    std::string toStr() override;
};

class VariableDeclarationStatement : public Statement {
  public:
    ASTUniquePtr<Expression> name;
    ASTUniquePtr<Type> value_type;
    ASTUniquePtr<Expression> value;
    bool is_volatile = false;
    bool is_const = false;
    inline VariableDeclarationStatement(Expression* name, Type* type, Expression* value = nullptr, bool is_volatile = false, bool is_const = false)
        : name(name), value_type(type), value(value), is_volatile(is_volatile), is_const(is_const) {}
    inline NodeType type() override { return NodeType::VariableDeclarationStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~VariableDeclarationStatement() override;
};

class VariableAssignmentStatement : public Statement {
  public:
    ASTUniquePtr<Expression> name;
    ASTUniquePtr<Expression> value;
    inline VariableAssignmentStatement(Expression* name, Expression* value) : name(name), value(value) {}
    inline NodeType type() override { return NodeType::VariableAssignmentStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~VariableAssignmentStatement() override;
};

class TryCatchStatement : public Statement {
  public:
    ASTUniquePtr<Statement> try_block;
    std::vector<std::tuple<ASTUniquePtr<Type>, ASTUniquePtr<IdentifierLiteral>, ASTUniquePtr<Statement>>> catch_blocks;
    inline TryCatchStatement(Statement* try_block, std::vector<std::tuple<Type*, IdentifierLiteral*, Statement*>> catch_blocks) : try_block(try_block) {
        for (auto& [t, id, s] : catch_blocks) {
            this->catch_blocks.push_back({t, id, s});
        }
    }
    inline NodeType type() override { return NodeType::TryCatchStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~TryCatchStatement() override;
};

class SwitchCaseStatement : public Statement {
  public:
    ASTUniquePtr<Expression> condition;
    std::vector<std::tuple<ASTUniquePtr<Expression>, ASTUniquePtr<Statement>>> cases;
    ASTUniquePtr<Statement> other;
    inline SwitchCaseStatement(Expression* condition, std::vector<std::tuple<Expression*, Statement*>> cases, Statement* other = nullptr) : condition(condition), other(other) {
        for (auto& [expr, stmt] : cases) {
            this->cases.push_back({expr, stmt});
        }
    }
    inline NodeType type() override { return NodeType::SwitchCaseStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~SwitchCaseStatement() override;
};

class InfixExpression : public Expression {
  public:
    ASTUniquePtr<Expression> left;
    ASTUniquePtr<Expression> right;
    token::TokenType op;
    inline InfixExpression(Expression* left, token::TokenType op, const std::string& literal, Expression* right = nullptr) : left(left), right(right), op(op) {
        this->meta_data.more_data.insert("operator_literal", literal);
    }
    inline NodeType type() override { return NodeType::InfixedExpression; };
    std::string toStr() override;

    // Destructor Declaration
    ~InfixExpression() override;
};

class IndexExpression : public Expression {
  public:
    ASTUniquePtr<Expression> left;
    ASTUniquePtr<Expression> index;
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
    inline StringLiteral(const std::string& value) : value(value) { this->meta_data.more_data.insert("length", int(value.length())); }
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
        this->meta_data.st_line_no = value.end_line_no;
        this->meta_data.end_line_no = value.end_line_no;
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
    ASTUniquePtr<Expression> name = nullptr;
    std::vector<ASTUniquePtr<Statement>> fields = {};
    std::vector<ASTUniquePtr<Type>> generics = {};
    inline StructStatement(Expression* name, const std::vector<Statement*>& fields) : name(name) {
        for (auto f : fields) { this->fields.push_back(f); }
    }
    inline NodeType type() override { return NodeType::StructStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~StructStatement() override;
};

class EnumStatement : public Statement {
  public:
    ASTUniquePtr<Expression> name = nullptr;
    std::vector<std::string> fields = {};
    inline EnumStatement(Expression* name, const std::vector<std::string>& fields) : name(name), fields(fields) {}
    ~EnumStatement() override;
    inline NodeType type() override { return NodeType::EnumStatement; };
    std::string toStr() override;
};

class MacroStatement : public Statement {
  public:
    std::string name;
    ASTUniquePtr<BlockStatement> body;
    inline MacroStatement(std::string name, BlockStatement* body) : name(name), body(body) {}
    inline NodeType type() override { return NodeType::MacroStatement; };
    std::string toStr() override;

    // Destructor Declaration
    ~MacroStatement() override;
};

class ArrayLiteral : public Expression {
  public:
    std::vector<ASTUniquePtr<Expression>> elements;
    bool _new;
    inline ArrayLiteral(const std::vector<Expression*>& elements, bool _new = false) : _new(_new) {
        for (auto el : elements) { this->elements.push_back(el); }
    }
    inline NodeType type() override { return NodeType::ArrayLiteral; };
    std::string toStr() override;

    // Destructor Declaration
    ~ArrayLiteral() override;
};

} // namespace AST

inline AST::Expression* AST::Node::castToExpression() { return static_cast<Expression*>(this); }
inline AST::Statement* AST::Node::castToStatement() { return static_cast<Statement*>(this); }
inline AST::Type* AST::Node::castToType() { return static_cast<Type*>(this); }
inline AST::Program* AST::Node::castToProgram() { return static_cast<Program*>(this); }
inline AST::FunctionParameter* AST::Node::castToFunctionParameter() { return static_cast<FunctionParameter*>(this); }
inline AST::ExpressionStatement* AST::Node::castToExpressionStatement() { return static_cast<ExpressionStatement*>(this); }
inline AST::BlockStatement* AST::Node::castToBlockStatement() { return static_cast<BlockStatement*>(this); }
inline AST::ReturnStatement* AST::Node::castToReturnStatement() { return static_cast<ReturnStatement*>(this); }
inline AST::RaiseStatement* AST::Node::castToRaiseStatement() { return static_cast<RaiseStatement*>(this); }
inline AST::FunctionStatement* AST::Node::castToFunctionStatement() { return static_cast<FunctionStatement*>(this); }
inline AST::IfElseStatement* AST::Node::castToIfElseStatement() { return static_cast<IfElseStatement*>(this); }
inline AST::WhileStatement* AST::Node::castToWhileStatement() { return static_cast<WhileStatement*>(this); }
inline AST::ForStatement* AST::Node::castToForStatement() { return static_cast<ForStatement*>(this); }
inline AST::ForEachStatement* AST::Node::castToForEachStatement() { return static_cast<ForEachStatement*>(this); }
inline AST::BreakStatement* AST::Node::castToBreakStatement() { return static_cast<BreakStatement*>(this); }
inline AST::ContinueStatement* AST::Node::castToContinueStatement() { return static_cast<ContinueStatement*>(this); }
inline AST::ImportStatement* AST::Node::castToImportStatement() { return static_cast<ImportStatement*>(this); }
inline AST::VariableDeclarationStatement* AST::Node::castToVariableDeclarationStatement() { return static_cast<VariableDeclarationStatement*>(this); }
inline AST::VariableAssignmentStatement* AST::Node::castToVariableAssignmentStatement() { return static_cast<VariableAssignmentStatement*>(this); }
inline AST::TryCatchStatement* AST::Node::castToTryCatchStatement() { return static_cast<TryCatchStatement*>(this); }
inline AST::StructStatement* AST::Node::castToStructStatement() { return static_cast<StructStatement*>(this); }
inline AST::EnumStatement* AST::Node::castToEnumStatement() { return static_cast<EnumStatement*>(this); }
inline AST::SwitchCaseStatement* AST::Node::castToSwitchCaseStatement() { return static_cast<SwitchCaseStatement*>(this); }
inline AST::MacroStatement* AST::Node::castToMacroStatement() { return static_cast<MacroStatement*>(this); }
inline AST::IdentifierLiteral* AST::Node::castToIdentifierLiteral() { return static_cast<IdentifierLiteral*>(this); }
inline AST::IntegerLiteral* AST::Node::castToIntegerLiteral() { return static_cast<IntegerLiteral*>(this); }
inline AST::FloatLiteral* AST::Node::castToFloatLiteral() { return static_cast<FloatLiteral*>(this); }
inline AST::StringLiteral* AST::Node::castToStringLiteral() { return static_cast<StringLiteral*>(this); }
inline AST::BooleanLiteral* AST::Node::castToBooleanLiteral() { return static_cast<BooleanLiteral*>(this); }
inline AST::ArrayLiteral* AST::Node::castToArrayLiteral() { return static_cast<ArrayLiteral*>(this); }
inline AST::InfixExpression* AST::Node::castToInfixExpression() { return static_cast<InfixExpression*>(this); }
inline AST::IndexExpression* AST::Node::castToIndexExpression() { return static_cast<IndexExpression*>(this); }
inline AST::CallExpression* AST::Node::castToCallExpression() { return static_cast<CallExpression*>(this); }

#endif // AST_HPP