#include "parser.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>

#include "AST/ast.hpp"

parser::Parser::Parser(std::shared_ptr<Lexer> lexer) : lexer(lexer) {
    this->_nextToken();
    this->_nextToken();
}

std::shared_ptr<AST::Program> parser::Parser::parseProgram() {
    std::shared_ptr<AST::Program> program = std::make_shared<AST::Program>();
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    while(current_token->type != token::TokenType::EndOfFile) {
        auto statement = this->_parseStatement();
        if(statement != nullptr) {
            program->statements.push_back(statement);
        }
        this->_nextToken();
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    program->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return program;
}

std::shared_ptr<AST::Statement> parser::Parser::_parseStatement() {
    if(this->_currentTokenIs(token::TokenType::Identifier)) {
        int st_line_no = current_token->line_no;
        int st_col_no = current_token->col_no;
        auto identifier = this->_parseInfixIdenifier();
        if(this->_peekTokenIs(token::TokenType::Colon)) {
            return this->_parseVariableDeclaration(identifier, st_line_no, st_col_no);
        } else if(this->_peekTokenIs(token::TokenType::Equals)) {
            return this->_parseVariableAssignment(identifier, st_line_no, st_col_no);
        } else if(this->_peekTokenIs(token::TokenType::LeftParen)) {
            auto smt = std::make_shared<AST::ExpressionStatement>(this->_parseFunctionCall(identifier, st_line_no, st_col_no));
            if(!this->_expectPeek(token::TokenType::Semicolon)) {
                return nullptr;
            }
            return smt;
        } else {
            return this->_parseExpressionStatement(identifier, st_line_no, st_col_no);
        }
    } else if(this->_currentTokenIs(token::TokenType::LeftBrace)) {
        return this->_parseBlockStatement();
    } else if(this->_currentTokenIs(token::TokenType::Return)) {
        return this->_parseReturnStatement();
    } else if(this->_currentTokenIs(token::TokenType::Def)) {
        return this->_parseFunctionStatement();
    } else if(this->_currentTokenIs(token::TokenType::AtTheRate)) {
        return this->_parseDeco();
    } else if(this->_currentTokenIs(token::TokenType::If)) {
        return this->_parseIfElseStatement();
    } else if(this->_currentTokenIs(token::TokenType::While)) {
        return this->_parseWhileStatement();
    } else if(this->_currentTokenIs(token::TokenType::Break)) {
        return this->_parseBreakStatement();
    } else if(this->_currentTokenIs(token::TokenType::Continue)) {
        return this->_parseContinueStatement();
    } else if(this->_currentTokenIs(token::TokenType::Import)) {
        return this->_parseImportStatement();
    } else if(this->_currentTokenIs(token::TokenType::Volatile)) {
        this->_nextToken();
        return this->_parseVariableDeclaration(nullptr, -1, -1, true);
    } else if(this->_currentTokenIs(token::TokenType::Struct)) {
        return this->_parseStructStatement();
    } else {
        return this->_parseExpressionStatement();
    }
}

std::shared_ptr<AST::Statement> parser::Parser::_parseDeco() {
    if(!this->_expectPeek(token::TokenType::Identifier)) {
        return nullptr;
    }
    std::string name = this->current_token->literal;
    if(name == std::string("generic")) {
        if(!this->_expectPeek(token::TokenType::LeftParen)) {
            return nullptr;
        }
        this->_nextToken();
        std::vector<std::shared_ptr<AST::GenericType>> generics;
        while(this->current_token->type != token::TokenType::RightParen) {
            if(this->current_token->type == token::TokenType::Identifier) {
                auto identifier = std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);
                if(!this->_expectPeek(token::TokenType::Colon)) {
                    return nullptr;
                }
                this->_nextToken();
                std::vector<std::shared_ptr<AST::Type>> type;
                while(this->current_token->type != token::TokenType::RightParen && this->current_token->type != token::TokenType::Comma) {
                    type.push_back(this->_parseType());
                    if(this->_peekTokenIs(token::TokenType::Pipe)) {
                        this->_nextToken();
                        this->_nextToken();
                    } else {
                        break;
                    }
                }
                generics.push_back(std::make_shared<AST::GenericType>(identifier, type));
                if(this->_peekTokenIs(token::TokenType::Comma)) {
                    this->_nextToken();
                    this->_nextToken();
                } else {
                    break;
                }
            } else {
                _peekError(current_token->type, token::TokenType::Identifier);
                break;
            }
        }
        if(!this->_expectPeek(token::TokenType::RightParen)) {
            return nullptr;
        }
        if(this->_peekTokenIs(token::TokenType::Def)) {
            this->_nextToken();
            auto func = this->_parseFunctionStatement();
            func->generic = generics;
            return func;
        } else if(this->_peekTokenIs(token::TokenType::Struct)) {
            this->_nextToken();
            auto _struct = this->_parseStructStatement();
            _struct->generics = generics;
            return _struct;
        }
        this->_peekError(this->current_token->type, token::TokenType::Def);
    }
    return nullptr;
};

