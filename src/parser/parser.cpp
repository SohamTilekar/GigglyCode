#include <cstdlib>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include "../errors/errors.hpp"
#include "AST/ast.hpp"
#include "parser.hpp"

using namespace parser;
#ifdef LOG
#define LOG_MSG(msg) \
    Logger::getInstance().log(__FILE__, __FUNCTION__, __LINE__, msg);
#define LOG_TOK() \
    Logger::getInstance().log(__FILE__, __FUNCTION__, __LINE__, this->current_token.toString(false));
#else
#define LOG_MSG(msg)
#define LOG_TOK()
#endif

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

Parser::Parser(Lexer* lexer) : lexer(lexer) {
    // Initialize with the first two tokens
    this->_nextToken(); // [FT]
    LOG_MSG("First token read.");
    this->_nextToken(); // [LT]
    LOG_MSG("Peek token read.");
}

AST::Program* Parser::parseProgram() {
    auto program = new AST::Program;
    int startLineNo = current_token.line_no;
    int startColNo = current_token.col_no;

    LOG_MSG("Starting to parse program.");

    // Parse statements until EndOfFile token is reached
    while (current_token.type != TokenType::EndOfFile) {
        auto statement = this->_parseStatement(); // [stmtFT] -> [stmtLT]
        if (statement) {
            program->statements.push_back(statement);
            LOG_MSG("Parsed a statement.");
        }
        this->_nextToken(); // [stmtLT] -> [stmtFT | EOF]
        LOG_TOK()
    } // [EOF]

    int endLineNo = current_token.line_no;
    int endColNo = current_token.col_no;
    program->set_meta_data(startLineNo, startColNo, endLineNo, endColNo);

    LOG_MSG("Finished parsing program.");

    return program;
}

AST::Statement* Parser::_parseStatement() {
    switch (current_token.type) {
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
        case TokenType::Switch:
            return this->_parseSwitchCaseStatement(); // [switchFT] -> [;]
        case TokenType::Volatile:
            this->_nextToken();                                            // [volatileFT] -> [Identifier!]
            LOG_TOK()
            return this->_parseVariableDeclaration(nullptr, -1, -1, true); // [Identifier!FT] -> [;]
        default:
            return this->_parseExpressionStatement(); // [ExpressionFT] -> [;]
    }
}

AST::Statement* Parser::_interpretIdentifier() {
    int stLineNo = current_token.line_no;
    int stColNo = current_token.col_no;
    auto identifier = new AST::IdentifierLiteral(this->current_token);

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
        auto stmt = new AST::ExpressionStatement(functionCall);
        _expectPeek(TokenType::Semicolon); // Expect ';' after function call
        return stmt;
    }

    // Default to expression statement
    return _parseExpressionStatement(identifier, stLineNo, stColNo);
}

AST::Statement* Parser::_parseDeco() {
    this->_expectPeek(TokenType::Identifier); // [@FT] -> [Identifier]
    std::string name = this->current_token.literal;

    if (name == "generic") {
        return this->_parseGenericDeco(); // [IdentifierFT] -> [)]
    } else if (name == "autocast") {
        return this->_parseAutocastDeco(); // [IdentifierFT] -> [)]
    }
    errors::SyntaxError("WrongDecoName", this->lexer->source, this->current_token, "Unknown Deco type: " + name, "Check the deco name for case sensitivity. Valid options: `autocast` or `generic`.")
        .raise();
}

AST::Statement* Parser::_parseGenericDeco() {
    this->_expectPeek(TokenType::LeftParen); // [Identifier] -> [(]
    this->_nextToken();                      // [(] -> [Identifier | )]!
    LOG_TOK()
    std::vector<AST::GenericType*> generics;

    while (this->current_token.type != TokenType::RightParen) {
        if (this->_currentTokenIs(TokenType::Identifier)) {
            auto identifier = new AST::IdentifierLiteral(this->current_token);
            this->_expectPeek(TokenType::Colon); // [Identifier] -> [:]
            this->_nextToken();                  // [:] -> [Type]
            LOG_TOK()
            std::vector<AST::Type*> type;

            while (this->current_token.type != TokenType::RightParen && this->current_token.type != TokenType::Comma) {
                type.push_back(this->_parseType()); // [Type] remains unchanged
                if (this->_peekTokenIs(TokenType::Pipe)) {
                    this->_nextToken(); // [Type] -> [|]
                    LOG_TOK()
                    this->_nextToken(); // [|] -> [Next Type]
                    LOG_TOK()
                } else break;
            }

            generics.push_back(new AST::GenericType(identifier, type));
            if (this->_peekTokenIs(TokenType::Comma)) {
                this->_nextToken(); // [Type] -> [,]
                LOG_TOK()
                this->_nextToken(); // [,] -> [Next Identifier]
                LOG_TOK()
            } else break;
        } else {
            _currentTokenError(current_token.type, {TokenType::Identifier});
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
            auto deco = _deco->castToFunctionStatement();
            deco->generic = generics;
            return deco;
        } else if (_deco->type() == AST::NodeType::StructStatement) {
            auto deco = _deco->castToStructStatement();
            deco->generics = generics;
            return deco;
        }
    }

    this->_currentTokenError(this->current_token.type, {TokenType::Def, TokenType::AtTheRate});
}

