#include "ast.hpp"
#include "../../lexer/token.hpp"

#include <memory>

using namespace AST;

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
        case NodeType::GenericType:
            return "GenericType";
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
        case NodeType::ForStatement:
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

JsonPtr Type::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["name"] = *name->toJSON();
    jsonAst["generics"] = Json::array();
    for (auto& gen : generics) { jsonAst["generics"].push_back(*gen->toJSON()); }
    return std::make_shared<Json>(jsonAst);
};

JsonPtr GenericType::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["name"] = *name->toJSON();
    jsonAst["generic_union"] = Json::array();
    for (auto& gen : generic_union) { jsonAst["generic_union"].push_back(*gen->toJSON()); }
    return std::make_shared<Json>(jsonAst);
};

JsonPtr Program::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["statements"] = Json::array();
    for (auto& stmt : statements) { jsonAst["statements"].push_back(*stmt->toJSON()); }
    return std::make_shared<Json>(jsonAst);
}

JsonPtr ExpressionStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["Expression"] = expr == nullptr ? nullptr : *expr->toJSON();
    return std::make_shared<Json>(jsonAst);
}

JsonPtr BlockStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["statements"] = Json::array();
    for (auto& stmt : statements) { jsonAst["statements"].push_back(*stmt->toJSON()); }
    return std::make_shared<Json>(jsonAst);
}

JsonPtr ReturnStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["value"] = value == nullptr ? nullptr : *(value->toJSON());
    return std::make_shared<Json>(jsonAst);
}

JsonPtr RaiseStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["value"] = value == nullptr ? nullptr : *(value->toJSON());
    return std::make_shared<Json>(jsonAst);
}

JsonPtr FunctionStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["name"] = *name->toJSON();
    jsonAst["parameters"] = Json::array();
    for (auto& param : parameters) { jsonAst["parameters"].push_back(*param->toJSON()); }
    jsonAst["return_type"] = return_type ? *return_type->toJSON() : nullptr;
    jsonAst["body"] = body ? *body->toJSON() : nullptr;
    jsonAst["generic"] = Json::array();
    for (auto& gen : generic) { jsonAst["generic"].push_back(*gen->toJSON()); }
    return std::make_shared<Json>(jsonAst);
}

JsonPtr FunctionParameter::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["param_name"] = *name->toJSON();
    jsonAst["param_type"] = *value_type->toJSON();
    return std::make_shared<Json>(jsonAst);
}

JsonPtr CallExpression::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["name"] = *name->toJSON();
    jsonAst["arguments"] = Json::array();
    for (auto& arg : arguments) { jsonAst["arguments"].push_back(*arg->toJSON()); }
    return std::make_shared<Json>(jsonAst);
}

JsonPtr IfElseStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["condition"] = *condition->toJSON();
    jsonAst["consequence"] = *consequence->toJSON();
    jsonAst["alternative"] = alternative == nullptr ? nullptr : *alternative->toJSON();
    return std::make_shared<Json>(jsonAst);
}

JsonPtr WhileStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["condition"] = *condition->toJSON();
    jsonAst["body"] = *body->toJSON();
    jsonAst["ifbreak"] = ifbreak ? *ifbreak->toJSON() : nullptr;
    jsonAst["notbreak"] = notbreak ? *notbreak->toJSON() : nullptr;
    return std::make_shared<Json>(jsonAst);
}

JsonPtr ForStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["body"] = *body->toJSON();
    jsonAst["get"] = *get->toJSON();
    jsonAst["from"] = *from->toJSON();
    return std::make_shared<Json>(jsonAst);
}

JsonPtr BreakStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["loopNum"] = loopIdx;
    return std::make_shared<Json>(jsonAst);
}

JsonPtr ContinueStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["loopNum"] = loopIdx;
    return std::make_shared<Json>(jsonAst);
}

JsonPtr ImportStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["path"] = relativePath;
    return std::make_shared<Json>(jsonAst);
}

JsonPtr VariableDeclarationStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["name"] = *name->toJSON();
    jsonAst["value_type"] = *value_type->toJSON();
    jsonAst["value"] = value == nullptr ? nullptr : *value->toJSON();
    jsonAst["volatile"] = is_volatile;
    return std::make_shared<Json>(jsonAst);
}

JsonPtr VariableAssignmentStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["name"] = *name->toJSON();
    jsonAst["value"] = *value->toJSON();
    return std::make_shared<Json>(jsonAst);
}

JsonPtr TryCatchStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["try"] = *try_block->toJSON();
    jsonAst["catch"] = Json::array();
    for (auto& [type, var, block] : catch_blocks) { jsonAst["catch"].push_back({*type->toJSON(), *var->toJSON(), *block->toJSON()}); }
    return std::make_shared<Json>(jsonAst);
}

JsonPtr InfixExpression::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["operator"] = token::tokenTypeString(op);
    jsonAst["left_node"] = *(left->toJSON());
    jsonAst["right_node"] = right == nullptr ? nullptr : *right->toJSON();
    return std::make_shared<Json>(jsonAst);
}

JsonPtr IndexExpression::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["left_node"] = *(left->toJSON());
    jsonAst["index"] = *(index->toJSON());
    return std::make_shared<Json>(jsonAst);
}

JsonPtr IntegerLiteral::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["value"] = value;
    return std::make_shared<Json>(jsonAst);
}

JsonPtr FloatLiteral::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["value"] = value;
    return std::make_shared<Json>(jsonAst);
}

JsonPtr StringLiteral::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["value"] = value;
    return std::make_shared<Json>(jsonAst);
}

JsonPtr IdentifierLiteral::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["value"] = value;
    return std::make_shared<Json>(jsonAst);
}

JsonPtr BooleanLiteral::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["value"] = value;
    return std::make_shared<Json>(jsonAst);
}

JsonPtr StructStatement::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["name"] = *name->toJSON();
    jsonAst["fields"] = Json::array();
    for (auto& field : fields) { jsonAst["fields"].push_back(*field->toJSON()); }
    jsonAst["generics"] = Json::array();
    for (auto& field : generics) { jsonAst["generics"].push_back(*field->toJSON()); }
    return std::make_shared<Json>(jsonAst);
}

JsonPtr ArrayLiteral::toJSON() {
    auto jsonAst = Json();
    jsonAst["type"] = nodeTypeToString(type());
    jsonAst["elements"] = Json::array();
    for (auto& element : elements) { jsonAst["elements"].push_back(*element->toJSON()); }
    return std::make_shared<Json>(jsonAst);
}