std::shared_ptr<AST::FunctionStatement> parser::Parser::_parseFunctionStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    if(!this->_expectPeek(token::TokenType::Identifier)) {
        return nullptr;
    }
    auto name = std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);
    name->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    if(!this->_expectPeek(token::TokenType::LeftParen)) {
        return nullptr;
    }
    this->_nextToken();
    std::vector<std::shared_ptr<AST::FunctionParameter>> parameters;
    while(this->current_token->type != token::TokenType::RightParen) {
        if(this->current_token->type == token::TokenType::Identifier) {
            auto identifier = std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);
            if(!this->_expectPeek(token::TokenType::Colon)) {
                return nullptr;
            }
            this->_nextToken();
            auto type = this->_parseType();
            parameters.push_back(std::make_shared<AST::FunctionParameter>(identifier, type));
            this->_nextToken();
            if(this->current_token->type == token::TokenType::Comma) {
                this->_nextToken();
                continue;
            } else if(this->current_token->type == token::TokenType::RightParen) {
                break;
            } else {
                _peekError(current_token->type, token::TokenType::Comma);
                break;
            }
        } else {
            _peekError(current_token->type, token::TokenType::Identifier);
            break;
        }
    }
    std::vector<std::shared_ptr<AST::FunctionParameter>> closure_parameters;
    if(this->_peekTokenIs(token::TokenType::Use)) {
        this->_nextToken();
        this->_nextToken();
        this->_nextToken();
        while(this->current_token->type != token::TokenType::RightParen) {
            if(this->current_token->type == token::TokenType::Identifier) {
                auto identifier = std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);
                if(!this->_expectPeek(token::TokenType::Colon)) {
                    return nullptr;
                }
                this->_nextToken();
                auto type = this->_parseType();
                closure_parameters.push_back(std::make_shared<AST::FunctionParameter>(identifier, type));
                this->_nextToken();
                if(this->current_token->type == token::TokenType::Comma) {
                    this->_nextToken();
                    continue;
                } else if(this->current_token->type == token::TokenType::RightParen) {
                    break;
                } else {
                    _peekError(current_token->type, token::TokenType::Comma);
                    break;
                }
            } else {
                _peekError(current_token->type, token::TokenType::Identifier);
                break;
            }
        }
    }
    if(!this->_expectPeek(token::TokenType::RightArrow)) {
        return nullptr;
    }
    this->_nextToken();
    auto return_type = this->_parseType();
    std::shared_ptr<AST::BlockStatement> body = nullptr;
    if(this->_peekTokenIs(token::TokenType::Semicolon)) {
        this->_nextToken();
    } else {
        if(!this->_expectPeek(token::TokenType::LeftBrace)) {
            return nullptr;
        }
        body = this->_parseBlockStatement();
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto function_statement = std::make_shared<AST::FunctionStatement>(name, parameters, closure_parameters, return_type, body, std::vector<std::shared_ptr<AST::GenericType>>{});
    function_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return function_statement;
}

std::shared_ptr<AST::WhileStatement> parser::Parser::_parseWhileStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    if(!this->_expectPeek(token::TokenType::LeftParen)) {
        return nullptr;
    }
    this->_nextToken();
    auto condition = this->_parseExpression(PrecedenceType::LOWEST);
    if(!this->_expectPeek(token::TokenType::RightParen)) {
        return nullptr;
    }
    this->_nextToken();
    auto body = this->_parseStatement();
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto while_statement = std::make_shared<AST::WhileStatement>(condition, body);
    while_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return while_statement;
}