AST::Statement* Parser::_parseAutocastDeco() {
    this->_expectPeek({TokenType::Def, TokenType::Struct, TokenType::AtTheRate}); // [autocast] -> [Def | Struct | @]

    if (this->_currentTokenIs(TokenType::Def)) {
        auto func = this->_parseFunctionStatement(); // [Def] -> [Function Statement]
        func->extra_info["autocast"] = true;
        return func;
    } else if (this->_currentTokenIs(TokenType::AtTheRate)) {
        auto _deco = this->_parseDeco(); // [@] -> [Decorator Statement]
        if (_deco->type() == AST::NodeType::FunctionStatement) {
            auto deco = _deco->castToFunctionStatement();
            deco->extra_info["autocast"] = true;
            return deco;
        }
    }

    this->_currentTokenError(this->current_token.type, {TokenType::Def, TokenType::AtTheRate});
}

std::vector<AST::FunctionParameter*> Parser::_parseFunctionParameters() {
    std::vector<AST::FunctionParameter*> parameters;
    while (this->current_token.type != TokenType::RightParen) {
        if (this->current_token.type == TokenType::Identifier) {
            auto identifier = new AST::IdentifierLiteral(this->current_token);
            this->_expectPeek(TokenType::Colon); // Expect ':' after parameter name
            this->_nextToken();                  // Move to parameter type
            LOG_TOK()
            auto type = this->_parseType();
            parameters.push_back(new AST::FunctionParameter(identifier, type));
            this->_expectPeek({TokenType::Comma, TokenType::RightParen});
            if (this->_currentTokenIs(TokenType::Comma)) {
                this->_nextToken(); // Consume ',' and continue
                LOG_TOK()
                continue;
            } else if (this->_currentTokenIs(TokenType::RightParen)) {
                break;
            }
        } else {
            _currentTokenError(current_token.type, {TokenType::Identifier});
            break;
        }
    }
    return parameters;
}

std::vector<AST::FunctionParameter*> Parser::_parseClosureParameters() {
    std::vector<AST::FunctionParameter*> closure_parameters;
    while (!this->_currentTokenIs(TokenType::RightParen)) {
        if (this->_currentTokenIs(TokenType::Identifier)) {
            auto identifier = new AST::IdentifierLiteral(this->current_token);
            this->_expectPeek(TokenType::Colon); // Expect ':' after closure parameter name
            this->_nextToken();                  // Move to closure parameter type
            LOG_TOK()
            auto type = this->_parseType();
            closure_parameters.push_back(new AST::FunctionParameter(identifier, type));
            this->_nextToken(); // Consume ',' or ')'
            LOG_TOK()
            if (this->_currentTokenIs(TokenType::Comma)) {
                this->_nextToken(); // Consume ',' and continue
                LOG_TOK()
                continue;
            } else if (this->_currentTokenIs(TokenType::RightParen)) {
                break;
            } else {
                _currentTokenError(current_token.type, {TokenType::Comma, TokenType::RightParen});
                break;
            }
        } else {
            _currentTokenError(current_token.type, {TokenType::Identifier});
            break;
        }
    }
    return closure_parameters;
}

AST::FunctionStatement* Parser::_parseFunctionStatement() {
    int st_line_no = current_token.line_no;
    int st_col_no = current_token.col_no;

    this->_expectPeek(TokenType::Identifier); // Expect function name
    auto name = new AST::IdentifierLiteral(this->current_token);
    name->set_meta_data(current_token.line_no, current_token.col_no, current_token.line_no, current_token.end_col_no);

    this->_expectPeek(TokenType::LeftParen); // Expect '(' after function name
    this->_nextToken();                      // Move to parameters
    LOG_TOK()

    auto parameters = _parseFunctionParameters();

    // Handle closure parameters if present
    std::vector<AST::FunctionParameter*> closure_parameters;
    if (this->_peekTokenIs(TokenType::Use)) {
        this->_nextToken(); // Consume 'use' keyword
        LOG_TOK()
        this->_nextToken(); // Consume '('
        LOG_TOK()
        this->_nextToken(); // Move to closure parameters
        LOG_TOK()
        closure_parameters = _parseClosureParameters();
    }

    // Handle return type if present
    AST::Type* return_type = nullptr;
    if (this->_peekTokenIs(TokenType::RightArrow)) {
        this->_nextToken(); // Consume '->'
        LOG_TOK()
        this->_nextToken(); // Move to return type
        LOG_TOK()
        return_type = this->_parseType();
    }

    // Parse function body or expect ';'
    AST::BlockStatement* body = nullptr;
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken(); // Consume ';' indicating no body
        LOG_TOK()
    } else {
        this->_expectPeek(TokenType::LeftBrace); // Expect '{' to begin function body
        body = this->_parseBlockStatement();
    }

    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;

    auto function_statement = new AST::FunctionStatement(name, parameters, closure_parameters, return_type, body, std::vector<AST::GenericType*>{});

    function_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return function_statement;
}

