#include <cstdlib>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "AST/ast.hpp"
#include "parser.hpp"

using namespace parser;

// Flags to indicate specific parsing states
bool is_new_call = false;
bool is_new_arr = false;

/*
Conventions for Token Representation:
- `-i`: Represents an integer, indicating a specific instance or count, appended after declaration.
- `[TokenType]`: Encloses the current token type within square brackets.
- `stmt`: Represents a statement token.
- `exp`: Represents an expression token.
- `{Name}`: Represents a specific kind of token by name.
- `FT`: First Token in a sequence.
- `LT`: Last Token in a sequence.
- `|`: Denotes alternative tokens or outcomes.
- `->`: Indicates the transition from the previous state to the new state after processing the current line.
- `stmtLT`: Typically represents a semicolon (`;`).
- `!` after `]`: Indicates that the token could also be unknown or optional.
*/

Parser::Parser(LexerPtr lexer) : lexer(lexer) {
    // Initialize with the first two tokens
    this->_nextToken(); // [FT]
    this->_nextToken(); // [LT]
}

shared_ptr<AST::Program> Parser::parseProgram() {
    auto program = make_shared<AST::Program>();
    int startLineNo = current_token->line_no;
    int startColNo = current_token->col_no;

    // Parse statements until EndOfFile token is reached
    while (current_token->type != TokenType::EndOfFile) {
        auto statement = this->_parseStatement(); // [stmtFT] -> [stmtLT]
        if (statement) { program->statements.push_back(statement); }
        this->_nextToken(); // [stmtLT] -> [stmtFT | EOF]
    } // [EOF]

    int endLineNo = current_token->line_no;
    int endColNo = current_token->col_no;
    program->set_meta_data(startLineNo, startColNo, endLineNo, endColNo);
    return program;
}

StatementPtr Parser::_parseStatement() {
    switch (current_token->type) {
        case TokenType::Identifier:
            return this->_interpretIdentifier(); // [IdentifierFT] -> [;]
        case TokenType::LeftBrace:
            return this->_parseBlockStatement(); // [{FT] -> [; | }]
        case TokenType::Return:
            return this->_parseReturnStatement(); // [returnFT] -> [;]
        case TokenType::Raise:
            return this->_parseRaiseStatement(); // [raiseFT] -> [;]
        case TokenType::Def:
            return this->_parseFunctionStatement(); // [defFT] -> [; | }]
        case TokenType::AtTheRate:
            return this->_parseDeco(); // [@FT] -> [;]
        case TokenType::If:
            return this->_parseIfElseStatement(); // [ifFT] -> [}]
        case TokenType::While:
            return this->_parseWhileStatement(); // [whileFT] -> [}]
        case TokenType::For:
            return this->_parseForStatement(); // [forFT] -> [}]
        case TokenType::Break:
            return this->_parseBreakStatement(); // [breakFT] -> [;]
        case TokenType::Continue:
            return this->_parseContinueStatement(); // [continueFT] -> [;]
        case TokenType::Import:
            return this->_parseImportStatement(); // [importFT] -> [;]
        case TokenType::Struct:
            return this->_parseStructStatement(); // [structFT] -> [}]
        case TokenType::Try:
            return this->_parseTryCatchStatement(); // [tryFT] -> [;]
        case TokenType::Volatile:
            this->_nextToken();                                            // [volatileFT] -> [Identifier!]
            return this->_parseVariableDeclaration(nullptr, -1, -1, true); // [Identifier!FT] -> [;]
        default:
            return this->_parseExpressionStatement(); // [ExpressionFT] -> [;]
    }
}

StatementPtr Parser::_interpretIdentifier() {
    int stLineNo = current_token->line_no;
    int stColNo = current_token->col_no;
    auto identifier = _parseIdentifier(); // Parse the identifier

    if (_peekTokenIs(TokenType::Colon)) {
        // Variable declaration
        return _parseVariableDeclaration(identifier, stLineNo, stColNo);
    }

    if (_peekTokenIs(TokenType::Equals)) {
        // Variable assignment
        return _parseVariableAssignment(identifier, stLineNo, stColNo);
    }

    if (_peekTokenIs(TokenType::LeftParen)) {
        // Function call
        auto functionCall = _parseFunctionCall(identifier, stLineNo, stColNo);
        auto stmt = make_shared<AST::ExpressionStatement>(functionCall);
        _expectPeek(TokenType::Semicolon); // Expect ';' after function call
        return stmt;
    }

    // Default to expression statement
    return _parseExpressionStatement(identifier, stLineNo, stColNo);
}

