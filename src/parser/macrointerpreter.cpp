#include "macrointerpreter.hpp"
#include "AST/ast.hpp"
#include <exception>
#include <string>
#include <vector>

class Break : public std::exception {
    public:
      Break() {}
      const char* what() const noexcept override { return "Break Should be in the for loop or while loop"; }
};

class Continue : public std::exception {
    public:
      Continue() {}
      const char* what() const noexcept override { return "Continue Should be in the for loop or while loop"; }
};

class error : public std::exception {
    public:
      error(std::string msg) : msg(msg) {}
      std::string msg;
      const char* what() const noexcept override { return msg.c_str(); }
};

void MacroInterpreter::visitStatement(AST::Statement* node) {
    if (node->type() == AST::NodeType::BlockStatement) {
        visitBlockStatement(node->castToBlockStatement());
    } else if (node->type() == AST::NodeType::ExpressionStatement) {
        visitExpressionStatement(node->castToExpressionStatement());
    } else if (node->type() == AST::NodeType::VariableAssignmentStatement) {
        visitVariableAssignmentStatement(node->castToVariableAssignmentStatement());
    } else if (node->type() == AST::NodeType::ReturnStatement) {
        visitReturnStatement(node->castToReturnStatement());
    } else if (node->type() == AST::NodeType::IfElseStatement) {
        visitIfElseStatement(node->castToIfElseStatement());
    } else if (node->type() == AST::NodeType::WhileStatement) {
        visitWhileStatement(node->castToWhileStatement());
    } else if (node->type() == AST::NodeType::ForStatement) {
        visitForStatement(node->castToForStatement());
    // } else if (node->type() == AST::NodeType::SwitchCaseStatement) {
    //     visitSwitchCaseStatement(node->castToSwitchCaseStatement());
    } else {
        throw std::runtime_error("Unknown statement type");
    }
};

void MacroInterpreter::visitBlockStatement(AST::BlockStatement* node) {
    for (auto stmt : node->statements) {
        visitStatement(stmt);
    }
};

void MacroInterpreter::visitExpressionStatement(AST::ExpressionStatement* node) {
    visitExpression(node->expr);
};

void MacroInterpreter::visitVariableAssignmentStatement(AST::VariableAssignmentStatement* node) {
    if (node->name->type() != AST::NodeType::IdentifierLiteral) {
        throw std::runtime_error("Variable name is not an identifier");
    }
    MIObjects value = visitExpression(node->value);

    variabels[node->name->castToIdentifierLiteral()->value] = value;
};

void MacroInterpreter::visitReturnStatement(AST::ReturnStatement* node) {
    auto return_value = visitExpression(node->value);
    if (return_value.Type != MIObjectType::TokenVector) {
        throw std::runtime_error("Return value is not a token vector");
    }
    // Iterating through the return_value vector in reverse & pussing tokens to the lexer->tokenBuffer
    const auto& tokens = std::get<std::vector<token::Token>>(return_value.Value);
    for (auto it = tokens.rbegin(); it != tokens.rend(); ++it) {
        lexer->tokenBuffer.push_back(*it);
    }
};

void MacroInterpreter::visitIfElseStatement(AST::IfElseStatement* node) {
    MIObjects condition = visitExpression(node->condition);
    if (condition.Type != MIObjectType::Bool) {
        throw std::runtime_error("Condition is not a boolean");
    }
    if (std::get<bool>(condition.Value)) {
        visitBlockStatement(node->consequence->castToBlockStatement());
    } else if (node->alternative != nullptr) {
        visitBlockStatement(node->alternative->castToBlockStatement());
    }
};

void MacroInterpreter::visitWhileStatement(AST::WhileStatement* node) {
    MIObjects condition = visitExpression(node->condition);
    if (condition.Type != MIObjectType::Bool) {
        throw std::runtime_error("Condition is not a boolean");
    }
    while (std::get<bool>(condition.Value)) {
        try {
            visitBlockStatement(node->body->castToBlockStatement());
            condition = visitExpression(node->condition);
        } catch (Break) {
            break;
        } catch (Continue) {
            continue;
        }
    }
};

void MacroInterpreter::visitForStatement(AST::ForStatement* node) {
    auto iterate_from = visitExpression(node->from);
    if (iterate_from.Type != MIObjectType::TokenVector || iterate_from.Type != MIObjectType::TokenTypeVector) {
        throw std::runtime_error("From value is not a token vector or token type vector");
    }
    if (iterate_from.Type != MIObjectType::TokenVector) {
        auto vector = std::get<std::vector<token::Token>>(iterate_from.Value);
        auto name = node->get->castToIdentifierLiteral()->value;
        for (auto token : vector) {
            variabels[name] = MIObjects(MIObjectType::Token, token);
            try {
                visitStatement(node->body);
            } catch (Break) {
                break;
            } catch (Continue) {
                continue;
            }
        }
    }
};