std::shared_ptr<AST::BreakStatement> parser::Parser::_parseBreakStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    this->_nextToken();
    int loopNum = 0;
    if(this->_currentTokenIs(token::TokenType::Integer)) {
        loopNum = std::stoi(current_token->literal);
        this->_nextToken();
    }
    if(this->_currentTokenIs(token::TokenType::Semicolon))
        this->_nextToken();
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto break_statement = std::make_shared<AST::BreakStatement>(loopNum);
    break_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return break_statement;
}

std::shared_ptr<AST::ContinueStatement> parser::Parser::_parseContinueStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    this->_nextToken();
    int loopNum = 0;
    if(this->_currentTokenIs(token::TokenType::Integer)) {
        loopNum = std::stoi(current_token->literal);
        this->_nextToken();
    }
    if(this->_currentTokenIs(token::TokenType::Semicolon))
        this->_nextToken();
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto continue_statement = std::make_shared<AST::ContinueStatement>(loopNum);
    continue_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return continue_statement;
}

std::shared_ptr<AST::ImportStatement> parser::Parser::_parseImportStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    if(!this->_expectPeek(token::TokenType::String)) {
        return nullptr;
    }
    auto import_statement = std::make_shared<AST::ImportStatement>(this->current_token->literal);
    if(!this->_expectPeek(token::TokenType::Semicolon)) {
        return nullptr;
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    import_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return import_statement;
}

std::shared_ptr<AST::Expression> parser::Parser::_parseFunctionCall(std::shared_ptr<AST::Expression> identifier, int st_line_no, int st_col_no) {
    if(!identifier) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        identifier = std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);
    }
    identifier->set_meta_data(st_line_no, st_col_no, current_token->line_no, current_token->end_col_no);
    this->_nextToken();
    auto args = this->_parse_expression_list(token::TokenType::RightParen);
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto call_expression = std::make_shared<AST::CallExpression>(identifier, args);
    call_expression->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return call_expression;
}

std::vector<std::shared_ptr<AST::Expression>> parser::Parser::_parse_expression_list(token::TokenType end) {
    std::vector<std::shared_ptr<AST::Expression>> args;
    if(this->_peekTokenIs(end)) {
        this->_nextToken();
        return args;
    }
    this->_nextToken();
    args.push_back(this->_parseExpression(PrecedenceType::LOWEST));
    while(this->_peekTokenIs(token::TokenType::Comma)) {
        this->_nextToken();
        this->_nextToken();
        args.push_back(this->_parseExpression(PrecedenceType::LOWEST));
    }
    if(!this->_expectPeek(end)) {
        return std::vector<std::shared_ptr<AST::Expression>>{};
    }
    return args;
}

std::shared_ptr<AST::ReturnStatement> parser::Parser::_parseReturnStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    this->_nextToken();
    auto expr = this->_parseExpression(PrecedenceType::LOWEST);
    if(this->_peekTokenIs(token::TokenType::Semicolon)) {
        this->_nextToken();
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto return_statement = std::make_shared<AST::ReturnStatement>(expr);
    return_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return return_statement;
}

std::shared_ptr<AST::BlockStatement> parser::Parser::_parseBlockStatement() {
    this->_nextToken();


    std::vector<std::shared_ptr<AST::Statement>> statements;
    while(!this->_currentTokenIs(token::TokenType::RightBrace) && !this->_currentTokenIs(token::TokenType::EndOfFile)) {
        auto stmt = this->_parseStatement();
        if(stmt != nullptr) {
            statements.push_back(stmt);
        }
        this->_nextToken();
    }
    if(this->_peekTokenIs(token::TokenType::Semicolon)) {
        this->_nextToken();
    }


    auto block_statement = std::make_shared<AST::BlockStatement>(statements);
    return block_statement;
}

std::shared_ptr<AST::ExpressionStatement> parser::Parser::_parseExpressionStatement(std::shared_ptr<AST::Expression> identifier, int st_line_no, int st_col_no) {
    if(identifier == nullptr) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        identifier = std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);
    }
    auto expr = this->_parseExpression(PrecedenceType::LOWEST, identifier, st_line_no, st_col_no);
    if(this->_peekTokenIs(token::TokenType::Semicolon)) {
        this->_nextToken();
    }
    auto stmt = std::make_shared<AST::ExpressionStatement>(expr);
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    stmt->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return stmt;
}

