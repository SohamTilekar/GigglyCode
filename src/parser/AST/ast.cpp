#include "ast.hpp"
#include "../../lexer/token.hpp"
#include <yaml-cpp/yaml.h>

using namespace AST;

Type::~Type() = default;
Program::~Program() = default;
ExpressionStatement::~ExpressionStatement() = default;
BlockStatement::~BlockStatement() = default;
ReturnStatement::~ReturnStatement() = default;
RaiseStatement::~RaiseStatement() = default;
FunctionParameter::~FunctionParameter() = default;
FunctionStatement::~FunctionStatement() = default;
CallExpression::~CallExpression() = default;
IfElseStatement::~IfElseStatement() = default;
WhileStatement::~WhileStatement() = default;
ForEachStatement::~ForEachStatement() = default;
ForStatement::~ForStatement() = default;
VariableDeclarationStatement::~VariableDeclarationStatement() = default;
VariableAssignmentStatement::~VariableAssignmentStatement() = default;
TryCatchStatement::~TryCatchStatement() = default;
SwitchCaseStatement::~SwitchCaseStatement() = default;
InfixExpression::~InfixExpression() = default;
IndexExpression::~IndexExpression() = default;
IntegerLiteral::~IntegerLiteral() = default;
FloatLiteral::~FloatLiteral() = default;
StringLiteral::~StringLiteral() = default;
IdentifierLiteral::~IdentifierLiteral() = default;
BooleanLiteral::~BooleanLiteral() = default;
StructStatement::~StructStatement() = default;
EnumStatement::~EnumStatement() = default;
MacroStatement::~MacroStatement() = default;
ArrayLiteral::~ArrayLiteral() = default;

std::string AST::nodeTypeToString(NodeType type) {
    switch (type) {
        case NodeType::Program:
            return "Program";
        case NodeType::ExpressionStatement:
            return "ExpressionStatement";
        case NodeType::VariableDeclarationStatement:
            return "VariableDeclarationStatement";
        case NodeType::VariableAssignmentStatement:
            return "VariableAssignmentStatement";
        case NodeType::FunctionStatement:
            return "FunctionStatement";
        case NodeType::FunctionParameter:
            return "FunctionParameter";
        case NodeType::CallExpression:
            return "CallExpression";
        case NodeType::BlockStatement:
            return "BlockStatement";
        case NodeType::ReturnStatement:
            return "ReturnStatement";
        case NodeType::Type:
            return "Type";
        case NodeType::InfixedExpression:
            return "InfixedExpression";
        case NodeType::IntegerLiteral:
            return "IntegerLiteral";
        case NodeType::FloatLiteral:
            return "FloatLiteral";
        case NodeType::StringLiteral:
            return "StringLiteral";
        case NodeType::IdentifierLiteral:
            return "IdentifierLiteral";
        case NodeType::BooleanLiteral:
            return "BooleanLiteral";
        case NodeType::IfElseStatement:
            return "IfElseStatement";
        case NodeType::WhileStatement:
            return "WhileStatement";
        case NodeType::ForEachStatement:
            return "ForStatement";
        case NodeType::BreakStatement:
            return "BreakStatement";
        case NodeType::ContinueStatement:
            return "ContinueStatement";
        case NodeType::ImportStatement:
            return "ImportStatement";
        case NodeType::StructStatement:
            return "StructDecelerationStatement";
        case NodeType::IndexExpression:
            return "IndexExpression";
        case NodeType::ArrayLiteral:
            return "ArrayLiteral";
        case NodeType::TryCatchStatement:
            return "TryCatchStatement";
        case NodeType::RaiseStatement:
            return "RaiseStatement";
        default:
            return "UNKNOWN";
    }
};