StatementPtr Parser::_parseDeco() {
    _expectPeek(TokenType::Identifier); // [@FT] -> [Identifier]
    std::string name = this->current_token->literal;

    if (name == "generic") {
        return _parseGenericDeco(); // [IdentifierFT] -> [)]
    } else if (name == "autocast") {
        return _parseAutocastDeco(); // [IdentifierFT] -> [)]
    }
    errors::SyntaxError("WrongDecoName", this->lexer->source, *this->current_token, "Unknown Deco type: " + name, "Check the deco name for case sensitivity. Valid options: `autocast` or `generic`.")
        .raise();
}

StatementPtr Parser::_parseGenericDeco() {
    this->_expectPeek(TokenType::LeftParen); // [Identifier] -> [(]
    this->_nextToken();                      // [(] -> [Identifier | )]!
    std::vector<shared_ptr<AST::GenericType>> generics;

    while (this->current_token->type != TokenType::RightParen) {
        if (this->_currentTokenIs(TokenType::Identifier)) {
            auto identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
            this->_expectPeek(TokenType::Colon); // [Identifier] -> [:]
            this->_nextToken();                  // [:] -> [Type]
            std::vector<shared_ptr<AST::Type>> type;

            while (this->current_token->type != TokenType::RightParen && this->current_token->type != TokenType::Comma) {
                type.push_back(this->_parseType()); // [Type] remains unchanged
                if (this->_peekTokenIs(TokenType::Pipe)) {
                    this->_nextToken(); // [Type] -> [|]
                    this->_nextToken(); // [|] -> [Next Type]
                } else break;
            }

            generics.push_back(make_shared<AST::GenericType>(identifier, type));
            if (this->_peekTokenIs(TokenType::Comma)) {
                this->_nextToken(); // [Type] -> [,]
                this->_nextToken(); // [,] -> [Next Identifier]
            } else break;
        } else {
            _currentTokenError(current_token->type, {TokenType::Identifier});
            break;
        }
    }

    this->_expectPeek(TokenType::RightParen);                                     // [Generic Parameters] -> [RightParen]
    this->_expectPeek({TokenType::Def, TokenType::Struct, TokenType::AtTheRate}); // [RightParen] -> [Def | Struct | @]

    if (this->_currentTokenIs(TokenType::Def)) {
        auto func = this->_parseFunctionStatement(); // [Def] -> [Function Statement]
        func->generic = generics;
        return func;
    } else if (this->_currentTokenIs(TokenType::Struct)) {
        auto _struct = this->_parseStructStatement(); // [Struct] -> [Struct Statement]
        _struct->generics = generics;
        return _struct;
    } else if (this->_currentTokenIs(TokenType::AtTheRate)) {
        auto _deco = this->_parseDeco(); // [@] -> [Decorator Statement]
        if (_deco->type() == AST::NodeType::FunctionStatement) {
            auto deco = std::static_pointer_cast<AST::FunctionStatement>(_deco);
            deco->generic = generics;
            return deco;
        } else if (_deco->type() == AST::NodeType::StructStatement) {
            auto deco = std::static_pointer_cast<AST::StructStatement>(_deco);
            deco->generics = generics;
            return deco;
        }
    }

    this->_currentTokenError(this->current_token->type, {TokenType::Def, TokenType::AtTheRate});
}

StatementPtr Parser::_parseAutocastDeco() {
    this->_expectPeek({TokenType::Def, TokenType::Struct, TokenType::AtTheRate}); // [autocast] -> [Def | Struct | @]

    if (this->_currentTokenIs(TokenType::Def)) {
        auto func = this->_parseFunctionStatement(); // [Def] -> [Function Statement]
        func->extra_info["autocast"] = true;
        return func;
    } else if (this->_currentTokenIs(TokenType::AtTheRate)) {
        auto _deco = this->_parseDeco(); // [@] -> [Decorator Statement]
        if (_deco->type() == AST::NodeType::FunctionStatement) {
            auto deco = std::static_pointer_cast<AST::FunctionStatement>(_deco);
            deco->extra_info["autocast"] = true;
            return deco;
        }
    }

    this->_currentTokenError(this->current_token->type, {TokenType::Def, TokenType::AtTheRate});
}

std::vector<shared_ptr<AST::FunctionParameter>> Parser::_parseFunctionParameters() {
    std::vector<shared_ptr<AST::FunctionParameter>> parameters;
    while (this->current_token->type != TokenType::RightParen) {
        if (this->current_token->type == TokenType::Identifier) {
            auto identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
            this->_expectPeek(TokenType::Colon); // Expect ':' after parameter name
            this->_nextToken();                  // Move to parameter type
            auto type = this->_parseType();
            parameters.push_back(make_shared<AST::FunctionParameter>(identifier, type));
            this->_expectPeek({TokenType::Comma, TokenType::RightParen});
            if (this->_currentTokenIs(TokenType::Comma)) {
                this->_nextToken(); // Consume ',' and continue
                continue;
            } else if (this->_currentTokenIs(TokenType::RightParen)) {
                break;
            }
        } else {
            _currentTokenError(current_token->type, {TokenType::Identifier});
            break;
        }
    }
    return parameters;
}