std::shared_ptr<AST::Statement> parser::Parser::_parseVariableDeclaration(std::shared_ptr<AST::Expression> identifier, int st_line_no, int st_col_no, bool is_volatile) {
    if(identifier == nullptr) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        identifier = std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);
    }
    if(!this->_expectPeek(token::TokenType::Colon)) {
        return nullptr;
    }
    this->_nextToken();
    auto type = this->_parseType();
    if(this->peek_token->type == token::TokenType::Semicolon) {
        this->_nextToken();
        int end_line_no = current_token->line_no;
        int end_col_no = current_token->col_no;
        auto variableDeclarationStatement = std::make_shared<AST::VariableDeclarationStatement>(identifier, type, nullptr, is_volatile);
        variableDeclarationStatement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
        variableDeclarationStatement->meta_data.more_data["name_line_no"] = st_line_no;
        variableDeclarationStatement->meta_data.more_data["name_col_no"] = st_col_no;
        variableDeclarationStatement->meta_data.more_data["name_end_col_no"] = current_token->end_col_no;
        return variableDeclarationStatement;
    } else if(this->_expectPeek(token::TokenType::Equals)) {
        this->_nextToken();
        auto expr = this->_parseExpression(PrecedenceType::LOWEST);
        this->_nextToken();
        int end_line_no = current_token->line_no;
        int end_col_no = current_token->col_no;
        auto variableDeclarationStatement = std::make_shared<AST::VariableDeclarationStatement>(identifier, type, expr, is_volatile);
        variableDeclarationStatement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
        variableDeclarationStatement->meta_data.more_data["name_line_no"] = st_line_no;
        variableDeclarationStatement->meta_data.more_data["name_col_no"] = st_col_no;
        variableDeclarationStatement->meta_data.more_data["name_end_col_no"] = current_token->end_col_no;
        return variableDeclarationStatement;
    }
    return nullptr;
}

std::shared_ptr<AST::Expression> parser::Parser::_parseInfixIdenifier() {
    if(this->current_token->type != token::TokenType::Identifier) {
        std::cerr << "Cant parser infixIdentifier Expression" << std::endl;
        exit(1);
    }
    if(!this->_peekTokenIs(token::TokenType::Dot)) {
        return std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);
    }
    auto li = std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);
    this->_nextToken();
    this->_nextToken();
    return std::make_shared<AST::InfixExpression>(li, token::TokenType::Dot, ".", this->_parseInfixIdenifier());
}

std::shared_ptr<AST::Type> parser::Parser::_parseType() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    std::shared_ptr<AST::Expression> name;
    name = this->_parseInfixIdenifier();
    std::vector<std::shared_ptr<AST::Type>> generics;
    if(this->_peekTokenIs(token::TokenType::LeftBracket)) {
        this->_nextToken();
        this->_nextToken();
        while(this->current_token->type != token::TokenType::RightBracket) {
            auto generic = this->_parseType();
            generics.push_back(generic);
            this->_nextToken();
            if(this->current_token->type == token::TokenType::Comma) {
                this->_nextToken();
            }
        }
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto generic_type_node = std::make_shared<AST::Type>(name, generics);
    generic_type_node->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return generic_type_node;
}

std::shared_ptr<AST::Statement> parser::Parser::_parseVariableAssignment(std::shared_ptr<AST::Expression> identifier, int st_line_no, int st_col_no) {
    if(identifier == nullptr) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        identifier = std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);
    }
    if(!this->_expectPeek(token::TokenType::Equals)) {
        return nullptr;
    }
    this->_nextToken();
    auto expr = this->_parseExpression(PrecedenceType::LOWEST);
    this->_nextToken();
    auto stmt = std::make_shared<AST::VariableAssignmentStatement>(identifier, expr);
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    stmt->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return stmt;
}

std::shared_ptr<AST::StructStatement> parser::Parser::_parseStructStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;

    if(!this->_expectPeek(token::TokenType::Identifier)) {
        return nullptr;
    }
    std::shared_ptr<AST::Expression> name = std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);

    if(!this->_expectPeek(token::TokenType::LeftBrace)) {
        return nullptr;
    }
    this->_nextToken();
    std::vector<std::shared_ptr<AST::Statement>> statements;

    while(!this->_currentTokenIs(token::TokenType::RightBrace) && !this->_currentTokenIs(token::TokenType::EndOfFile)) {
        if(this->_currentTokenIs(token::TokenType::Def)) {
            auto stmt = this->_parseFunctionStatement();
            if(stmt != nullptr) {
                statements.push_back(stmt);
            } else {
            }
            this->_nextToken();
            continue;
        }
        auto stmt = this->_parseVariableDeclaration();
        if(stmt != nullptr) {
            statements.push_back(stmt);
        } else {
        }
        this->_nextToken();
    }
    if(this->peek_token->type == token::TokenType::Semicolon) {
        this->_nextToken();
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;

    auto struct_stmt = std::make_shared<AST::StructStatement>(name, statements);
    struct_stmt->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return struct_stmt;
}

