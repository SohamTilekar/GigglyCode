#include "parser.hpp"

parser::Parser::Parser(std::shared_ptr<Lexer> lexer) {
    this->lexer = lexer;
    current_token = nullptr;
    peek_token = nullptr;
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
        auto identifier = this->_parseIdentifier();
        if(this->_peekTokenIs(token::TokenType::Colon)) {
            return this->_parseVariableDeclaration(identifier, st_line_no, st_col_no);
        } else if(this->_peekTokenIs(token::TokenType::Equals)) {
            return this->_parseVariableAssignment(identifier, st_line_no, st_col_no);
        }else {
            return this->_parseExpressionStatement(identifier, st_line_no, st_col_no);
        }
    } else if(this->_currentTokenIs(token::TokenType::LeftBrace)) {
        return this->_parseBlockStatement();
    } else if(this->_currentTokenIs(token::TokenType::Return)) {
        return this->_parseReturnStatement();
    } else if(this->_currentTokenIs(token::TokenType::Def)) {
        return this->_parseFunctionStatement();
    } else if(this->_currentTokenIs(token::TokenType::If)) {
        return this->_parseIfElseStatement();
    } else if(this->_currentTokenIs(token::TokenType::While)) {
        return this->_parseWhileStatement();
    } else if(this->_currentTokenIs(token::TokenType::Break)) {
        return this->_parseBreakStatement();
    } else if(this->_currentTokenIs(token::TokenType::Continue)) {
        return this->_parseContinueStatement();
    } else if(this->_currentTokenIs(token::TokenType::Class)) {
        return this->_parseClassDeclarationStatement();
    } else
        return this->_parseExpressionStatement();
}

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
            auto identifier = _parseIdentifier();
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
    if(!this->_expectPeek(token::TokenType::RightArrow)) {
        return nullptr;
    }
    this->_nextToken();
    auto return_type = this->_parseType();
    if(!this->_expectPeek(token::TokenType::LeftBrace)) {
        return nullptr;
    }
    auto body = this->_parseBlockStatement();
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto function_statement = std::make_shared<AST::FunctionStatement>(name, parameters, return_type, body);
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
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto break_statement = std::make_shared<AST::BreakStatement>();
    break_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return break_statement;
}

std::shared_ptr<AST::ContinueStatement> parser::Parser::_parseContinueStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    this->_nextToken();
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto continue_statement = std::make_shared<AST::ContinueStatement>();
    continue_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return continue_statement;
}

std::shared_ptr<AST::Expression> parser::Parser::_parseFunctionCall(std::shared_ptr<AST::Expression> identifier, int st_line_no, int st_col_no) {
    if (identifier == nullptr) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        identifier = _parseIdentifier();
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
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    std::vector<std::shared_ptr<AST::Statement>> statements;
    while(!this->_currentTokenIs(token::TokenType::RightBrace) && !this->_currentTokenIs(token::TokenType::EndOfFile)) {
        auto stmt = this->_parseStatement();
        if(stmt != nullptr) {
            statements.push_back(stmt);
        }
        this->_nextToken();
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto block_statement = std::make_shared<AST::BlockStatement>(statements);
    return block_statement;
}

std::shared_ptr<AST::ExpressionStatement> parser::Parser::_parseExpressionStatement(std::shared_ptr<AST::Expression> identifier, int st_line_no, int st_col_no) {
    if (identifier == nullptr) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        identifier = _parseIdentifier();
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

std::shared_ptr<AST::Statement> parser::Parser::_parseVariableDeclaration(std::shared_ptr<AST::Expression> identifier, int st_line_no, int st_col_no) {
    if (identifier == nullptr) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        identifier = _parseIdentifier();
    }
    if (!this->_expectPeek(token::TokenType::Colon)) {
        return nullptr;
    }
    this->_nextToken();
    auto type = this->_parseType();
    if(this->peek_token->type == token::TokenType::Semicolon) {
        this->_nextToken();
        int end_line_no = current_token->line_no;
        int end_col_no = current_token->col_no;
        auto variableDeclarationStatement = std::make_shared<AST::VariableDeclarationStatement>(identifier, type);
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
        auto variableDeclarationStatement = std::make_shared<AST::VariableDeclarationStatement>(identifier, type, expr);
        variableDeclarationStatement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
        variableDeclarationStatement->meta_data.more_data["name_line_no"] = st_line_no;
        variableDeclarationStatement->meta_data.more_data["name_col_no"] = st_col_no;
        variableDeclarationStatement->meta_data.more_data["name_end_col_no"] = current_token->end_col_no;
        return variableDeclarationStatement;
    }
    return nullptr;
}

std::shared_ptr<AST::BaseType> parser::Parser::_parseType() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    std::shared_ptr<AST::Expression> name;
    if(this->_currentTokenIs(token::TokenType::Integer))
        name = std::make_shared<AST::IntegerLiteral>(std::stoll(this->current_token->literal));
    else if(this->_currentTokenIs(token::TokenType::Float))
        name = std::make_shared<AST::FloatLiteral>(std::stod(this->current_token->literal));
    else if(this->_currentTokenIs(token::TokenType::String))
        name = std::make_shared<AST::StringLiteral>(this->current_token->literal);
    else if(this->_currentTokenIs(token::TokenType::RawString))
        name = std::make_shared<AST::StringLiteral>(this->current_token->literal);
    else
        name = _parseIdentifier();
    std::vector<std::shared_ptr<AST::BaseType>> generics;
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
    auto generic_type_node = std::make_shared<AST::GenericType>(name, generics);
    generic_type_node->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return generic_type_node;
}