AST::WhileStatement* Parser::_parseWhileStatement() {
    int st_line_no = current_token.line_no;
    int st_col_no = current_token.col_no;

    _expectPeek(TokenType::LeftParen); // Expect '(' after 'while'
    _nextToken();                      // Move to condition expression
    LOG_TOK()

    auto condition = _parseExpression(PrecedenceType::LOWEST);

    _expectPeek(TokenType::RightParen); // Expect ')' after condition
    _nextToken();                       // Move to loop body
    LOG_TOK()

    auto body = _parseStatement(); // Parse the loop body

    LoopModifiers modifiers = _parseLoopModifiers(); // Parse any loop modifiers

    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;

    auto while_statement = new AST::WhileStatement(condition, body, modifiers.ifbreak, modifiers.notbreak);

    while_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return while_statement;
}

AST::ForStatement* Parser::_parseForStatement() {
    int st_line_no = current_token.line_no;
    int st_col_no = current_token.col_no;

    _expectPeek(TokenType::LeftParen);  // Expect '(' after 'for'
    _expectPeek(TokenType::Identifier); // Expect identifier in 'for (identifier in ...)

    auto get = new AST::IdentifierLiteral(current_token);

    _expectPeek(TokenType::In); // Expect 'in' keyword
    _nextToken();               // Move to the 'from' expression
    LOG_TOK()

    auto from = _parseExpression(PrecedenceType::LOWEST); // Parse 'from' expression

    _expectPeek(TokenType::RightParen); // Expect ')' after 'in' expression
    _nextToken();                       // Move to loop body
    LOG_TOK()

    auto body = _parseStatement(); // Parse the loop body

    LoopModifiers modifiers = _parseLoopModifiers(); // Parse any loop modifiers

    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;

    auto for_statement = new AST::ForStatement(get, from, body, modifiers.ifbreak, modifiers.notbreak);

    for_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return for_statement;
}

parser::Parser::LoopModifiers parser::Parser::_parseLoopModifiers() {
    LoopModifiers modifiers = {nullptr, nullptr};
    while (_peekTokenIs(TokenType::NotBreak) || _peekTokenIs(TokenType::IfBreak)) {
        if (_peekTokenIs(TokenType::NotBreak)) {
            _nextToken(); // Consume 'notbreak' token
            LOG_TOK()
            _nextToken(); // Move to the statement following 'notbreak'
            LOG_TOK()
            modifiers.notbreak = _parseStatement();
        } else if (_peekTokenIs(TokenType::IfBreak)) {
            _nextToken(); // Consume 'ifbreak' token
            LOG_TOK()
            _nextToken(); // Move to the statement following 'ifbreak'
            LOG_TOK()
            modifiers.ifbreak = _parseStatement();
        }
    }
    return modifiers;
}

AST::BreakStatement* Parser::_parseBreakStatement() {
    int st_line_no = current_token.line_no;
    int st_col_no = current_token.col_no;
    this->_nextToken(); // [breakFT] -> [Optional Loop Number]
    LOG_TOK()
    int loopNum = 0;
    if (this->_currentTokenIs(TokenType::Integer)) {
        loopNum = std::stoi(current_token.literal); // [IntegerFT] -> [Next Token]
        this->_nextToken();                          // [Next Token] remains unchanged
        LOG_TOK()
    }
    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;
    auto break_statement = new AST::BreakStatement(loopNum);
    break_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return break_statement;
}

AST::ContinueStatement* Parser::_parseContinueStatement() {
    int st_line_no = current_token.line_no;
    int st_col_no = current_token.col_no;
    this->_nextToken(); // [continueFT] -> [Optional Loop Number]
    LOG_TOK()
    int loopNum = 0;
    if (this->_currentTokenIs(TokenType::Integer)) {
        loopNum = std::stoi(current_token.literal); // [IntegerFT] -> [Next Token]
        this->_nextToken();                          // [Next Token] remains unchanged
        LOG_TOK()
    }
    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;
    auto continue_statement = new AST::ContinueStatement(loopNum);
    continue_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return continue_statement;
}

AST::ImportStatement* Parser::_parseImportStatement() {
    int st_line_no = current_token.line_no;
    int st_col_no = current_token.col_no;
    this->_expectPeek(TokenType::String); // [importFT] -> [String]
    auto import_statement = new AST::ImportStatement(this->current_token.literal);
    this->_expectPeek(TokenType::Semicolon); // [String] -> [;]
    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;
    import_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return import_statement;
}