std::string Type::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "name" << YAML::Value << name->toStr();
    out << YAML::Key << "generics" << YAML::Value << YAML::BeginSeq;
    for (auto& gen : generics) { out << YAML::Load(gen->toStr()); }
    out << YAML::EndSeq;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string Program::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "statements" << YAML::Value << YAML::BeginSeq;
    for (auto& stmt : statements) { out << YAML::Load(stmt->toStr()); }
    out << YAML::EndSeq;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string ExpressionStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "Expression";
    if (expr) {
        out << YAML::Value << YAML::Load(expr->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string BlockStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "statements" << YAML::Value << YAML::BeginSeq;
    for (auto& stmt : statements) { out << YAML::Load(stmt->toStr()); }
    out << YAML::EndSeq;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string ReturnStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "value";
    if (value) {
        out << YAML::Value << YAML::Load(value->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string RaiseStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "value";
    if (value) {
        out << YAML::Value << YAML::Load(value->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string FunctionStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "name" << YAML::Value << name->toStr();
    out << YAML::Key << "parameters" << YAML::Value << YAML::BeginSeq;
    for (auto& param : parameters) { out << YAML::Load(param->toStr()); }
    out << YAML::EndSeq;
    out << YAML::Key << "return_type";
    if (return_type) {
        out << YAML::Value << YAML::Load(return_type->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "body";
    if (body) {
        out << YAML::Value << YAML::Load(body->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "generic" << YAML::Value << YAML::BeginSeq;
    for (auto& gen : generic) { out << YAML::Load(gen->toStr()); }
    out << YAML::EndSeq;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string FunctionParameter::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "param_name" << YAML::Value << name->toStr();
    out << YAML::Key << "param_type" << YAML::Value;
    out << YAML::Load(value_type->toStr());
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string CallExpression::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "name" << YAML::Value << name->toStr();
    out << YAML::Key << "arguments" << YAML::Value << YAML::BeginSeq;
    for (auto& arg : arguments) { out << YAML::Load(arg->toStr()); }
    out << YAML::EndSeq;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string IfElseStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "condition";
    if (condition) {
        out << YAML::Value << YAML::Load(condition->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "consequence";
    if (consequence) {
        out << YAML::Value << YAML::Load(consequence->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "alternative";
    if (alternative) {
        out << YAML::Value << YAML::Load(alternative->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string WhileStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "condition";
    if (condition) {
        out << YAML::Value << YAML::Load(condition->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "body";
    if (body) {
        out << YAML::Value << YAML::Load(body->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "ifbreak";
    if (ifbreak) {
        out << YAML::Value << YAML::Load(ifbreak->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "notbreak";
    if (notbreak) {
        out << YAML::Value << YAML::Load(notbreak->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string ForStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "init";
    if (init) {
        out << YAML::Value << YAML::Load(init->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "condition";
    if (condition) {
        out << YAML::Value << YAML::Load(condition->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "update";
    if (update) {
        out << YAML::Value << YAML::Load(update->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "body";
    if (body) {
        out << YAML::Value << YAML::Load(body->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "ifbreak";
    if (ifbreak) {
        out << YAML::Value << YAML::Load(ifbreak->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "notbreak";
    if (notbreak) {
        out << YAML::Value << YAML::Load(notbreak->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string ForEachStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "body";
    if (body) {
        out << YAML::Value << YAML::Load(body->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "get";
    if (get) {
        out << YAML::Value << YAML::Load(get->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "from";
    if (from) {
        out << YAML::Value << YAML::Load(from->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string BreakStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "loopNum" << YAML::Value << loopIdx;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string ContinueStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "loopNum" << YAML::Value << loopIdx;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string ImportStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "path" << YAML::Value << relativePath;
    out << YAML::Key << "as" << YAML::Value << as;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string VariableDeclarationStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "name" << YAML::Value << name->toStr();
    out << YAML::Key << "value_type";
    if (value_type) {
        out << YAML::Value << YAML::Load(value_type->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "value";
    if (value) {
        out << YAML::Value << YAML::Load(value->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "volatile" << YAML::Value << (is_volatile ? "true" : "false");
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string VariableAssignmentStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "name" << YAML::Value << name->toStr();
    out << YAML::Key << "value" << YAML::Value << YAML::Load(value->toStr());
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string TryCatchStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "try";
    if (try_block) {
        out << YAML::Value << YAML::Load(try_block->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "catch" << YAML::Value << YAML::BeginSeq;
    for (auto& [type, var, block] : catch_blocks) {
        YAML::Node catchNode;
        catchNode["type"] = YAML::Load(type->toStr());
        catchNode["var"] = YAML::Load(var->toStr());
        catchNode["block"] = YAML::Load(block->toStr());
        out << catchNode;
    }
    out << YAML::EndSeq;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string SwitchCaseStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "condition";
    if (condition) {
        out << YAML::Value << YAML::Load(condition->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "case" << YAML::Value << YAML::BeginSeq;
    for (auto& [_case, block] : cases) {
        YAML::Node caseNode;
        caseNode["case"] = YAML::Load(_case->toStr());
        caseNode["block"] = YAML::Load(block->toStr());
        out << caseNode;
    }
    out << YAML::EndSeq;
    out << YAML::Key << "other";
    if (other) {
        out << YAML::Value << YAML::Load(other->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string InfixExpression::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "operator" << YAML::Value << token::tokenTypeString(op);
    out << YAML::Key << "left_node";
    if (left) {
        out << YAML::Value << YAML::Load(left->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "right_node";
    if (right) {
        out << YAML::Value << YAML::Load(right->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string IndexExpression::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "left_node";
    if (left) {
        out << YAML::Value << YAML::Load(left->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::Key << "index";
    if (index) {
        out << YAML::Value << YAML::Load(index->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string IntegerLiteral::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "value" << YAML::Value << value;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string FloatLiteral::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "value" << YAML::Value << value;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string StringLiteral::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "value" << YAML::Value << value;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string IdentifierLiteral::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "value" << YAML::Value << value;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string BooleanLiteral::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "value" << YAML::Value << (value ? "true" : "false");
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string StructStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "name" << YAML::Value << name->toStr();
    out << YAML::Key << "fields" << YAML::Value << YAML::BeginSeq;
    for (auto& field : fields) { out << YAML::Load(field->toStr()); }
    out << YAML::EndSeq;
    out << YAML::Key << "generics" << YAML::Value << YAML::BeginSeq;
    for (auto& gen : generics) { out << YAML::Load(gen->toStr()); }
    out << YAML::EndSeq;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string EnumStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "name" << YAML::Value << name->toStr();
    out << YAML::Key << "fields" << YAML::Value << YAML::BeginSeq;
    for (auto& field : fields) { out << YAML::Load(field); }
    out << YAML::EndSeq;
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string MacroStatement::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "name" << YAML::Value << name;
    out << YAML::Key << "body";
    if (body) {
        out << YAML::Value << YAML::Load(body->toStr());
    } else {
        out << YAML::Value << "null";
    }
    out << YAML::EndMap;
    return std::string(out.c_str());
}

std::string ArrayLiteral::toStr() {
    YAML::Emitter out;
    out << YAML::BeginMap;
    out << YAML::Key << "type" << YAML::Value << nodeTypeToString(type());
    out << YAML::Key << "elements" << YAML::Value << YAML::BeginSeq;
    for (auto& element : elements) { out << YAML::Load(element->toStr()); }
    out << YAML::EndSeq;
    out << YAML::EndMap;
    return std::string(out.c_str());
}