std::vector<shared_ptr<AST::FunctionParameter>> Parser::_parseClosureParameters() {
    std::vector<shared_ptr<AST::FunctionParameter>> closure_parameters;
    while (!this->_currentTokenIs(TokenType::RightParen)) {
        if (this->_currentTokenIs(TokenType::Identifier)) {
            auto identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
            this->_expectPeek(TokenType::Colon); // Expect ':' after closure parameter name
            this->_nextToken();                  // Move to closure parameter type
            auto type = this->_parseType();
            closure_parameters.push_back(make_shared<AST::FunctionParameter>(identifier, type));
            this->_nextToken(); // Consume ',' or ')'
            if (this->_currentTokenIs(TokenType::Comma)) {
                this->_nextToken(); // Consume ',' and continue
                continue;
            } else if (this->_currentTokenIs(TokenType::RightParen)) {
                break;
            } else {
                _currentTokenError(current_token->type, {TokenType::Comma, TokenType::RightParen});
                break;
            }
        } else {
            _currentTokenError(current_token->type, {TokenType::Identifier});
            break;
        }
    }
    return closure_parameters;
}

shared_ptr<AST::FunctionStatement> Parser::_parseFunctionStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;

    this->_expectPeek(TokenType::Identifier); // Expect function name
    auto name = make_shared<AST::IdentifierLiteral>(this->current_token);
    name->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);

    this->_expectPeek(TokenType::LeftParen); // Expect '(' after function name
    this->_nextToken();                      // Move to parameters

    auto parameters = _parseFunctionParameters();

    // Handle closure parameters if present
    std::vector<shared_ptr<AST::FunctionParameter>> closure_parameters;
    if (this->_peekTokenIs(TokenType::Use)) {
        this->_nextToken(); // Consume 'use' keyword
        this->_nextToken(); // Consume '('
        this->_nextToken(); // Move to closure parameters
        closure_parameters = _parseClosureParameters();
    }

    // Handle return type if present
    shared_ptr<AST::Type> return_type = nullptr;
    if (this->_peekTokenIs(TokenType::RightArrow)) {
        this->_nextToken(); // Consume '->'
        this->_nextToken(); // Move to return type
        return_type = this->_parseType();
    }

    // Parse function body or expect ';'
    shared_ptr<AST::BlockStatement> body = nullptr;
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken(); // Consume ';' indicating no body
    } else {
        this->_expectPeek(TokenType::LeftBrace); // Expect '{' to begin function body
        body = this->_parseBlockStatement();
    }

    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;

    auto function_statement = make_shared<AST::FunctionStatement>(name, parameters, closure_parameters, return_type, body, std::vector<shared_ptr<AST::GenericType>>{});

    function_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return function_statement;
}

shared_ptr<AST::WhileStatement> Parser::_parseWhileStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;

    _expectPeek(TokenType::LeftParen); // Expect '(' after 'while'
    _nextToken();                      // Move to condition expression

    auto condition = _parseExpression(PrecedenceType::LOWEST);

    _expectPeek(TokenType::RightParen); // Expect ')' after condition
    _nextToken();                       // Move to loop body

    auto body = _parseStatement(); // Parse the loop body

    LoopModifiers modifiers = _parseLoopModifiers(); // Parse any loop modifiers

    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;

    auto while_statement = make_shared<AST::WhileStatement>(condition, body, modifiers.ifbreak, modifiers.notbreak);

    while_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return while_statement;
}

shared_ptr<AST::ForStatement> Parser::_parseForStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;

    _expectPeek(TokenType::LeftParen);  // Expect '(' after 'for'
    _expectPeek(TokenType::Identifier); // Expect identifier in 'for (identifier in ...)

    auto get = make_shared<AST::IdentifierLiteral>(current_token);

    _expectPeek(TokenType::In); // Expect 'in' keyword
    _nextToken();               // Move to the 'from' expression

    auto from = _parseExpression(PrecedenceType::LOWEST); // Parse 'from' expression

    _expectPeek(TokenType::RightParen); // Expect ')' after 'in' expression
    _nextToken();                       // Move to loop body

    auto body = _parseStatement(); // Parse the loop body

    LoopModifiers modifiers = _parseLoopModifiers(); // Parse any loop modifiers

    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;

    auto for_statement = make_shared<AST::ForStatement>(get, from, body, modifiers.ifbreak, modifiers.notbreak);

    for_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return for_statement;
}