std::shared_ptr<AST::Expression> parser::Parser::_parseExpression(PrecedenceType precedence, std::shared_ptr<AST::Expression> parsed_expression, int st_line_no, int st_col_no) {
    if(parsed_expression == nullptr) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        auto iter = prefix_parse_fns.find(current_token->type);
        if(iter == prefix_parse_fns.end()) {
            this->_noPrefixParseFnError(current_token->type);
            return nullptr;
        }
        auto prefix_fn = iter->second;
        parsed_expression = prefix_fn();
    }
    while(!_peekTokenIs(token::TokenType::Semicolon) && precedence < _peekPrecedence()) {
        auto iter = infix_parse_Fns.find(peek_token->type);
        if(iter == infix_parse_Fns.end()) {
            return parsed_expression;
        }
        this->_nextToken();
        auto infix_fn = iter->second;
        parsed_expression = infix_fn(parsed_expression);
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    parsed_expression->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return parsed_expression;
}

std::shared_ptr<AST::Statement> parser::Parser::_parseIfElseStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    if(!this->_expectPeek(token::TokenType::LeftParen)) {
        return nullptr;
    }
    this->_nextToken();
    auto condition = this->_parseExpression(PrecedenceType::LOWEST);
    if(!this->_expectPeek(token::TokenType::RightParen)) {
        return nullptr;
    }
    this->_nextToken();
    auto consequence = this->_parseStatement();
    std::shared_ptr<AST::Statement> alternative = nullptr;
    if(this->_peekTokenIs(token::TokenType::Else)) {
        this->_nextToken();
        this->_nextToken();
        alternative = this->_parseStatement();
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto if_else_statement = std::make_shared<AST::IfElseStatement>(condition, consequence, alternative);
    if_else_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return if_else_statement;
}

std::shared_ptr<AST::Expression> parser::Parser::_parseInfixExpression(std::shared_ptr<AST::Expression> leftNode) {
    int st_line_no = leftNode->meta_data.st_line_no;
    int st_col_no = leftNode->meta_data.st_col_no;
    auto infix_expr = std::make_shared<AST::InfixExpression>(leftNode, this->current_token->type, this->current_token->literal);
    infix_expr->meta_data.more_data["operator_line_no"] = this->current_token->line_no;
    infix_expr->meta_data.more_data["operator_st_col_no"] = this->current_token->col_no;
    infix_expr->meta_data.more_data["operator_end_col_no"] = this->current_token->end_col_no;
    auto precedence = this->_currentPrecedence();
    this->_nextToken();
    infix_expr->right = this->_parseExpression(precedence);
    int end_line_no = infix_expr->right->meta_data.end_line_no;
    int end_col_no = infix_expr->right->meta_data.end_col_no;
    infix_expr->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return infix_expr;
}

std::shared_ptr<AST::Expression> parser::Parser::_parseIndexExpression(std::shared_ptr<AST::Expression> leftNode) {
    int st_line_no = leftNode->meta_data.st_line_no;
    int st_col_no = leftNode->meta_data.st_col_no;
    auto index_expr = std::make_shared<AST::IndexExpression>(leftNode);
    index_expr->meta_data.more_data["index_line_no"] = this->current_token->line_no;
    index_expr->meta_data.more_data["index_st_col_no"] = this->current_token->col_no;
    index_expr->meta_data.more_data["index_end_col_no"] = this->current_token->end_col_no;
    this->_nextToken();
    index_expr->index = this->_parseExpression(PrecedenceType::INDEX);
    int end_line_no = index_expr->index->meta_data.end_line_no;
    int end_col_no = index_expr->index->meta_data.end_col_no;
    index_expr->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    if(!this->_expectPeek(token::TokenType::RightBracket)) {
        return nullptr;
    }
    return index_expr;
}

