#include "ast.hpp"
#include "../../lexer/token.hpp"

#include <memory>


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

std::shared_ptr<nlohmann::json> AST::Type::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["name"] = *this->name->toJSON();
    jsonAst["generics"] = nlohmann::json::array();
    for (auto& gen : this->generics) {
        jsonAst["generics"].push_back(*gen->toJSON());
    }
    return std::make_shared<nlohmann::json>(jsonAst);
};

std::shared_ptr<nlohmann::json> AST::GenericType::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["name"] = *this->name->toJSON();
    jsonAst["generic_union"] = nlohmann::json::array();
    for (auto& gen : this->generic_union) {
        jsonAst["generic_union"].push_back(*gen->toJSON());
    }
    return std::make_shared<nlohmann::json>(jsonAst);
};

std::shared_ptr<nlohmann::json> AST::Program::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["statements"] = nlohmann::json::array();
    for (auto& stmt : this->statements) {
        jsonAst["statements"].push_back(*stmt->toJSON());
    }
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::ExpressionStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["Expression"] = this->expr == nullptr ? nullptr : *expr->toJSON();
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::BlockStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["statements"] = nlohmann::json::array();
    for (auto& stmt : this->statements) {
        jsonAst["statements"].push_back(*stmt->toJSON());
    }
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::ReturnStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["value"] = this->value == nullptr ? nullptr : *(this->value->toJSON());
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::RaiseStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["value"] = this->value == nullptr ? nullptr : *(this->value->toJSON());
    return std::make_shared<nlohmann::json>(jsonAst);
}


std::shared_ptr<nlohmann::json> AST::FunctionStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["name"] = *this->name->toJSON();
    jsonAst["parameters"] = nlohmann::json::array();
    for (auto& param : this->parameters) {
        jsonAst["parameters"].push_back(*param->toJSON());
    }
    jsonAst["return_type"] = this->return_type ? *this->return_type->toJSON() : nullptr;
    jsonAst["body"] = this->body ? *this->body->toJSON() : nullptr;
    jsonAst["generic"] = nlohmann::json::array();
    for (auto& gen : this->generic) {
        jsonAst["generic"].push_back(*gen->toJSON());
    }
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::FunctionParameter::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["param_name"] = *this->name->toJSON();
    jsonAst["param_type"] = *this->value_type->toJSON();
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::CallExpression::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["name"] = *this->name->toJSON();
    jsonAst["arguments"] = nlohmann::json::array();
    for (auto& arg : this->arguments) {
        jsonAst["arguments"].push_back(*arg->toJSON());
    }
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::IfElseStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["condition"] = *this->condition->toJSON();
    jsonAst["consequence"] = *this->consequence->toJSON();
    jsonAst["alternative"] = this->alternative == nullptr ? nullptr : *this->alternative->toJSON();
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::WhileStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["condition"] = *this->condition->toJSON();
    jsonAst["body"] = *this->body->toJSON();
    jsonAst["ifbreak"] = this->ifbreak ? *this->ifbreak->toJSON() : nullptr;
    jsonAst["notbreak"] = this->notbreak ? *this->notbreak->toJSON() : nullptr;
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::ForStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["body"] = *this->body->toJSON();
    jsonAst["get"] = *this->get->toJSON();
    jsonAst["from"] = *this->from->toJSON();
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::BreakStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["loopNum"] = this->loopIdx;
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::ContinueStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["loopNum"] = this->loopIdx;
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::ImportStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["path"] = this->relativePath;
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::VariableDeclarationStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["name"] = *this->name->toJSON();
    jsonAst["value_type"] = *this->value_type->toJSON();
    jsonAst["value"] = this->value == nullptr ? nullptr : *value->toJSON();
    jsonAst["volatile"] = this->is_volatile;
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::VariableAssignmentStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["name"] = *this->name->toJSON();
    jsonAst["value"] = *value->toJSON();
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::TryCatchStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["try"] = *try_block->toJSON();
    jsonAst["catch"] = nlohmann::json::array();
    for (auto& [type, var, block] : this->catch_blocks) {
        jsonAst["catch"].push_back({*type->toJSON(), *var->toJSON(), *block->toJSON()});
    }
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::InfixExpression::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["operator"] = token::tokenTypeString(this->op);
    jsonAst["left_node"] = *(left->toJSON());
    jsonAst["right_node"] = this->right == nullptr ? nullptr : *right->toJSON();
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::IndexExpression::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["left_node"] = *(left->toJSON());
    jsonAst["index"] = *(index->toJSON());
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::IntegerLiteral::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["value"] = this->value;
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::FloatLiteral::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["value"] = this->value;
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::StringLiteral::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["value"] = this->value;
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::IdentifierLiteral::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["value"] = this->value;
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::BooleanLiteral::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["value"] = this->value;
    return std::make_shared<nlohmann::json>(jsonAst);
}

std::shared_ptr<nlohmann::json> AST::StructStatement::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["name"] = *this->name->toJSON();
    jsonAst["fields"] = nlohmann::json::array();
    for (auto& field : this->fields) {
        jsonAst["fields"].push_back(*field->toJSON());
    }
    jsonAst["generics"] = nlohmann::json::array();
    for (auto& field : this->generics) {
        jsonAst["generics"].push_back(*field->toJSON());
    }
    return std::make_shared<nlohmann::json>(jsonAst);
}


std::shared_ptr<nlohmann::json> AST::ArrayLiteral::toJSON() {
    auto jsonAst = nlohmann::json();
    jsonAst["type"] = nodeTypeToString(this->type());
    jsonAst["elements"] = nlohmann::json::array();
    for (auto& element : this->elements) {
        jsonAst["elements"].push_back(*element->toJSON());
    }
    return std::make_shared<nlohmann::json>(jsonAst);
}