std::shared_ptr<AST::Statement> parser::Parser::_parseVariableAssignment(std::shared_ptr<AST::Expression> identifier, int st_line_no, int st_col_no) {
    if (identifier == nullptr) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        identifier = _parseIdentifier();
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

std::shared_ptr<AST::ClassDeclarationStatement> parser::Parser::_parseClassDeclarationStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    if(!this->_expectPeek(token::TokenType::Identifier)) {
        return nullptr;
    }
    auto name = std::make_shared<AST::IdentifierLiteral>(this->current_token->literal);
    name->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    if(!this->_expectPeek(token::TokenType::LeftBrace)) {
        return nullptr;
    }
    this->_nextToken();
    std::vector<std::shared_ptr<AST::VariableDeclarationStatement>> variables;
    std::vector<std::shared_ptr<AST::FunctionStatement>> methods;
    while(this->current_token->type != token::TokenType::RightBrace && this->current_token->type != token::TokenType::EndOfFile) {
        if(this->current_token->type == token::TokenType::Def) {
            auto method = this->_parseFunctionStatement();
            this->_nextToken();
            if(method != nullptr) {
                methods.push_back(std::dynamic_pointer_cast<AST::FunctionStatement>(method));
            }
            else {
                std::cout << "Method is null" << std::endl;
                break;
            }
        } else if(this->current_token->type == token::TokenType::Identifier) {
            auto variable = this->_parseVariableDeclaration();
            this->_nextToken();
            if(variable != nullptr) {
                variables.push_back(std::dynamic_pointer_cast<AST::VariableDeclarationStatement>(variable));
            }
            else {
                std::cout << "Variable is null" << std::endl;
                break;
            }
        } else {
            std::cout << "Unexpected token: " << *token::tokenTypeString(this->current_token->type) << std::endl;
            break;
        }
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto class_declaration_statement = std::make_shared<AST::ClassDeclarationStatement>(name, variables, methods);
    class_declaration_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return class_declaration_statement;
};

std::shared_ptr<AST::Expression> parser::Parser::_parseExpression(PrecedenceType precedence, std::shared_ptr<AST::Expression> parsed_expression, int st_line_no, int st_col_no) {
    if (parsed_expression == nullptr) {
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

std::shared_ptr<AST::Expression> parser::Parser::_parseIdentifier() {
    std::string identifier = current_token->literal;
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    while (_peekTokenIs(token::TokenType::Dot)) {
        _nextToken();
        if (!_expectPeek(token::TokenType::Identifier)) {
            return nullptr;
        }
        identifier += "." + current_token->literal;
    }
    if (_peekTokenIs(token::TokenType::LeftParen)) {
        return _parseFunctionCall(std::make_shared<AST::IdentifierLiteral>(identifier), st_line_no, st_col_no);
    }
    auto expr = std::make_shared<AST::IdentifierLiteral>(identifier);
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

void parser::Parser::_peekError(token::TokenType type, token::TokenType expected_type, std::string suggestedFix) {
    std::shared_ptr<errors::SyntaxError> error = std::make_shared<errors::SyntaxError>(
        this->lexer->source, *peek_token, "Expected to be " + *token::tokenTypeString(expected_type) + ", but got " + *token::tokenTypeString(type),
        suggestedFix);
    this->errors.push_back(error);
}

void parser::Parser::_noPrefixParseFnError(token::TokenType type) {
    std::shared_ptr<errors::NoPrefixParseFnError> error = std::make_shared<errors::NoPrefixParseFnError>(
        this->lexer->source, *peek_token, "No prefix parse function for " + *token::tokenTypeString(type));
    this->errors.push_back(error);
}