parser::Parser::LoopModifiers parser::Parser::_parseLoopModifiers() {
    LoopModifiers modifiers = {nullptr, nullptr};
    while (_peekTokenIs(TokenType::NotBreak) || _peekTokenIs(TokenType::IfBreak)) {
        if (_peekTokenIs(TokenType::NotBreak)) {
            _nextToken(); // Consume 'notbreak' token
            _nextToken(); // Move to the statement following 'notbreak'
            modifiers.notbreak = _parseStatement();
        } else if (_peekTokenIs(TokenType::IfBreak)) {
            _nextToken(); // Consume 'ifbreak' token
            _nextToken(); // Move to the statement following 'ifbreak'
            modifiers.ifbreak = _parseStatement();
        }
    }
    return modifiers;
}

shared_ptr<AST::BreakStatement> Parser::_parseBreakStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    this->_nextToken(); // [breakFT] -> [Optional Loop Number]
    int loopNum = 0;
    if (this->_currentTokenIs(TokenType::Integer)) {
        loopNum = std::stoi(current_token->literal); // [IntegerFT] -> [Next Token]
        this->_nextToken();                          // [Next Token] remains unchanged
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto break_statement = make_shared<AST::BreakStatement>(loopNum);
    break_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return break_statement;
}

shared_ptr<AST::ContinueStatement> Parser::_parseContinueStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    this->_nextToken(); // [continueFT] -> [Optional Loop Number]
    int loopNum = 0;
    if (this->_currentTokenIs(TokenType::Integer)) {
        loopNum = std::stoi(current_token->literal); // [IntegerFT] -> [Next Token]
        this->_nextToken();                          // [Next Token] remains unchanged
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto continue_statement = make_shared<AST::ContinueStatement>(loopNum);
    continue_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return continue_statement;
}

shared_ptr<AST::ImportStatement> Parser::_parseImportStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    this->_expectPeek(TokenType::String); // [importFT] -> [String]
    auto import_statement = make_shared<AST::ImportStatement>(this->current_token->literal);
    this->_expectPeek(TokenType::Semicolon); // [String] -> [;]
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    import_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return import_statement;
}

ExpressionPtr Parser::_parseFunctionCall(ExpressionPtr identifier, int st_line_no, int st_col_no) {
    if (!identifier) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
    }
    identifier->set_meta_data(st_line_no, st_col_no, current_token->line_no, current_token->end_col_no);
    std::cout << "Before: ";
    this->current_token->print();
    this->_nextToken(); // [Identifier] -> [(] or [LeftParen]
    bool is_new_call_local = is_new_call;
    is_new_call = false;
    auto args = this->_parse_expression_list(TokenType::RightParen); // [(] -> [Arguments] -> [)]
    std::cout << "After: ";
    this->current_token->print();
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto call_expression = make_shared<AST::CallExpression>(identifier, args);
    call_expression->_new = is_new_call_local;
    call_expression->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return call_expression;
}

std::vector<ExpressionPtr> Parser::_parse_expression_list(TokenType end) {
    std::vector<ExpressionPtr> args;
    if (this->_peekTokenIs(end)) {
        this->_nextToken(); // [LeftParen] -> [end]
        return args;
    }
    this->_nextToken(); // [LeftParen] -> [First Argument]
    args.push_back(this->_parseExpression(PrecedenceType::LOWEST));
    while (this->_peekTokenIs(TokenType::Comma)) {
        this->_nextToken(); // [Argument] -> [,]
        this->_nextToken(); // [,] -> [Next Argument]
        args.push_back(this->_parseExpression(PrecedenceType::LOWEST));
    }
    this->_expectPeek(end); // [Last Argument] -> [end]
    return args;
}

shared_ptr<AST::ReturnStatement> Parser::_parseReturnStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken(); // [returnFT] -> [;]
        int end_line_no = current_token->line_no;
        int end_col_no = current_token->col_no;
        auto return_statement = make_shared<AST::ReturnStatement>();
        return_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
        return return_statement;
    }
    this->_nextToken(); // [returnFT] -> [Expression]
    auto expr = this->_parseExpression(PrecedenceType::LOWEST);
    if (this->_peekTokenIs(TokenType::Semicolon)) this->_nextToken(); // [Expression] -> [;]
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto return_statement = make_shared<AST::ReturnStatement>(expr);
    return_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return return_statement;
}

shared_ptr<AST::RaiseStatement> Parser::_parseRaiseStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    this->_nextToken();                                                   // [raiseFT] -> [Expression]
    auto expr = this->_parseExpression(PrecedenceType::LOWEST);           // [Expression] remains unchanged
    if (this->_peekTokenIs(TokenType::Semicolon)) { this->_nextToken(); } // [Expression] -> [;]
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto raise_statement = make_shared<AST::RaiseStatement>(expr);
    raise_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return raise_statement;
}