AST::Expression* Parser::_parseFunctionCall(AST::Expression* identifier, int st_line_no, int st_col_no) {
    if (!identifier) {
        st_line_no = current_token.line_no;
        st_col_no = current_token.col_no;
        identifier = new AST::IdentifierLiteral(this->current_token);
    }
    identifier->set_meta_data(st_line_no, st_col_no, current_token.line_no, current_token.end_col_no);
    this->_nextToken(); // [Identifier] -> [(] or [LeftParen]
    LOG_TOK()
    bool is_new_call_local = is_new_call;
    is_new_call = false;
    auto args = this->_parse_expression_list(TokenType::RightParen); // [(] -> [Arguments] -> [)]
    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;
    auto call_expression = new AST::CallExpression(identifier, args);
    call_expression->_new = is_new_call_local;
    call_expression->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return call_expression;
}

std::vector<AST::Expression*> Parser::_parse_expression_list(TokenType end) {
    std::vector<AST::Expression*> args;
    if (this->_peekTokenIs(end)) {
        this->_nextToken(); // [LeftParen] -> [end]
        LOG_TOK()
        return args;
    }
    this->_nextToken(); // [LeftParen] -> [First Argument]
    LOG_TOK()
    args.push_back(this->_parseExpression(PrecedenceType::LOWEST));
    while (this->_peekTokenIs(TokenType::Comma)) {
        this->_nextToken(); // [Argument] -> [,]
        LOG_TOK()
        this->_nextToken(); // [,] -> [Next Argument]
        LOG_TOK()
        args.push_back(this->_parseExpression(PrecedenceType::LOWEST));
    }
    this->_expectPeek(end); // [Last Argument] -> [end]
    return args;
}

AST::ReturnStatement* Parser::_parseReturnStatement() {
    int st_line_no = current_token.line_no;
    int st_col_no = current_token.col_no;
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken(); // [returnFT] -> [;]
        LOG_TOK()
        int end_line_no = current_token.line_no;
        int end_col_no = current_token.col_no;
        auto return_statement = new AST::ReturnStatement();
        return_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
        return return_statement;
    }
    this->_nextToken(); // [returnFT] -> [Expression]
    LOG_TOK()
    auto expr = this->_parseExpression(PrecedenceType::LOWEST);
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken(); // [Expression] -> [;]
        LOG_TOK()
    }
    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;
    auto return_statement = new AST::ReturnStatement(expr);
    return_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return return_statement;
}

AST::RaiseStatement* Parser::_parseRaiseStatement() {
    int st_line_no = current_token.line_no;
    int st_col_no = current_token.col_no;
    this->_nextToken();                                                   // [raiseFT] -> [Expression]
    LOG_TOK()
    auto expr = this->_parseExpression(PrecedenceType::LOWEST);           // [Expression] remains unchanged
    if (this->_peekTokenIs(TokenType::Semicolon)) { this->_nextToken();
        LOG_TOK()
    } // [Expression] -> [;]
    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;
    auto raise_statement = new AST::RaiseStatement(expr);
    raise_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return raise_statement;
}

AST::BlockStatement* Parser::_parseBlockStatement() {
    this->_nextToken(); // [{FT] -> [First Statement]
    LOG_TOK()

    std::vector<AST::Statement*> statements;
    while (!this->_currentTokenIs(TokenType::RightBrace) && !this->_currentTokenIs(TokenType::EndOfFile)) {
        auto stmt = this->_parseStatement(); // [StatementFT] -> [StatementLT]
        if (stmt) { statements.push_back(stmt); }
        this->_nextToken(); // [StatementLT] -> [Next Statement or }]
        LOG_TOK()
    }
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken();
        LOG_TOK()
    } // [}LT] -> [;]

    auto block_statement = new AST::BlockStatement(statements);
    block_statement->set_meta_data(current_token.line_no, current_token.col_no, current_token.line_no, current_token.end_col_no);
    return block_statement;
}

AST::Statement* Parser::_parseExpressionStatement(AST::Expression* identifier, int st_line_no, int st_col_no) {
    if (identifier == nullptr) {
        st_line_no = current_token.line_no;
        st_col_no = current_token.col_no;
        identifier = new AST::IdentifierLiteral(this->current_token);
    }
    auto expr = this->_parseExpression(PrecedenceType::LOWEST, identifier, st_line_no, st_col_no);                                                 // [Expression] remains unchanged
    if (this->_peekTokenIs(TokenType::Equals)) return this->_parseVariableAssignment(expr, expr->meta_data.st_col_no, expr->meta_data.end_col_no); // [Variable Assignment]
    this->_expectPeek(TokenType::Semicolon);                                                                                                       // [ExpressionLT] -> [;]
    auto stmt = new AST::ExpressionStatement(expr);
    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;
    stmt->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return stmt;
}