MIObjects MacroInterpreter::visitExpression(AST::Expression* node) {
    switch (node->type()) {
        case AST::NodeType::CallExpression: {
            return visitCallExpression(node->castToCallExpression());
        }
        case AST::NodeType::InfixedExpression: {
            return visitInfixedExpression(node->castToInfixExpression());
        }
        case AST::NodeType::IndexExpression: {
            return visitIndexExpression(node->castToIndexExpression());
        }
        case AST::NodeType::ArrayLiteral: {
            return visitArrayLiteral(node->castToArrayLiteral());
        }
        case AST::NodeType::IntegerLiteral: {
            return visitIntegerLiteral(node->castToIntegerLiteral());
        }
        case AST::NodeType::FloatLiteral: {
            return visitFloatLiteral(node->castToFloatLiteral());
        }
        case AST::NodeType::IdentifierLiteral: {
            if (!variabels.contains(node->castToIdentifierLiteral()->value)) {
                throw error("Fuck You " + node->castToIdentifierLiteral()->value + __FILE__ + ":" + std::to_string(__LINE__));
            }
            return variabels[node->castToIdentifierLiteral()->value];
        }
        default: {
            throw error("Cant Interpret this type of Expresion" + AST::nodeTypeToString(node->type()));
        }
    }
};