shared_ptr<AST::BlockStatement> Parser::_parseBlockStatement() {
    this->_nextToken(); // [{FT] -> [First Statement]

    std::vector<StatementPtr> statements;
    while (!this->_currentTokenIs(TokenType::RightBrace) && !this->_currentTokenIs(TokenType::EndOfFile)) {
        auto stmt = this->_parseStatement(); // [StatementFT] -> [StatementLT]
        if (stmt) { statements.push_back(stmt); }
        this->_nextToken(); // [StatementLT] -> [Next Statement or }]
    }
    if (this->_peekTokenIs(TokenType::Semicolon)) this->_nextToken(); // [}LT] -> [;]

    auto block_statement = make_shared<AST::BlockStatement>(statements);
    block_statement->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return block_statement;
}

StatementPtr Parser::_parseExpressionStatement(ExpressionPtr identifier, int st_line_no, int st_col_no) {
    if (identifier == nullptr) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
    }
    auto expr = this->_parseExpression(PrecedenceType::LOWEST, identifier, st_line_no, st_col_no);                                                 // [Expression] remains unchanged
    if (this->_peekTokenIs(TokenType::Equals)) return this->_parseVariableAssignment(expr, expr->meta_data.st_col_no, expr->meta_data.end_col_no); // [Variable Assignment]
    this->_expectPeek(TokenType::Semicolon);                                                                                                       // [ExpressionLT] -> [;]
    auto stmt = make_shared<AST::ExpressionStatement>(expr);
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    stmt->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return stmt;
}

StatementPtr Parser::_parseVariableDeclaration(ExpressionPtr identifier, int st_line_no, int st_col_no, bool is_volatile) {
    if (!identifier) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
    }
    this->_expectPeek(TokenType::Colon); // [Identifier] -> [:]
    this->_nextToken();                  // [:] -> [Type]
    auto type = this->_parseType();      // [Type] remains unchanged
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken(); // [Type] -> [;]
        int end_line_no = current_token->line_no;
        int end_col_no = current_token->col_no;
        auto variableDeclarationStatement = make_shared<AST::VariableDeclarationStatement>(identifier, type, nullptr, is_volatile);
        variableDeclarationStatement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
        variableDeclarationStatement->meta_data.more_data["name_line_no"] = st_line_no;
        variableDeclarationStatement->meta_data.more_data["name_col_no"] = st_col_no;
        variableDeclarationStatement->meta_data.more_data["name_end_col_no"] = current_token->end_col_no;
        return variableDeclarationStatement;
    } else if (this->_expectPeek({TokenType::Equals, TokenType::Semicolon})) {
        this->_nextToken();                                         // [Type] -> [=] or [Type] -> [;]
        auto expr = this->_parseExpression(PrecedenceType::LOWEST); // [Expression] remains unchanged
        this->_nextToken();                                         // [Expression] -> [;]
        int end_line_no = current_token->line_no;
        int end_col_no = current_token->col_no;
        auto variableDeclarationStatement = make_shared<AST::VariableDeclarationStatement>(identifier, type, expr, is_volatile);
        variableDeclarationStatement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
        variableDeclarationStatement->meta_data.more_data["name_line_no"] = st_line_no;
        variableDeclarationStatement->meta_data.more_data["name_col_no"] = st_col_no;
        variableDeclarationStatement->meta_data.more_data["name_end_col_no"] = current_token->end_col_no;
        return variableDeclarationStatement;
    }
    return nullptr;
}

shared_ptr<AST::TryCatchStatement> Parser::_parseTryCatchStatement() {
    this->_nextToken();                       // [tryFT] -> [Statement]
    auto try_block = this->_parseStatement(); // [Statement] -> [Next Token]
    std::vector<std::tuple<shared_ptr<AST::Type>, shared_ptr<AST::IdentifierLiteral>, StatementPtr>> catch_blocks;

    while (this->_peekTokenIs(TokenType::Catch)) {
        this->_nextToken();                       // [try] -> [catch]
        this->_expectPeek(TokenType::LeftParen);  // [catch] -> [(]
        this->_nextToken();                       // [(] -> [Exception Type]
        auto exception_type = this->_parseType(); // [Exception Type] remains unchanged
        this->_expectPeek(TokenType::Identifier); // [Exception Type] -> [Identifier]
        auto exception_var = make_shared<AST::IdentifierLiteral>(this->current_token);
        this->_expectPeek(TokenType::RightParen);   // [Identifier] -> [)]
        this->_nextToken();                         // [)] -> [Catch Block Statement]
        auto catch_block = this->_parseStatement(); // [Statement] -> [Next Token]
        catch_blocks.push_back({exception_type, exception_var, catch_block});
    }

    if (catch_blocks.empty()) {
        this->_peekTokenError(this->current_token->type, {TokenType::Catch});
        return nullptr;
    }

    return make_shared<AST::TryCatchStatement>(try_block, catch_blocks);
}