AST::Statement* Parser::_parseVariableDeclaration(AST::Expression* identifier, int st_line_no, int st_col_no, bool is_volatile) {
    if (!identifier) {
        st_line_no = current_token.line_no;
        st_col_no = current_token.col_no;
        identifier = new AST::IdentifierLiteral(this->current_token);
    }
    this->_expectPeek(TokenType::Colon); // [Identifier] -> [:]
    this->_nextToken();                  // [:] -> [Type]
    LOG_TOK()
    auto type = this->_parseType();      // [Type] remains unchanged
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken(); // [Type] -> [;]
        LOG_TOK()
        int end_line_no = current_token.line_no;
        int end_col_no = current_token.col_no;
        auto variableDeclarationStatement = new AST::VariableDeclarationStatement(identifier, type, nullptr, is_volatile);
        variableDeclarationStatement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
        variableDeclarationStatement->meta_data.more_data["name_line_no"] = st_line_no;
        variableDeclarationStatement->meta_data.more_data["name_col_no"] = st_col_no;
        variableDeclarationStatement->meta_data.more_data["name_end_col_no"] = current_token.end_col_no;
        return variableDeclarationStatement;
    } else if (this->_expectPeek({TokenType::Equals, TokenType::Semicolon})) {
        this->_nextToken();                                         // [Type] -> [=] or [Type] -> [;]
        LOG_TOK()
        auto expr = this->_parseExpression(PrecedenceType::LOWEST); // [Expression] remains unchanged
        this->_nextToken();                                         // [Expression] -> [;]
        LOG_TOK()
        int end_line_no = current_token.line_no;
        int end_col_no = current_token.col_no;
        auto variableDeclarationStatement = new AST::VariableDeclarationStatement(identifier, type, expr, is_volatile);
        variableDeclarationStatement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
        variableDeclarationStatement->meta_data.more_data["name_line_no"] = st_line_no;
        variableDeclarationStatement->meta_data.more_data["name_col_no"] = st_col_no;
        variableDeclarationStatement->meta_data.more_data["name_end_col_no"] = current_token.end_col_no;
        return variableDeclarationStatement;
    }
    return nullptr;
}

AST::TryCatchStatement* Parser::_parseTryCatchStatement() {
    this->_nextToken();                       // [tryFT] -> [Statement]
    LOG_TOK()
    auto try_block = this->_parseStatement(); // [Statement] -> [Next Token]
    std::vector<std::tuple<AST::Type*, AST::IdentifierLiteral*, AST::Statement*>> catch_blocks;

    while (this->_peekTokenIs(TokenType::Catch)) {
        this->_nextToken();                       // [try] -> [catch]
        LOG_TOK()
        this->_expectPeek(TokenType::LeftParen);  // [catch] -> [(]
        this->_nextToken();                       // [(] -> [Exception Type]
        LOG_TOK()
        auto exception_type = this->_parseType(); // [Exception Type] remains unchanged
        this->_expectPeek(TokenType::Identifier); // [Exception Type] -> [Identifier]
        auto exception_var = new AST::IdentifierLiteral(this->current_token);
        this->_expectPeek(TokenType::RightParen);   // [Identifier] -> [)]
        this->_nextToken();                         // [)] -> [Catch Block Statement]
        LOG_TOK()
        auto catch_block = this->_parseStatement(); // [Statement] -> [Next Token]
        catch_blocks.push_back({exception_type, exception_var, catch_block});
    }

    if (catch_blocks.empty()) {
        this->_peekTokenError(this->current_token.type, {TokenType::Catch});
        return nullptr;
    }

    return new AST::TryCatchStatement(try_block, catch_blocks);
}

AST::SwitchCaseStatement* Parser::_parseSwitchCaseStatement() {
    this->_expectPeek(TokenType::LeftParen); // [switchFT] -> [(]
    LOG_TOK()
    this->_nextToken(); // [(] -> [ExpresionFT]
    LOG_TOK()
    auto condition = this->_parseExpression(PrecedenceType::LOWEST); // [ExpresisonFT] -> [ExpresisonLT]
    this->_nextToken(); // [ExpresisonLT] -> [)]
    this->_expectPeek(TokenType::LeftBrace); // [)] -> [{]
    std::vector<std::tuple<AST::Expression*, AST::Statement*>> case_blocks;

    while (this->_peekTokenIs(TokenType::Case)) {
        this->_nextToken();                       // [try] -> [catch]
        LOG_TOK()
        this->_expectPeek(TokenType::LeftParen);  // [catch] -> [(]
        this->_nextToken();                       // [(] -> [Exception Type]
        LOG_TOK()
        auto _case = this->_parseExpression(PrecedenceType::LOWEST); // [Exception Type] remains unchanged
        this->_expectPeek(TokenType::RightParen);   // [Identifier] -> [)]
        this->_nextToken();                         // [)] -> [Catch Block Statement]
        LOG_TOK()
        auto catch_block = this->_parseStatement(); // [Statement] -> [Next Token]
        case_blocks.push_back({_case, catch_block});
    }
    AST::Statement* other = nullptr;
    if (this->_peekTokenIs(TokenType::Other)) {
        this->_nextToken(); // [Statement] -> [Other]
        LOG_TOK()
        this->_nextToken(); // [Other] -> [StatementFT]
        LOG_TOK()
        other = this->_parseStatement();
        LOG_TOK()
    }
    this->_expectPeek(TokenType::RightBrace);
    LOG_TOK()
    return new AST::SwitchCaseStatement(condition, case_blocks, other);
}