std::shared_ptr<AST::Expression> parser::Parser::_parseGroupedExpression() {
    this->_nextToken();
    int st_line_no = this->current_token->line_no;
    int st_col_no = this->current_token->col_no;
    auto expr = this->_parseExpression(PrecedenceType::LOWEST);
    if(!this->_expectPeek(token::TokenType::RightParen)) {
        return nullptr;
    }
    int end_line_no = this->current_token->line_no;
    int end_col_no = this->current_token->end_col_no;
    expr->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return expr;
}


std::shared_ptr<AST::Expression> parser::Parser::_parseIntegerLiteral() {
    auto expr = std::make_shared<AST::IntegerLiteral>(std::stoll(current_token->literal));
    expr->meta_data.st_line_no = current_token->line_no;
    expr->meta_data.st_col_no = current_token->col_no;
    expr->meta_data.end_line_no = current_token->line_no;
    expr->meta_data.end_col_no = current_token->end_col_no;
    return expr;
}

std::shared_ptr<AST::Expression> parser::Parser::_parseFloatLiteral() {
    auto expr = std::make_shared<AST::FloatLiteral>(std::stod(current_token->literal));
    expr->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return expr;
}

std::shared_ptr<AST::Expression> parser::Parser::_parseBooleanLiteral() {
    auto expr = std::make_shared<AST::BooleanLiteral>(current_token->type == token::TokenType::True);
    expr->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return expr;
}

std::shared_ptr<AST::Expression> parser::Parser::_parseStringLiteral() {
    auto expr = std::make_shared<AST::StringLiteral>(current_token->literal);
    expr->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return expr;
}

void parser::Parser::_nextToken() {
    current_token = peek_token;
    peek_token = lexer->nextToken();
}

bool parser::Parser::_currentTokenIs(token::TokenType type) { return current_token->type == type; }

bool parser::Parser::_peekTokenIs(token::TokenType type) { return peek_token->type == type; }

bool parser::Parser::_expectPeek(token::TokenType type) {
    if(_peekTokenIs(type)) {
        _nextToken();
        return true;
    } else {
        _peekError(peek_token->type, type);
        return false;
    }
}

parser::PrecedenceType parser::Parser::_currentPrecedence() {
    auto it = token_precedence.find(current_token->type);
    if(it != token_precedence.end()) {
        return it->second;
    } else {
        return PrecedenceType::LOWEST;
    }
}

parser::PrecedenceType parser::Parser::_peekPrecedence() {
    auto it = token_precedence.find(peek_token->type);
    if(it != token_precedence.end()) {
        return it->second;
    } else {
        return PrecedenceType::LOWEST;
    }
}

std::shared_ptr<AST::Expression> parser::Parser::_parseArrayLiteral() {
    auto elements = std::vector<std::shared_ptr<AST::Expression>>();
    for(_nextToken(); !_currentTokenIs(token::TokenType::RightBracket); _nextToken()) {
        if(_currentTokenIs(token::TokenType::Comma)) {
            continue;
        }
        auto expr = _parseExpression(PrecedenceType::LOWEST);
        if(expr != nullptr) {
            elements.push_back(expr);
        }
    }
    auto array = std::make_shared<AST::ArrayLiteral>(elements);
    array->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return array;
};

std::shared_ptr<AST::Expression> parser::Parser::_parseIdentifier() {
    if(this->current_token->type != token::TokenType::Identifier) {
        std::cerr << this->current_token->literal + " is not Identifier" << std::endl;
        exit(1);
    }
    auto identifier = std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);
    if(_peekTokenIs(token::TokenType::LeftParen)) {
        int st_line_no = this->current_token->line_no;
        int st_col_no = this->current_token->col_no;
        return _parseFunctionCall(std::make_shared<AST::IdentifierLiteral>(this->current_token->literal), st_line_no, st_col_no);
    }
    identifier->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return identifier;
}

void parser::Parser::_peekError(token::TokenType type, token::TokenType expected_type, std::string suggestedFix) {
    std::shared_ptr<errors::SyntaxError> error = std::make_shared<errors::SyntaxError>(
        "SyntaxError", this->lexer->source, *peek_token, "Expected to be " + *token::tokenTypeString(expected_type) + ", but got " + *token::tokenTypeString(type), suggestedFix);
    this->errors.push_back(error);
}

void parser::Parser::_noPrefixParseFnError(token::TokenType type) {
    std::shared_ptr<errors::NoPrefixParseFnError> error =
        std::make_shared<errors::NoPrefixParseFnError>(this->lexer->source, *peek_token, "No prefix parse function for " + *token::tokenTypeString(type));
    this->errors.push_back(error);
}