ExpressionPtr Parser::_parseInfixIdenifier() {
    if (this->current_token->type != TokenType::Identifier) {
        std::cerr << "Cannot parse infixIdentifier Expression. Token: " << token::tokenTypeString(this->current_token->type) << std::endl;
        exit(1);
    }
    if (!this->_peekTokenIs(TokenType::Dot)) { return make_shared<AST::IdentifierLiteral>(this->current_token); }
    auto li = make_shared<AST::IdentifierLiteral>(this->current_token);
    this->_nextToken();                                                                              // [Identifier] -> [.]
    this->_nextToken();                                                                              // [.] -> [Next Identifier]
    return make_shared<AST::InfixExpression>(li, TokenType::Dot, ".", this->_parseInfixIdenifier()); // [Next Identifier] remains unchanged
}

shared_ptr<AST::Type> Parser::_parseType() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    ExpressionPtr name;
    name = this->_parseInfixIdenifier(); // [Type] -> [Infix Identifier]
    std::vector<shared_ptr<AST::Type>> generics;
    bool ref = false;
    if (this->_peekTokenIs(TokenType::LeftBracket)) {
        this->_nextToken(); // [Type] -> [[
        this->_nextToken(); // [[ -> [Generic Type]
        while (this->current_token->type != TokenType::RightBracket) {
            auto generic = this->_parseType(); // [Generic Type] remains unchanged
            generics.push_back(generic);
            this->_nextToken(); // [Generic Type] -> [,] or []]
            if (this->current_token->type == TokenType::Comma) { this->_nextToken(); }
        }
    }
    if (this->_peekTokenIs(TokenType::Refrence)) {
        this->_nextToken(); // [Type] -> [&]
        ref = true;
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto generic_type_node = make_shared<AST::Type>(name, generics, ref);
    generic_type_node->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return generic_type_node;
}

StatementPtr Parser::_parseVariableAssignment(ExpressionPtr identifier, int st_line_no, int st_col_no) {
    if (!identifier) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
    }
    this->_expectPeek(TokenType::Equals);                       // [Identifier] -> [=]
    this->_nextToken();                                         // [=] -> [Expression]
    auto expr = this->_parseExpression(PrecedenceType::LOWEST); // [Expression] remains unchanged
    this->_nextToken();                                         // [Expression] -> [;]
    auto stmt = make_shared<AST::VariableAssignmentStatement>(identifier, expr);
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    stmt->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return stmt;
}

shared_ptr<AST::StructStatement> Parser::_parseStructStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;

    this->_expectPeek(TokenType::Identifier); // [structFT] -> [Identifier]
    ExpressionPtr name = make_shared<AST::IdentifierLiteral>(this->current_token);

    this->_expectPeek(TokenType::LeftBrace); // [Identifier] -> [{]
    this->_nextToken();                      // [{] -> [Struct Body]
    std::vector<StatementPtr> statements;

    while (!this->_currentTokenIs(TokenType::RightBrace) && !this->_currentTokenIs(TokenType::EndOfFile)) {
        if (this->_currentTokenIs(TokenType::Def)) {
            auto stmt = this->_parseFunctionStatement(); // [defFT] -> [Function Statement]
            if (stmt) statements.push_back(stmt);
        } else if (this->_currentTokenIs(TokenType::Identifier)) {
            auto stmt = this->_parseVariableDeclaration(); // [Identifier] -> [Variable Declaration]
            if (stmt) statements.push_back(stmt);
        } else if (this->_currentTokenIs(TokenType::AtTheRate)) {
            auto stmt = this->_parseDeco(); // [@FT] -> [Decorator Statement]
            if (stmt) statements.push_back(stmt);
        } else {
            this->_currentTokenError(current_token->type, {TokenType::Identifier, TokenType::Def, TokenType::AtTheRate});
            return nullptr;
        }
        this->_nextToken(); // [StatementLT] -> [Next Statement or }]
    }
    if (this->peek_token->type == TokenType::Semicolon) { this->_nextToken(); } // [}] -> [;]
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;

    auto struct_stmt = make_shared<AST::StructStatement>(name, statements);
    struct_stmt->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return struct_stmt;
}