AST::Expression* Parser::_parseInfixIdenifier() {
    if (this->current_token.type != TokenType::Identifier) {
        std::cerr << "Cannot parse infixIdentifier Expression. Token: " << token::tokenTypeString(this->current_token.type) << std::endl;
        exit(1);
    }
    if (!this->_peekTokenIs(TokenType::Dot)) { return new AST::IdentifierLiteral(this->current_token); }
    auto li = new AST::IdentifierLiteral(this->current_token);
    this->_nextToken();                                                                              // [Identifier] -> [.]
    LOG_TOK()
    this->_nextToken();                                                                              // [.] -> [Next Identifier]
    LOG_TOK()
    return new AST::InfixExpression(li, TokenType::Dot, ".", this->_parseInfixIdenifier()); // [Next Identifier] remains unchanged
}

AST::Type* Parser::_parseType() {
    int st_line_no = current_token.line_no;
    int st_col_no = current_token.col_no;
    AST::Expression* name;
    name = this->_parseInfixIdenifier(); // [Type] -> [Infix Identifier]
    std::vector<AST::Type*> generics;
    bool ref = false;
    if (this->_peekTokenIs(TokenType::LeftBracket)) {
        this->_nextToken(); // [Type] -> [[
        LOG_TOK()
        this->_nextToken(); // [[ -> [Generic Type]
        LOG_TOK()
        while (this->current_token.type != TokenType::RightBracket) {
            auto generic = this->_parseType(); // [Generic Type] remains unchanged
            generics.push_back(generic);
            this->_nextToken(); // [Generic Type] -> [,] or []]
            LOG_TOK()
            if (this->current_token.type == TokenType::Comma) { this->_nextToken();
                LOG_TOK()
            }
        }
    }
    if (this->_peekTokenIs(TokenType::Refrence)) {
        this->_nextToken(); // [Type] -> [&]
        LOG_TOK()
        ref = true;
    }
    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;
    auto generic_type_node = new AST::Type(name, generics, ref);
    generic_type_node->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return generic_type_node;
}

AST::Statement* Parser::_parseVariableAssignment(AST::Expression* identifier, int st_line_no, int st_col_no) {
    if (!identifier) {
        st_line_no = current_token.line_no;
        st_col_no = current_token.col_no;
        identifier = new AST::IdentifierLiteral(this->current_token);
    }
    this->_expectPeek(TokenType::Equals);                       // [Identifier] -> [=]
    this->_nextToken();                                         // [=] -> [Expression]
    LOG_TOK()
    auto expr = this->_parseExpression(PrecedenceType::LOWEST); // [Expression] remains unchanged
    this->_nextToken();                                         // [Expression] -> [;]
    LOG_TOK()
    auto stmt = new AST::VariableAssignmentStatement(identifier, expr);
    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;
    stmt->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return stmt;
}

AST::StructStatement* Parser::_parseStructStatement() {
    int st_line_no = current_token.line_no;
    int st_col_no = current_token.col_no;

    this->_expectPeek(TokenType::Identifier); // [structFT] -> [Identifier]
    AST::Expression* name = new AST::IdentifierLiteral(this->current_token);

    this->_expectPeek(TokenType::LeftBrace); // [Identifier] -> [{]
    this->_nextToken();                      // [{] -> [Struct Body]
    LOG_TOK()
    std::vector<AST::Statement*> statements;

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
            this->_currentTokenError(current_token.type, {TokenType::Identifier, TokenType::Def, TokenType::AtTheRate});
            return nullptr;
        }
        this->_nextToken(); // [StatementLT] -> [Next Statement or }]
        LOG_TOK()
    }
    if (this->peek_token.type == TokenType::Semicolon) { this->_nextToken();
        LOG_TOK()
    } // [}] -> [;]
    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;

    auto struct_stmt = new AST::StructStatement(name, statements);
    struct_stmt->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return struct_stmt;
}

AST::Expression* Parser::_parseExpression(PrecedenceType precedence, AST::Expression* parsed_expression, int st_line_no, int st_col_no) {
    if (parsed_expression == nullptr) {
        st_line_no = current_token.line_no;
        st_col_no = current_token.col_no;
        auto iter = prefix_parse_fns.find(current_token.type);
        if (iter == prefix_parse_fns.end()) {
            this->_noPrefixParseFnError(current_token.type);
            return nullptr;
        }
        auto prefix_fn = iter->second;
        parsed_expression = prefix_fn(); // [Prefix Expression] -> [Parsed Expression]
    }
    while (!_peekTokenIs(TokenType::Semicolon) && precedence < _peekPrecedence()) {
        auto iter = infix_parse_fns.find(peek_token.type);
        if (iter == infix_parse_fns.end()) { return parsed_expression; }
        this->_nextToken(); // [Current Expression] -> [Infix Operator]
        LOG_TOK()
        auto infix_fn = iter->second;
        parsed_expression = infix_fn(parsed_expression); // [Infix Operator] -> [Infix Expression]
    }
    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;
    parsed_expression->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return parsed_expression;
}