MIObjects MacroInterpreter::visitCallExpression(AST::CallExpression* node) {
    auto name = node->name->castToIdentifierLiteral()->value;
    if (name == "expectPeek") {
        if (node->arguments.size() == 1) {
            auto arg0 = visitExpression(node->arguments[0]);
            if (arg0.Type != MIObjectType::TokenTypeVector) {
                if (arg0.Type == MIObjectType::TokenType) {
                    parser->_expectPeek(std::get<token::TokenType>(arg0.Value));
                    return MIObjects(MIObjectType::Void, {});
                }
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            parser->_expectPeek(std::get<std::vector<token::TokenType>>(arg0.Value));
            return MIObjects(MIObjectType::Void, {});
        } else if (node->arguments.size() == 2) {
            auto arg0 = visitExpression(node->arguments[0]);
            auto arg1 = visitExpression(node->arguments[1]);
            if (arg1.Type != MIObjectType::Str) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            auto arg1_val = std::get<std::string>(arg1.Value);
            if (arg0.Type != MIObjectType::TokenTypeVector) {
                if (arg0.Type == MIObjectType::TokenType) {
                    parser->_expectPeek(std::get<token::TokenType>(arg0.Value), arg1_val);
                    return MIObjects(MIObjectType::Void, {});
                }
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            parser->_expectPeek(std::get<std::vector<token::TokenType>>(arg0.Value), arg1_val);
            return MIObjects(MIObjectType::Void, {});
        }
        throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
    } else if (name == "nextToken") {
        if (node->arguments.size() != 0) {
            throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
        }
        parser->_nextToken();
        return MIObjects(MIObjectType::Void, {});
    } else if (name == "Token") {
        if (node->arguments.size() == 4) {
            auto arg0 = visitExpression(node->arguments[0]);
            if (arg0.Type != MIObjectType::TokenType) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            auto arg0_val = std::get<token::TokenType>(arg0.Value);
            auto arg1 = visitExpression(node->arguments[1]);
            if (arg1.Type != MIObjectType::Int) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            auto arg1_val = std::get<int>(arg1.Value);
            auto arg2 = visitExpression(node->arguments[2]);
            if (arg2.Type != MIObjectType::Int) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            auto arg2_val = std::get<int>(arg1.Value);
            auto arg3 = visitExpression(node->arguments[3]);
            if (arg3.Type != MIObjectType::Int) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            auto arg3_val = std::get<int>(arg1.Value);
            return {MIObjectType::Token, token::Token(arg0_val, arg1_val, arg2_val, arg3_val)};
        } else if (node->arguments.size() == 6) {
            auto arg0 = visitExpression(node->arguments[0]);
            if (arg0.Type != MIObjectType::TokenType) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            auto arg0_val = std::get<token::TokenType>(arg0.Value);
            auto arg1 = visitExpression(node->arguments[1]);
            if (arg1.Type != MIObjectType::Str) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            auto arg1_val = std::get<std::string>(arg1.Value);
            auto arg2 = visitExpression(node->arguments[2]);
            if (arg2.Type != MIObjectType::Int) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            auto arg2_val = std::get<int>(arg1.Value);
            auto arg3 = visitExpression(node->arguments[3]);
            if (arg3.Type != MIObjectType::Int) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            auto arg3_val = std::get<int>(arg1.Value);
            auto arg4 = visitExpression(node->arguments[4]);
            if (arg4.Type != MIObjectType::Int) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            auto arg4_val = std::get<int>(arg1.Value);
            auto arg5 = visitExpression(node->arguments[5]);
            if (arg5.Type != MIObjectType::Int) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            auto arg5_val = std::get<int>(arg1.Value);
            return {MIObjectType::Token, token::Token(arg0_val, arg1_val, arg2_val, arg3_val, arg4_val, arg5_val)};
        }
        throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
    } else if (name == "peekTokenIs") {
        if (node->arguments.size() == 1) {
            auto arg0 = visitExpression(node->arguments[0]);
            if (arg0.Type != MIObjectType::TokenType) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            return {MIObjectType::Bool, parser->_peekTokenIs(std::get<token::TokenType>(arg0.Value))};
        }
        throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
    } else if (name == "currentTokenIs") {
        if (node->arguments.size() == 1) {
            auto arg0 = visitExpression(node->arguments[0]);
            if (arg0.Type != MIObjectType::TokenType) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            return {MIObjectType::Bool, parser->_currentTokenIs(std::get<token::TokenType>(arg0.Value))};
        }
        throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
    } else if (name == "currentToken") {
        if (node->arguments.size() != 0) {
            throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
        }
        return {MIObjectType::Token, parser->current_token};
    } else if (name == "peekToken") {
        if (node->arguments.size() != 0) {
            throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
        }
        return {MIObjectType::Token, parser->peek_token};
    }
    throw error("WDF is " + name + " " + __FILE__ + ":" + std::to_string(__LINE__));
};

MIObjects MacroInterpreter::visitInfixedExpression(AST::InfixExpression* node) {
    if (node->op == token::TokenType::Dot) {
        if (node->left->type() == AST::NodeType::IdentifierLiteral) {
            auto name = node->left->castToIdentifierLiteral()->value;
            if (name == "TokenType") {
                if (node->right->type() != AST::NodeType::IdentifierLiteral) {
                    throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
                }
                auto name = node->right->castToIdentifierLiteral()->value;
                if (name == "EndOfFile") {
                    return {MIObjectType::TokenType, token::TokenType::EndOfFile};
                } else if (name == "Illegal") {
                    return {MIObjectType::TokenType, token::TokenType::Illegal};
                } else if (name == "Coment") {
                    return {MIObjectType::TokenType, token::TokenType::Coment};
                } else if (name == "GreaterThan") {
                    return {MIObjectType::TokenType, token::TokenType::GreaterThan};
                } else if (name == "LessThan") {
                    return {MIObjectType::TokenType, token::TokenType::LessThan};
                } else if (name == "GreaterThanOrEqual") {
                    return {MIObjectType::TokenType, token::TokenType::GreaterThanOrEqual};
                } else if (name == "LessThanOrEqual") {
                    return {MIObjectType::TokenType, token::TokenType::LessThanOrEqual};
                } else if (name == "EqualEqual") {
                    return {MIObjectType::TokenType, token::TokenType::EqualEqual};
                } else if (name == "NotEquals") {
                    return {MIObjectType::TokenType, token::TokenType::NotEquals};
                } else if (name == "Identifier") {
                    return {MIObjectType::TokenType, token::TokenType::Identifier};
                } else if (name == "Integer") {
                    return {MIObjectType::TokenType, token::TokenType::Integer};
                } else if (name == "Float") {
                    return {MIObjectType::TokenType, token::TokenType::Float};
                } else if (name == "String") {
                    return {MIObjectType::TokenType, token::TokenType::String};
                } else if (name == "RawString") {
                    return {MIObjectType::TokenType, token::TokenType::RawString};
                } else if (name == "PlusEqual") {
                    return {MIObjectType::TokenType, token::TokenType::PlusEqual};
                } else if (name == "DashEqual") {
                    return {MIObjectType::TokenType, token::TokenType::DashEqual};
                } else if (name == "AsteriskEqual") {
                    return {MIObjectType::TokenType, token::TokenType::AsteriskEqual};
                } else if (name == "PercentEqual") {
                    return {MIObjectType::TokenType, token::TokenType::PercentEqual};
                } else if (name == "CaretEqual") {
                    return {MIObjectType::TokenType, token::TokenType::CaretEqual};
                } else if (name == "ForwardSlashEqual") {
                    return {MIObjectType::TokenType, token::TokenType::ForwardSlashEqual};
                } else if (name == "BackwardSlashEqual") {
                    return {MIObjectType::TokenType, token::TokenType::BackwardSlashEqual};
                } else if (name == "Equals") {
                    return {MIObjectType::TokenType, token::TokenType::Equals};
                } else if (name == "Is") {
                    return {MIObjectType::TokenType, token::TokenType::Is};
                } else if (name == "Increment") {
                    return {MIObjectType::TokenType, token::TokenType::Increment};
                } else if (name == "Decrement") {
                    return {MIObjectType::TokenType, token::TokenType::Decrement};
                } else if (name == "BitwiseAnd") {
                    return {MIObjectType::TokenType, token::TokenType::BitwiseAnd};
                } else if (name == "BitwiseOr") {
                    return {MIObjectType::TokenType, token::TokenType::BitwiseOr};
                } else if (name == "BitwiseXor") {
                    return {MIObjectType::TokenType, token::TokenType::BitwiseXor};
                } else if (name == "BitwiseNot") {
                    return {MIObjectType::TokenType, token::TokenType::BitwiseNot};
                } else if (name == "LeftShift") {
                    return {MIObjectType::TokenType, token::TokenType::LeftShift};
                } else if (name == "RightShift") {
                    return {MIObjectType::TokenType, token::TokenType::RightShift};
                } else if (name == "Dot") {
                    return {MIObjectType::TokenType, token::TokenType::Dot};
                } else if (name == "Ellipsis") {
                    return {MIObjectType::TokenType, token::TokenType::Ellipsis};
                } else if (name == "Plus") {
                    return {MIObjectType::TokenType, token::TokenType::Plus};
                } else if (name == "Dash") {
                    return {MIObjectType::TokenType, token::TokenType::Dash};
                } else if (name == "Asterisk") {
                    return {MIObjectType::TokenType, token::TokenType::Asterisk};
                } else if (name == "Percent") {
                    return {MIObjectType::TokenType, token::TokenType::Percent};
                } else if (name == "AsteriskAsterisk") {
                    return {MIObjectType::TokenType, token::TokenType::AsteriskAsterisk};
                } else if (name == "ForwardSlash") {
                    return {MIObjectType::TokenType, token::TokenType::ForwardSlash};
                } else if (name == "BackwardSlash") {
                    return {MIObjectType::TokenType, token::TokenType::BackwardSlash};
                } else if (name == "Refrence") {
                    return {MIObjectType::TokenType, token::TokenType::Refrence};
                } else if (name == "LeftParen") {
                    return {MIObjectType::TokenType, token::TokenType::LeftParen};
                } else if (name == "RightParen") {
                    return {MIObjectType::TokenType, token::TokenType::RightParen};
                } else if (name == "LeftBrace") {
                    return {MIObjectType::TokenType, token::TokenType::LeftBrace};
                } else if (name == "RightBrace") {
                    return {MIObjectType::TokenType, token::TokenType::RightBrace};
                } else if (name == "LeftBracket") {
                    return {MIObjectType::TokenType, token::TokenType::LeftBracket};
                } else if (name == "RightBracket") {
                    return {MIObjectType::TokenType, token::TokenType::RightBracket};
                } else if (name == "Colon") {
                    return {MIObjectType::TokenType, token::TokenType::Colon};
                } else if (name == "Semicolon") {
                    return {MIObjectType::TokenType, token::TokenType::Semicolon};
                } else if (name == "RightArrow") {
                    return {MIObjectType::TokenType, token::TokenType::RightArrow};
                } else if (name == "Comma") {
                    return {MIObjectType::TokenType, token::TokenType::Comma};
                } else if (name == "AtTheRate") {
                    return {MIObjectType::TokenType, token::TokenType::AtTheRate};
                } else if (name == "Pipe") {
                    return {MIObjectType::TokenType, token::TokenType::Pipe};
                } else if (name == "And") {
                    return {MIObjectType::TokenType, token::TokenType::And};
                } else if (name == "Or") {
                    return {MIObjectType::TokenType, token::TokenType::Or};
                } else if (name == "Not") {
                    return {MIObjectType::TokenType, token::TokenType::Not};
                } else if (name == "Def") {
                    return {MIObjectType::TokenType, token::TokenType::Def};
                } else if (name == "Return") {
                    return {MIObjectType::TokenType, token::TokenType::Return};
                } else if (name == "If") {
                    return {MIObjectType::TokenType, token::TokenType::If};
                } else if (name == "Else") {
                    return {MIObjectType::TokenType, token::TokenType::Else};
                } else if (name == "ElIf") {
                    return {MIObjectType::TokenType, token::TokenType::ElIf};
                } else if (name == "While") {
                    return {MIObjectType::TokenType, token::TokenType::While};
                } else if (name == "For") {
                    return {MIObjectType::TokenType, token::TokenType::For};
                } else if (name == "In") {
                    return {MIObjectType::TokenType, token::TokenType::In};
                } else if (name == "Break") {
                    return {MIObjectType::TokenType, token::TokenType::Break};
                } else if (name == "Continue") {
                    return {MIObjectType::TokenType, token::TokenType::Continue};
                } else if (name == "Struct") {
                    return {MIObjectType::TokenType, token::TokenType::Struct};
                } else if (name == "Enum") {
                    return {MIObjectType::TokenType, token::TokenType::Enum};
                } else if (name == "Volatile") {
                    return {MIObjectType::TokenType, token::TokenType::Volatile};
                } else if (name == "Use") {
                    return {MIObjectType::TokenType, token::TokenType::Use};
                } else if (name == "Import") {
                    return {MIObjectType::TokenType, token::TokenType::Import};
                } else if (name == "As") {
                    return {MIObjectType::TokenType, token::TokenType::As};
                } else if (name == "True") {
                    return {MIObjectType::TokenType, token::TokenType::True};
                } else if (name == "False") {
                    return {MIObjectType::TokenType, token::TokenType::False};
                } else if (name == "None") {
                    return {MIObjectType::TokenType, token::TokenType::None};
                } else if (name == "New") {
                    return {MIObjectType::TokenType, token::TokenType::New};
                } else if (name == "Try") {
                    return {MIObjectType::TokenType, token::TokenType::Try};
                } else if (name == "Catch") {
                    return {MIObjectType::TokenType, token::TokenType::Catch};
                } else if (name == "Raise") {
                    return {MIObjectType::TokenType, token::TokenType::Raise};
                } else if (name == "IfBreak") {
                    return {MIObjectType::TokenType, token::TokenType::IfBreak};
                } else if (name == "NotBreak") {
                    return {MIObjectType::TokenType, token::TokenType::NotBreak};
                } else if (name == "Switch") {
                    return {MIObjectType::TokenType, token::TokenType::Switch};
                } else if (name == "Case") {
                    return {MIObjectType::TokenType, token::TokenType::Case};
                } else if (name == "Other") {
                    return {MIObjectType::TokenType, token::TokenType::Other};
                }
                // Throw Exception
            }
            throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
        }
        throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
    } else if (node->op == token::TokenType::Plus) {
        throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
    } else if (node->op == token::TokenType::Dash) {
        throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
    } else if (node->op == token::TokenType::Asterisk) {
        throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
    } else if (node->op == token::TokenType::BackwardSlash) {
        throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
    }
    throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
};

MIObjects MacroInterpreter::visitIndexExpression(AST::IndexExpression* node) {
    throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
};

MIObjects MacroInterpreter::visitArrayLiteral(AST::ArrayLiteral* node) {
    if (node->elements.size() == 0) {
        throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
    }
    auto arg0_type = visitExpression(node->elements[0]).Type;
    if (arg0_type == MIObjectType::TokenType) {
        std::vector<token::TokenType> vec;
        for (auto element : node->elements) {
            auto ele = visitExpression(element);
            if (ele.Type != MIObjectType::TokenType) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            vec.push_back(std::get<token::TokenType>(ele.Value));
        }
        return {MIObjectType::TokenTypeVector, vec};
    } else if (arg0_type == MIObjectType::Token) {
        std::vector<token::Token> vec;
        for (auto element : node->elements) {
            auto ele = visitExpression(element);
            if (ele.Type != MIObjectType::Token) {
                throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
            }
            vec.push_back(std::get<token::Token>(ele.Value));
        }
        return {MIObjectType::TokenVector, vec};
    }
    throw error(std::string("Fuck You ") + __FILE__ + ":" + std::to_string(__LINE__));
};

MIObjects MacroInterpreter::visitIntegerLiteral(AST::IntegerLiteral* node) {
    return {MIObjectType::Int, int(node->value)};
};

MIObjects MacroInterpreter::visitFloatLiteral(AST::FloatLiteral* node) {
    return {MIObjectType::Float, float(node->value)};
};