ExpressionPtr Parser::_parseExpression(PrecedenceType precedence, ExpressionPtr parsed_expression, int st_line_no, int st_col_no) {
    if (parsed_expression == nullptr) {
        st_line_no = current_token->line_no;
        st_col_no = current_token->col_no;
        auto iter = prefix_parse_fns.find(current_token->type);
        if (iter == prefix_parse_fns.end()) {
            this->_noPrefixParseFnError(current_token->type);
            return nullptr;
        }
        auto prefix_fn = iter->second;
        parsed_expression = prefix_fn(); // [Prefix Expression] -> [Parsed Expression]
    }
    while (!_peekTokenIs(TokenType::Semicolon) && precedence < _peekPrecedence()) {
        auto iter = infix_parse_Fns.find(peek_token->type);
        if (iter == infix_parse_Fns.end()) { return parsed_expression; }
        this->_nextToken(); // [Current Expression] -> [Infix Operator]
        auto infix_fn = iter->second;
        parsed_expression = infix_fn(parsed_expression); // [Infix Operator] -> [Infix Expression]
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    parsed_expression->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return parsed_expression;
}

StatementPtr Parser::_parseIfElseStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no = current_token->col_no;
    this->_expectPeek(TokenType::LeftParen); // [ifFT] -> [(]
    this->_nextToken();                      // [(] -> [Condition]
    auto condition = this->_parseExpression(PrecedenceType::LOWEST);
    this->_expectPeek(TokenType::RightParen);   // [Condition] -> [)]
    this->_nextToken();                         // [)] -> [Consequence]
    auto consequence = this->_parseStatement(); // [Consequence] -> [Next Token]
    StatementPtr alternative = nullptr;
    if (this->_peekTokenIs(TokenType::Else)) {
        this->_nextToken();                    // [Consequence] -> [else]
        this->_nextToken();                    // [else] -> [Alternative]
        alternative = this->_parseStatement(); // [Alternative] -> [Next Token]
    }
    int end_line_no = current_token->line_no;
    int end_col_no = current_token->col_no;
    auto if_else_statement = make_shared<AST::IfElseStatement>(condition, consequence, alternative);
    if_else_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return if_else_statement;
}

ExpressionPtr Parser::_parseInfixExpression(ExpressionPtr leftNode) {
    int st_line_no = leftNode->meta_data.st_line_no;
    int st_col_no = leftNode->meta_data.st_col_no;
    auto infix_expr = make_shared<AST::InfixExpression>(leftNode, this->current_token->type, this->current_token->literal);
    infix_expr->meta_data.more_data["operator_line_no"] = this->current_token->line_no;
    infix_expr->meta_data.more_data["operator_st_col_no"] = this->current_token->col_no;
    infix_expr->meta_data.more_data["operator_end_col_no"] = this->current_token->end_col_no;
    auto precedence = this->_currentPrecedence();
    this->_nextToken();                                     // [Operator] -> [Next Expression]
    infix_expr->right = this->_parseExpression(precedence); // [Next Expression] remains unchanged
    int end_line_no = infix_expr->right->meta_data.end_line_no;
    int end_col_no = infix_expr->right->meta_data.end_col_no;
    infix_expr->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return infix_expr;
}

ExpressionPtr Parser::_parseIndexExpression(ExpressionPtr leftNode) {
    int st_line_no = leftNode->meta_data.st_line_no;
    int st_col_no = leftNode->meta_data.st_col_no;
    auto index_expr = make_shared<AST::IndexExpression>(leftNode);
    index_expr->meta_data.more_data["index_line_no"] = this->current_token->line_no;
    index_expr->meta_data.more_data["index_st_col_no"] = this->current_token->col_no;
    index_expr->meta_data.more_data["index_end_col_no"] = this->current_token->end_col_no;
    this->_nextToken();                                                // [LeftBracket] -> [Index Expression]
    index_expr->index = this->_parseExpression(PrecedenceType::INDEX); // [Index Expression] remains unchanged
    int end_line_no = index_expr->index->meta_data.end_line_no;
    int end_col_no = index_expr->index->meta_data.end_col_no;
    index_expr->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    this->_expectPeek(TokenType::RightBracket); // [Index Expression] -> [RightBracket]
    return index_expr;
}

ExpressionPtr Parser::_parseGroupedExpression() {
    this->_nextToken(); // [LeftParen] -> [Grouped Expression]
    int st_line_no = this->current_token->line_no;
    int st_col_no = this->current_token->col_no;
    auto expr = this->_parseExpression(PrecedenceType::LOWEST); // [Grouped ExpressionFT] -> [Grouped ExpressionLT]
    this->_expectPeek(TokenType::RightParen);                   // [Grouped Expression] -> [RightParen]
    int end_line_no = this->current_token->line_no;
    int end_col_no = this->current_token->end_col_no;
    expr->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return expr;
}

ExpressionPtr Parser::_parseIntegerLiteral() {
    auto expr = make_shared<AST::IntegerLiteral>(std::stoll(current_token->literal));
    expr->meta_data.st_line_no = current_token->line_no;
    expr->meta_data.st_col_no = current_token->col_no;
    expr->meta_data.end_line_no = current_token->line_no;
    expr->meta_data.end_col_no = current_token->end_col_no;
    return expr;
}