AST::Statement* Parser::_parseIfElseStatement() {
    int st_line_no = current_token.line_no;
    int st_col_no = current_token.col_no;
    this->_expectPeek(TokenType::LeftParen); // [ifFT] -> [(]
    this->_nextToken();                      // [(] -> [Condition]
    LOG_TOK()
    auto condition = this->_parseExpression(PrecedenceType::LOWEST);
    this->_expectPeek(TokenType::RightParen);   // [Condition] -> [)]
    this->_nextToken();                         // [)] -> [Consequence]
    LOG_TOK()
    auto consequence = this->_parseStatement(); // [Consequence] -> [Next Token]
    AST::Statement* alternative = nullptr;
    if (this->_peekTokenIs(TokenType::Else)) {
        this->_nextToken();                    // [Consequence] -> [else]
        LOG_TOK()
        this->_nextToken();                    // [else] -> [Alternative]
        LOG_TOK()
        alternative = this->_parseStatement(); // [Alternative] -> [Next Token]
    }
    int end_line_no = current_token.line_no;
    int end_col_no = current_token.col_no;
    auto if_else_statement = new AST::IfElseStatement(condition, consequence, alternative);
    if_else_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return if_else_statement;
}

AST::Expression* Parser::_parseInfixExpression(AST::Expression* leftNode) {
    int st_line_no = leftNode->meta_data.st_line_no;
    int st_col_no = leftNode->meta_data.st_col_no;
    auto infix_expr = new AST::InfixExpression(leftNode, this->current_token.type, this->current_token.literal);
    infix_expr->meta_data.more_data["operator_line_no"] = this->current_token.line_no;
    infix_expr->meta_data.more_data["operator_st_col_no"] = this->current_token.col_no;
    infix_expr->meta_data.more_data["operator_end_col_no"] = this->current_token.end_col_no;
    auto precedence = this->_currentPrecedence();
    this->_nextToken();                                     // [Operator] -> [Next Expression]
    LOG_TOK()
    infix_expr->right = this->_parseExpression(precedence); // [Next Expression] remains unchanged
    int end_line_no = infix_expr->right->meta_data.end_line_no;
    int end_col_no = infix_expr->right->meta_data.end_col_no;
    infix_expr->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return infix_expr;
}

AST::Expression* Parser::_parseIndexExpression(AST::Expression* leftNode) {
    int st_line_no = leftNode->meta_data.st_line_no;
    int st_col_no = leftNode->meta_data.st_col_no;
    auto index_expr = new AST::IndexExpression(leftNode);
    index_expr->meta_data.more_data["index_line_no"] = this->current_token.line_no;
    index_expr->meta_data.more_data["index_st_col_no"] = this->current_token.col_no;
    index_expr->meta_data.more_data["index_end_col_no"] = this->current_token.end_col_no;
    this->_nextToken();                                                // [LeftBracket] -> [Index Expression]
    LOG_TOK()
    index_expr->index = this->_parseExpression(PrecedenceType::INDEX); // [Index Expression] remains unchanged
    int end_line_no = index_expr->index->meta_data.end_line_no;
    int end_col_no = index_expr->index->meta_data.end_col_no;
    index_expr->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    this->_expectPeek(TokenType::RightBracket); // [Index Expression] -> [RightBracket]
    return index_expr;
}

AST::Expression* Parser::_parseGroupedExpression() {
    this->_nextToken(); // [LeftParen] -> [Grouped Expression]
    LOG_TOK()
    int st_line_no = this->current_token.line_no;
    int st_col_no = this->current_token.col_no;
    auto expr = this->_parseExpression(PrecedenceType::LOWEST); // [Grouped ExpressionFT] -> [Grouped ExpressionLT]
    this->_expectPeek(TokenType::RightParen);                   // [Grouped Expression] -> [RightParen]
    int end_line_no = this->current_token.line_no;
    int end_col_no = this->current_token.end_col_no;
    expr->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return expr;
}

AST::Expression* Parser::_parseIntegerLiteral() {
    auto expr = new AST::IntegerLiteral(std::stoll(current_token.literal));
    expr->meta_data.st_line_no = current_token.line_no;
    expr->meta_data.st_col_no = current_token.col_no;
    expr->meta_data.end_line_no = current_token.line_no;
    expr->meta_data.end_col_no = current_token.end_col_no;
    return expr;
}

AST::Expression* Parser::_parseFloatLiteral() {
    auto expr = new AST::FloatLiteral(std::stod(current_token.literal));
    expr->set_meta_data(current_token.line_no, current_token.col_no, current_token.line_no, current_token.end_col_no);
    return expr;
}