ExpressionPtr Parser::_parseFloatLiteral() {
    auto expr = make_shared<AST::FloatLiteral>(std::stod(current_token->literal));
    expr->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return expr;
}

ExpressionPtr Parser::_parseBooleanLiteral() {
    auto expr = make_shared<AST::BooleanLiteral>(current_token->type == TokenType::True);
    expr->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return expr;
}

ExpressionPtr Parser::_parseNew() {
    this->_expectPeek({TokenType::Identifier, TokenType::LeftBracket}); // [newFT] -> [Identifier | LeftBracket]
    if (this->_currentTokenIs(TokenType::Identifier)) is_new_call = true;
    else if (this->_currentTokenIs(TokenType::LeftBracket)) is_new_arr = true;
    else { _currentTokenError(current_token->type, {TokenType::Identifier, TokenType::LeftBracket}, "Expected an identifier or a left bracket after 'new'."); }
    return this->_parseExpression(PrecedenceType::LOWEST); // [new] -> [Expression]
}

ExpressionPtr Parser::_parseStringLiteral() {
    auto expr = make_shared<AST::StringLiteral>(current_token->literal);
    expr->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return expr;
}

void Parser::_nextToken() {
    current_token = peek_token;
    peek_token = lexer->nextToken();
}

bool Parser::_currentTokenIs(TokenType type) {
    return current_token->type == type;
}

bool Parser::_peekTokenIs(TokenType type) {
    return peek_token->type == type;
}

bool Parser::_expectPeek(std::vector<TokenType> types, std::string suggestedFix) {
    for (auto type : types) {
        if (_peekTokenIs(type)) {
            _nextToken();
            return true;
        }
    }
    _peekTokenError(peek_token->type, types, suggestedFix);
}

bool Parser::_expectPeek(TokenType type, std::string suggestedFix) {
    if (_peekTokenIs(type)) {
        _nextToken();
        return true;
    }
    _peekTokenError(peek_token->type, {type}, suggestedFix);
}

PrecedenceType Parser::_currentPrecedence() {
    auto it = token_precedence.find(current_token->type);
    if (it != token_precedence.end()) {
        return it->second;
    } else {
        return PrecedenceType::LOWEST;
    }
}

PrecedenceType Parser::_peekPrecedence() {
    auto it = token_precedence.find(peek_token->type);
    if (it != token_precedence.end()) {
        return it->second;
    } else {
        return PrecedenceType::LOWEST;
    }
}

ExpressionPtr Parser::_parseArrayLiteral() {
    auto elements = std::vector<ExpressionPtr>();
    for (_nextToken(); !_currentTokenIs(TokenType::RightBracket); _nextToken()) { // [LeftBracket] -> [Element]
        if (_currentTokenIs(TokenType::Comma)) { continue; }                      // Skip commas
        auto expr = _parseExpression(PrecedenceType::LOWEST);                     // [Element] remains unchanged
        if (expr) { elements.push_back(expr); }
    }
    auto array = make_shared<AST::ArrayLiteral>(elements, is_new_arr);
    is_new_arr = false;
    array->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return array;
}

ExpressionPtr Parser::_parseIdentifier() {
    if (this->current_token->type != TokenType::Identifier) {
        std::cerr << this->current_token->literal + " is not an Identifier." << std::endl;
        exit(1);
    }
    auto identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
    identifier->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return identifier;
}

void Parser::_peekTokenError(TokenType type, std::vector<TokenType> expected_types, std::string suggestedFix) {
    std::string expected_types_str;
    for (const auto& expected_type : expected_types) {
        if (!expected_types_str.empty()) { expected_types_str += ", "; }
        expected_types_str += token::tokenTypeString(expected_type);
    }
    errors::SyntaxError("SyntaxError", this->lexer->source, *peek_token, "Expected one of: " + expected_types_str + " but got " + token::tokenTypeString(type), suggestedFix).raise();
}

void Parser::_currentTokenError(TokenType type, std::vector<TokenType> expected_types, std::string suggestedFix) {
    std::string expected_types_str;
    for (const auto& expected_type : expected_types) {
        if (!expected_types_str.empty()) { expected_types_str += ", "; }
        expected_types_str += token::tokenTypeString(expected_type);
    }
    errors::SyntaxError("SyntaxError", this->lexer->source, *this->current_token, "Expected one of: " + expected_types_str + " but got " + token::tokenTypeString(type), suggestedFix).raise();
}

void Parser::_noPrefixParseFnError(TokenType type) {
    errors::NoPrefixParseFnError(this->lexer->source, *peek_token, "No prefix parse function for " + token::tokenTypeString(type)).raise();
}