AST::Expression* Parser::_parseBooleanLiteral() {
    auto expr = new AST::BooleanLiteral(current_token.type == TokenType::True);
    expr->set_meta_data(current_token.line_no, current_token.col_no, current_token.line_no, current_token.end_col_no);
    return expr;
}

AST::Expression* Parser::_parseNew() {
    this->_expectPeek({TokenType::Identifier, TokenType::LeftBracket}); // [newFT] -> [Identifier | LeftBracket]
    if (this->_currentTokenIs(TokenType::Identifier)) is_new_call = true;
    else if (this->_currentTokenIs(TokenType::LeftBracket)) is_new_arr = true;
    else { _currentTokenError(current_token.type, {TokenType::Identifier, TokenType::LeftBracket}, "Expected an identifier or a left bracket after 'new'."); }
    return this->_parseExpression(PrecedenceType::LOWEST); // [new] -> [Expression]
}

AST::Expression* Parser::_parseStringLiteral() {
    auto expr = new AST::StringLiteral(current_token.literal);
    expr->set_meta_data(current_token.line_no, current_token.col_no, current_token.line_no, current_token.end_col_no);
    return expr;
}

void Parser::_nextToken() {
    current_token = peek_token;
    peek_token = lexer->nextToken();
}

bool Parser::_currentTokenIs(TokenType type) {
    return current_token.type == type;
}

bool Parser::_peekTokenIs(TokenType type) {
    return peek_token.type == type;
}

bool Parser::_expectPeek(std::vector<TokenType> types, std::string suggestedFix) {
    for (auto type : types) {
        if (_peekTokenIs(type)) {
            _nextToken();
            LOG_TOK()
            return true;
        }
    }
    _peekTokenError(peek_token.type, types, suggestedFix);
}

bool Parser::_expectPeek(TokenType type, std::string suggestedFix) {
    if (_peekTokenIs(type)) {
        _nextToken();
        LOG_TOK()
        return true;
    }
    _peekTokenError(peek_token.type, {type}, suggestedFix);
}

PrecedenceType Parser::_currentPrecedence() {
    auto it = token_precedence.find(current_token.type);
    if (it != token_precedence.end()) {
        return it->second;
    } else {
        return PrecedenceType::LOWEST;
    }
}

PrecedenceType Parser::_peekPrecedence() {
    auto it = token_precedence.find(peek_token.type);
    if (it != token_precedence.end()) {
        return it->second;
    } else {
        return PrecedenceType::LOWEST;
    }
}

AST::Expression* Parser::_parseArrayLiteral() {
    auto elements = std::vector<AST::Expression*>();
    for (_nextToken(); !_currentTokenIs(TokenType::RightBracket); _nextToken()) { // [LeftBracket] -> [Element]
        LOG_TOK()
        if (_currentTokenIs(TokenType::Comma)) { continue; }                      // Skip commas
        auto expr = _parseExpression(PrecedenceType::LOWEST);                     // [Element] remains unchanged
        if (expr) { elements.push_back(expr); }
    }
    auto array = new AST::ArrayLiteral(elements, is_new_arr);
    is_new_arr = false;
    array->set_meta_data(current_token.line_no, current_token.col_no, current_token.line_no, current_token.end_col_no);
    return array;
}

AST::Expression* Parser::_parseIdentifier() {
    auto identifier = new AST::IdentifierLiteral(this->current_token);
    identifier->set_meta_data(current_token.line_no, current_token.col_no, current_token.line_no, current_token.end_col_no);
    if (_peekTokenIs(TokenType::LeftParen)) {
        auto functionCall = _parseFunctionCall(identifier, current_token.line_no, current_token.col_no);
        return functionCall;
    }
    return identifier;
}

void Parser::_peekTokenError(TokenType type, std::vector<TokenType> expected_types, std::string suggestedFix) {
    std::string expected_types_str;
    for (const auto& expected_type : expected_types) {
        if (!expected_types_str.empty()) { expected_types_str += ", "; }
        expected_types_str += token::tokenTypeString(expected_type);
    }
    errors::SyntaxError("SyntaxError", this->lexer->source, peek_token, "Expected one of: " + expected_types_str + " but got " + token::tokenTypeString(type), suggestedFix).raise();
}

void Parser::_currentTokenError(TokenType type, std::vector<TokenType> expected_types, std::string suggestedFix) {
    std::string expected_types_str;
    for (const auto& expected_type : expected_types) {
        if (!expected_types_str.empty()) { expected_types_str += ", "; }
        expected_types_str += token::tokenTypeString(expected_type);
    }
    errors::SyntaxError("SyntaxError", this->lexer->source, this->current_token, "Expected one of: " + expected_types_str + " but got " + token::tokenTypeString(type), suggestedFix).raise();
}

void Parser::_noPrefixParseFnError(TokenType type) {
    errors::NoPrefixParseFnError(this->lexer->source, peek_token, "No prefix parse function for " + token::tokenTypeString(type)).raise();
}
