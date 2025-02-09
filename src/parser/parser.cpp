#include <cstdlib>
#include <cstring>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>

#include "../errors/errors.hpp"
#include "AST/ast.hpp"
#include "macrointerpreter.hpp"
#include "parser.hpp"

using namespace parser;
#ifdef DEBUG_PARSER
#define LOG_MSG(msg) Logger::getInstance().log(__FILE__, __FUNCTION__, __LINE__, msg);
#define LOG_TOK() Logger::getInstance().log(__FILE__, __FUNCTION__, __LINE__, this->current_token.toString(tokens.source.string, false));
#else
#define LOG_MSG(msg)
#define LOG_TOK()
#endif

// Flags to indicate specific parsing states
bool is_new_call = false;
bool is_new_arr = false;

/*
Conventions for Token Representation:
- `-i`: Represents an integer, indicating a specific instance or count, appended
after declaration.
- `[TokenType]`: Encloses the current token type within square brackets.
- `stmt`: Represents a statement token.
- `exp`: Represents an expression token.
- `{Name}`: Represents a specific kind of token by name.
- `FT`: First Token in a sequence.
- `LT`: Last Token in a sequence.
- `|`: Denotes alternative tokens or outcomes.
- `->`: Indicates the transition from the previous state to the new state after
processing the current line.
- `stmtLT`: Typically represents a semicolon (`;`).
- `!` after `]`: Indicates that the token could also be unknown or optional.
*/

Parser::Parser(token::Tokens tokens, std::string file_path) : tokens(tokens), file_path(file_path) {
    // Initialize with the first two tokens
    this->_nextToken(); // [FT]
    LOG_MSG("First token read.");
    this->_nextToken(); // [LT]
    LOG_MSG("Peek token read.");
}

Parser::~Parser() {
    for (auto pair : macros) { pair.second->del(); }
}

AST::Program* Parser::parseProgram() {
    auto program = new AST::Program;
    LOG_MSG("Starting to parse program.");

    // Parse statements until EndOfFile token is reached
    while (current_token.type != TokenType::EndOfFile) {
        if (current_token.type == TokenType::AtTheRate) {
            if (peek_token.type == TokenType::Identifier && peek_token.getLiteral(tokens.source) == "macro") {
                current_token = peek_token;
                peek_token = tokens.nextToken();
                current_token = peek_token;
                peek_token = tokens.nextToken();
                LOG_TOK()
                auto name = current_token.getLiteral(tokens.source);
                LOG_MSG("Macro Name is `" + std::string(name) + "`")
                this->_expectPeek(TokenType::LeftBrace);
                LOG_TOK()
                auto body = this->_parseBlockStatement();
                LOG_TOK()
                macros[name] = new AST::MacroStatement(name, body);
                // void _parseMacroDecleration();
                LOG_TOK()
                this->_nextToken(); // [stmtLT] -> [stmtFT | EOF]
                LOG_TOK()
                continue;
            }
        }
        auto tok = current_token;
        auto statement = this->_parseStatement(); // [stmtFT] -> [stmtLT]

        if (statement->type == AST::NodeType::VariableDeclarationStatement
            ||statement->type == AST::NodeType::FunctionStatement
            ||statement->type == AST::NodeType::StructStatement
            ||statement->type == AST::NodeType::EnumStatement
            ||statement->type == AST::NodeType::ImportStatement) {
            program->statements.push_back(statement);
            LOG_MSG("Parsed a statement.");
        } else
            errors::raiseSyntaxError(this->file_path, tok, this->tokens.source.string, "Invalid Statement", "Only variable declarations, function declarations, struct declarations, enum declarations, and import statements are allowed at the top level.");
        this->_nextToken(); // [stmtLT] -> [stmtFT | EOF]
        LOG_TOK()
    } // [EOF]

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
        case TokenType::Enum:
            return this->_parseEnumStatement(); // [enumFT] -> [}]
        case TokenType::Try:
            return this->_parseTryCatchStatement(); // [tryFT] -> [;]
        case TokenType::Switch:
            return this->_parseSwitchCaseStatement(); // [switchFT] -> [;]
        case TokenType::Volatile:
            this->_nextToken(); // [volatileFT] -> [Identifier!]
            LOG_TOK()
            return this->_parseVariableDeclaration(nullptr, true); // [Identifier!FT] -> [;]
        case TokenType::Const:
            this->_nextToken(); // [volatileFT] -> [Identifier!]
            LOG_TOK()
            return this->_parseVariableDeclaration(nullptr, false, true); // [Identifier!FT] -> [;]
        default:
            return this->_parseExpressionStatement(); // [ExpressionFT] -> [;]
    }
}

AST::Statement* Parser::_interpretIdentifier() {
    auto identifier = _parseIdentifier();

    if (_peekTokenIs(TokenType::Colon)) {
        // Variable declaration
        return _parseVariableDeclaration(identifier);
    }

    if (_peekTokenIs(TokenType::Equals)) {
        // Variable assignment
        return _parseVariableAssignment(identifier);
    }

    if (_peekTokenIs(TokenType::LeftParen)) {
        // Function call
        auto functionCall = _parseFunctionCall(identifier);
        auto stmt = new AST::ExpressionStatement(identifier->firstToken, current_token, functionCall);
        _expectPeek(TokenType::Semicolon);
        return stmt;
    }

    return _parseExpressionStatement(identifier);
}

AST::Statement* Parser::_parseDeco() {
    this->_expectPeek(TokenType::Identifier); // [@FT] -> [Identifier]
    std::string name = this->current_token.getLiteral(tokens.source);

    if (name == "generic") {
        return this->_parseGenericDeco(); // [IdentifierFT] -> [)]
    } else if (name == "autocast") {
        return this->_parseAutocastDeco(); // [IdentifierFT] -> [)]
    } else if (name == "macros") {
        errors::raiseSyntaxError(this->file_path, this->current_token, this->tokens.source.string, "Macro define localy", "Define macro globaly it cant be declared localy");
    }
    errors::raiseSyntaxError(this->file_path,
                             this->current_token,
                             this->tokens.source.string,
                             "Unknown Deco type: " + name,
                             "Check the deco name for case sensitivity. Valid "
                             "options: `autocast` or `generic`.");
}

void Parser::_parseMacroDecleration() {
    auto name = current_token.getLiteral(tokens.source);
    LOG_MSG("Macro Name is `" + std::string(name) + "`")
    this->_expectPeek(TokenType::LeftBrace);
    LOG_TOK()
    auto body = this->_parseBlockStatement();
    LOG_TOK()
    macros[name] = new AST::MacroStatement(name, body);
}

AST::Statement* Parser::_parseGenericDeco() {
    this->_expectPeek(TokenType::LeftParen); // [Identifier] -> [(]
    this->_nextToken();                      // [(] -> [Identifier | )]!
    LOG_TOK()
    std::vector<AST::Type*> generics;

    while (this->current_token.type != TokenType::RightParen) {
        if (this->_currentTokenIs(TokenType::Identifier)) {
            auto identifier = _parseIdentifier();
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

            generics.push_back(new AST::Type(identifier->firstToken, current_token, identifier, type, false));
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
        if (_deco->type == AST::NodeType::FunctionStatement) {
            auto deco = _deco->castToFunctionStatement();
            deco->generic = generics;
            return deco;
        } else if (_deco->type == AST::NodeType::StructStatement) {
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
        func->autocast = true;
        return func;
    } else if (this->_currentTokenIs(TokenType::AtTheRate)) {
        auto _deco = this->_parseDeco(); // [@] -> [Decorator Statement]
        if (_deco->type == AST::NodeType::FunctionStatement) {
            auto deco = _deco->castToFunctionStatement();
            deco->autocast = true;
            return deco;
        }
    }

    this->_currentTokenError(this->current_token.type, {TokenType::Def, TokenType::AtTheRate});
}

std::vector<AST::FunctionParameter*> Parser::_parseFunctionParameters() {
    std::vector<AST::FunctionParameter*> parameters;
    while (this->current_token.type != TokenType::RightParen) {
        if (this->current_token.type == TokenType::Identifier) {
            auto identifier = _parseIdentifier();
            this->_expectPeek(TokenType::Colon); // Expect ':' after parameter name
            this->_nextToken();                  // Move to parameter type
            LOG_TOK()
            auto type = this->_parseType();
            parameters.push_back(new AST::FunctionParameter(identifier->firstToken, current_token, identifier, type, false));
            this->_expectPeek({TokenType::Comma, TokenType::RightParen});
            if (this->_currentTokenIs(TokenType::Comma)) {
                this->_nextToken(); // Consume ',' and continue
                LOG_TOK()
                continue;
            } else if (this->_currentTokenIs(TokenType::RightParen)) {
                break;
            }
        } else if (this->current_token.type == TokenType::Const) {
            auto first_pos = current_token.pos;
            this->_nextToken(); // Consume 'const' keyword
            LOG_TOK()
            this->_expectPeek(TokenType::Identifier);
            this->_nextToken(); // Move to parameter type
            LOG_TOK()
            auto type = this->_parseType();
            auto identifier = _parseIdentifier();
            parameters.push_back(new AST::FunctionParameter(first_pos, current_token, identifier, type, true));
            this->_expectPeek({TokenType::Comma, TokenType::RightParen});
            if (this->_currentTokenIs(TokenType::Comma)) {
                this->_nextToken(); // Consume ',' and continue
                LOG_TOK()
                continue;
            } else if (this->_currentTokenIs(TokenType::RightParen)) {
                break;
            }
        }else {
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
            auto identifier = _parseIdentifier();
            this->_expectPeek(TokenType::Colon); // Expect ':' after closure parameter name
            this->_nextToken();                  // Move to closure parameter type
            LOG_TOK()
            auto type = this->_parseType();
            closure_parameters.push_back(new AST::FunctionParameter(identifier->firstToken, current_token, identifier, type, false));
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
    auto first_pos = this->current_token.pos;
    this->_expectPeek(TokenType::Identifier);
    auto name = _parseIdentifier();

    this->_expectPeek(TokenType::LeftParen);
    this->_nextToken();
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
    bool return_const = false;
    AST::Type* return_type = nullptr;
    if (this->_peekTokenIs(TokenType::RightArrow)) {
        this->_nextToken(); // Consume '->'
        LOG_TOK()
        if (_peekTokenIs(TokenType::Const)) {
            return_const = true;
            _nextToken();
        }
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

    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);

    auto function_statement = new AST::FunctionStatement(first_pos, current_token, name, parameters, closure_parameters, return_type, return_const, body, std::vector<AST::Type*>{});

    return function_statement;
}

AST::WhileStatement* Parser::_parseWhileStatement() {
    auto first_pos = this->current_token.pos;
    _expectPeek(TokenType::LeftParen); // Expect '(' after 'while'
    _nextToken();                      // Move to condition expression
    LOG_TOK()

    auto condition = _parseExpression(PrecedenceType::LOWEST);

    _expectPeek(TokenType::RightParen); // Expect ')' after condition
    _nextToken();                       // Move to loop body
    LOG_TOK()

    auto body = _parseStatement(); // Parse the loop body

    LoopModifiers modifiers = _parseLoopModifiers(); // Parse any loop modifiers

    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);

    auto while_statement = new AST::WhileStatement(first_pos, current_token, condition, body, modifiers.ifbreak, modifiers.notbreak);
    return while_statement;
}

AST::Statement* Parser::_parseForStatement() {
    auto first_pos = this->current_token.pos;
    if (_peekTokenIs(TokenType::LeftParen)) {
        _nextToken();
        LOG_TOK()
        _nextToken();
        LOG_TOK()
        auto init = _parseStatement();
        _nextToken();
        LOG_TOK()
        auto condition = _parseExpression(PrecedenceType::LOWEST);
        _expectPeek(TokenType::Semicolon);
        _nextToken();
        LOG_TOK()
        auto updater = _parseStatement();
        _nextToken();
        LOG_TOK()
        auto body = _parseStatement();                   // Parse the loop body
        LoopModifiers modifiers = _parseLoopModifiers(); // Parse any loop modifiers
        uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
        auto for_statement = new AST::ForStatement(first_pos, current_token, init, condition, updater, body, modifiers.ifbreak, modifiers.notbreak);
        return for_statement;
    }
    _expectPeek(TokenType::Identifier); // Expect identifier in 'for identifier in ...
    auto get = _parseIdentifier();
    _expectPeek(TokenType::In); // Expect 'in' keyword
    _nextToken();               // Move to the 'from' expression
    LOG_TOK()
    auto from = _parseExpression(PrecedenceType::LOWEST); // Parse 'from' expression
    _nextToken();                                         // Move to loop body
    LOG_TOK()
    auto body = _parseStatement();                   // Parse the loop body
    LoopModifiers modifiers = _parseLoopModifiers(); // Parse any loop modifiers
    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
    auto for_statement = new AST::ForEachStatement(first_pos, current_token, get, from, body, modifiers.ifbreak, modifiers.notbreak);
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
    auto first_pos = this->current_token.pos;
    this->_nextToken(); // [breakFT] -> [Optional Loop Number | ;]
    LOG_TOK()
    uint8_t loopNum = 0;
    uint32_t pos = UINT24_MAX;
    if (this->_currentTokenIs(TokenType::Integer)) {
        loopNum = std::stoi(current_token.getLiteral(tokens.source)); // [IntegerFT] -> [Next Token]
        pos = current_token.pos;
        this->_nextToken(); // [Next Token] remains unchanged
        LOG_TOK()
    }
    auto break_statement = new AST::BreakStatement(first_pos, current_token, loopNum);
    break_statement->pos = pos;
    return break_statement;
}

AST::ContinueStatement* Parser::_parseContinueStatement() {
    auto first_pos = this->current_token.pos;
    this->_nextToken(); // [continueFT] -> [Optional Loop Number | ;]
    LOG_TOK()
    uint8_t loopNum = 0;
    uint32_t pos = UINT24_MAX;
    if (this->_currentTokenIs(TokenType::Integer)) {
        loopNum = std::stoi(current_token.getLiteral(tokens.source)); // [IntegerFT] -> [Next Token]
        pos = current_token.pos;
        this->_nextToken(); // [Next Token] remains unchanged
        LOG_TOK()
    }
    auto continue_statement = new AST::ContinueStatement(first_pos, current_token, loopNum);
    continue_statement->pos = pos;
    return continue_statement;
}

AST::ImportStatement* Parser::_parseImportStatement() {
    auto first_pos = this->current_token.pos;
    this->_expectPeek({TokenType::StringSSQ, TokenType::StringSTQ, TokenType::StringDSQ, TokenType::StringDTQ}); // [importFT] -> [String]
    auto path = this->current_token.getLiteral(tokens.source);
    std::string as = "";
    if (this->_peekTokenIs(TokenType::As)) {
        this->_nextToken();                                            // [String] -> [As]
        this->_expectPeek({TokenType::StringSSQ, TokenType::StringSTQ, TokenType::StringDSQ, TokenType::StringDTQ, TokenType::Identifier}); // [As] -> [String]
        as = this->current_token.getLiteral(tokens.source);
    }
    this->_expectPeek(TokenType::Semicolon); // [String] -> [;]
    auto import_statement = new AST::ImportStatement(first_pos, current_token, path, as);
    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
    return import_statement;
}

AST::CallExpression* Parser::_parseFunctionCall(AST::IdentifierLiteral* identifier) {
    auto first_pos = this->current_token.pos;
    if (!identifier) {
        identifier = _parseIdentifier();
    }
    this->_nextToken(); // [Identifier] -> [(] or [LeftParen]
    LOG_TOK()
    bool is_new_call_local = is_new_call;
    is_new_call = false;
    auto args = this->_parse_expression_list(TokenType::RightParen); // [(] -> [Arguments] -> [)]
    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
    auto call_expression = new AST::CallExpression(first_pos, current_token, identifier, args);
    call_expression->_new = is_new_call_local;
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
    auto first_pos = this->current_token.pos;
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        LOG_TOK()
        uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
        auto return_statement = new AST::ReturnStatement(first_pos, current_token);
        this->_nextToken(); // [returnFT] -> [;]
        return return_statement;
    }
    this->_nextToken(); // [returnFT] -> [Expression]
    LOG_TOK()
    auto expr = this->_parseExpression(PrecedenceType::LOWEST);
    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
    auto return_statement = new AST::ReturnStatement(first_pos, current_token, expr);
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken(); // [Expression] -> [;]
        LOG_TOK()
    }
    return return_statement;
}

AST::RaiseStatement* Parser::_parseRaiseStatement() {
    auto first_pos = this->current_token.pos;
    this->_nextToken(); // [raiseFT] -> [Expression]
    LOG_TOK()
    auto expr = this->_parseExpression(PrecedenceType::LOWEST); // [Expression] remains unchanged
    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
    auto raise_statement = new AST::RaiseStatement(first_pos, current_token, expr);
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken();
        LOG_TOK()
    } // [Expression] -> [;]
    return raise_statement;
}

AST::BlockStatement* Parser::_parseBlockStatement() {
    auto first_pos = this->current_token.pos;
    this->_nextToken(); // [{FT] -> [First Statement]
    LOG_TOK()

    std::vector<AST::Statement*> statements;
    while (!this->_currentTokenIs(TokenType::RightBrace) && !this->_currentTokenIs(TokenType::EndOfFile)) {
        auto stmt = this->_parseStatement(); // [StatementFT] -> [StatementLT]
        if (stmt) { statements.push_back(stmt); }
        this->_nextToken(); // [StatementLT] -> [Next Statement or }]
        LOG_TOK()
    }
    auto block_statement = new AST::BlockStatement(first_pos, current_token, statements);

    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken();
        LOG_TOK()
    } // [}LT] -> [;]

    return block_statement;
}

AST::Statement* Parser::_parseExpressionStatement(AST::Expression* first_token) {
    if (!first_token) {
        if (current_token.type == TokenType::Identifier) first_token = new AST::IdentifierLiteral(first_token->firstToken, first_token->lastToken, this->current_token.getLiteral(tokens.source));
        else if (current_token.type == TokenType::Integer) first_token = new AST::IntegerLiteral(first_token->firstToken, first_token->lastToken, std::atoll(this->current_token.getLiteral(tokens.source).c_str()));
        else if (current_token.type == TokenType::Float) first_token = new AST::FloatLiteral(first_token->firstToken, first_token->lastToken, std::atof(this->current_token.getLiteral(tokens.source).c_str()));
        else if (current_token.type == TokenType::StringSSQ) first_token = new AST::StringLiteral(first_token->firstToken, first_token->lastToken, this->current_token.getLiteral(tokens.source));
        else if (current_token.type == TokenType::StringSTQ) first_token = new AST::StringLiteral(first_token->firstToken, first_token->lastToken, this->current_token.getLiteral(tokens.source));
        else if (current_token.type == TokenType::StringDSQ) first_token = new AST::StringLiteral(first_token->firstToken, first_token->lastToken, this->current_token.getLiteral(tokens.source));
        else if (current_token.type == TokenType::StringDTQ) first_token = new AST::StringLiteral(first_token->firstToken, first_token->lastToken, this->current_token.getLiteral(tokens.source));
    }
    auto expr = this->_parseExpression(PrecedenceType::LOWEST, first_token); // [Expression] remains unchanged
    if (this->_peekTokenIs(TokenType::Equals)) return this->_parseVariableAssignment(expr); // [Variable Assignment]
    auto stmt = new AST::ExpressionStatement(first_token->firstToken, current_token, expr);
    this->_expectPeek(TokenType::Semicolon); // [ExpressionLT] -> [;]
    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
    return stmt;
}

AST::VariableDeclarationStatement* Parser::_parseVariableDeclaration(AST::Expression* identifier, bool is_volatile, bool is_const) {
    if (!identifier) {
        identifier = _parseIdentifier();
    }
    this->_expectPeek(TokenType::Colon); // [Identifier] -> [:]
    AST::Type* type = nullptr;
    if (!this->_peekTokenIs(TokenType::Equals)) {
        this->_nextToken(); // [:] -> [Type]
        LOG_TOK()
        type = this->_parseType(); // [Type] remains unchanged
        if (type->name->type == AST::NodeType::IdentifierLiteral && type->name->castToIdentifierLiteral()->value == "auto") {
            type->del();
            type = nullptr;
        }
    }
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        auto variableDeclarationStatement = new AST::VariableDeclarationStatement(identifier->firstToken, current_token, identifier, type, nullptr, is_volatile, is_const);
        this->_nextToken(); // [Type] -> [;]
        LOG_TOK()
        uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
        return variableDeclarationStatement;
    } else if (this->_expectPeek(TokenType::Equals)) {
        this->_nextToken(); // [Type] -> [=] or [Type] -> [;]
        LOG_TOK()
        auto expr = this->_parseExpression(PrecedenceType::LOWEST); // [Expression] remains unchanged
        auto variableDeclarationStatement = new AST::VariableDeclarationStatement(identifier->firstToken, current_token, identifier, type, expr, is_volatile, is_const);
        this->_expectPeek(TokenType::Semicolon);
        LOG_TOK()
        uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
        return variableDeclarationStatement;
    }
    return nullptr;
}

AST::TryCatchStatement* Parser::_parseTryCatchStatement() {
    auto first_pos = this->current_token.pos;
    this->_nextToken(); // [tryFT] -> [Statement]
    LOG_TOK()
    auto try_block = this->_parseStatement(); // [Statement] -> [Next Token]
    std::vector<std::tuple<AST::Type*, AST::IdentifierLiteral*, AST::Statement*>> catch_blocks;

    while (this->_peekTokenIs(TokenType::Catch)) {
        this->_nextToken(); // [try] -> [catch]
        LOG_TOK()
        this->_expectPeek(TokenType::LeftParen); // [catch] -> [(]
        this->_nextToken();                      // [(] -> [Exception Type]
        LOG_TOK()
        auto exception_type = this->_parseType(); // [Exception Type] remains unchanged
        this->_expectPeek(TokenType::Identifier); // [Exception Type] -> [Identifier]
        auto exception_var = _parseIdentifier();
        this->_expectPeek(TokenType::RightParen); // [Identifier] -> [)]
        this->_nextToken();                       // [)] -> [Catch Block Statement]
        LOG_TOK()
        auto catch_block = this->_parseStatement(); // [Statement] -> [Next Token]
        catch_blocks.push_back({exception_type, exception_var, catch_block});
    }

    if (catch_blocks.empty()) {
        this->_peekTokenError(this->current_token.type, {TokenType::Catch});
        return nullptr;
    }

    return new AST::TryCatchStatement(first_pos, current_token, try_block, catch_blocks);
}

AST::SwitchCaseStatement* Parser::_parseSwitchCaseStatement() {
    auto first_pos = this->current_token.pos;
    this->_expectPeek(TokenType::LeftParen); // [switchFT] -> [(]
    LOG_TOK()
    this->_nextToken(); // [(] -> [ExpresionFT]
    LOG_TOK()
    auto condition = this->_parseExpression(PrecedenceType::LOWEST); // [ExpresisonFT] -> [ExpresisonLT]
    this->_nextToken();                                              // [ExpresisonLT] -> [)]
    this->_expectPeek(TokenType::LeftBrace);                         // [)] -> [{]
    std::vector<std::tuple<AST::Expression*, AST::Statement*>> case_blocks;

    while (this->_peekTokenIs(TokenType::Case)) {
        this->_nextToken(); // [try] -> [catch]
        LOG_TOK()
        this->_expectPeek(TokenType::LeftParen); // [catch] -> [(]
        this->_nextToken();                      // [(] -> [Exception Type]
        LOG_TOK()
        auto _case = this->_parseExpression(PrecedenceType::LOWEST); // [Exception Type] remains unchanged
        this->_expectPeek(TokenType::RightParen);                    // [Identifier] -> [)]
        this->_nextToken();                                          // [)] -> [Catch Block Statement]
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
    return new AST::SwitchCaseStatement(first_pos, current_token, condition, case_blocks, other);
}

AST::Expression* Parser::_parseInfixIdenifier() {
    auto first_pos = this->current_token.pos;
    if (this->current_token.type != TokenType::Identifier) {
        std::cerr << "Cannot parse infixIdentifier Expression. Token: " << token::tokenTypeToString(this->current_token.type) << std::endl;
        exit(1);
    }
    if (!this->_peekTokenIs(TokenType::Dot)) { return _parseIdentifier(); }
    auto li = _parseIdentifier();
    this->_nextToken(); // [Identifier] -> [.]
    LOG_TOK()
    this->_nextToken(); // [.] -> [Next Identifier]
    LOG_TOK()
    return new AST::InfixExpression(first_pos, current_token, li, TokenType::Dot, ".",
                                    this->_parseInfixIdenifier()); // [Next Identifier] remains unchanged
}

AST::Type* Parser::_parseType() {
    auto first_pos = this->current_token.pos;
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
            if (this->current_token.type == TokenType::Comma) {
                this->_nextToken();
                LOG_TOK()
            }
        }
    }
    if (this->_peekTokenIs(TokenType::Refrence)) {
        this->_nextToken(); // [Type] -> [&]
        LOG_TOK()
        ref = true;
    }
    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
    auto generic_type_node = new AST::Type(first_pos, current_token, name, generics, ref);
    return generic_type_node;
}

AST::VariableAssignmentStatement* Parser::_parseVariableAssignment(AST::Expression* identifier) {
    if (!identifier) {
        identifier = _parseIdentifier();
    }
    this->_expectPeek(TokenType::Equals); // [Identifier] -> [=]
    this->_nextToken();                   // [=] -> [Expression]
    LOG_TOK()
    auto expr = this->_parseExpression(PrecedenceType::LOWEST); // [Expression] remains unchanged
    auto stmt = new AST::VariableAssignmentStatement(identifier->firstToken, current_token, identifier, expr);
    this->_expectPeek(TokenType::Semicolon);
    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
    return stmt;
}

AST::StructStatement* Parser::_parseStructStatement() {
    auto first_pos = this->current_token.pos;
    this->_expectPeek(TokenType::Identifier); // [structFT] -> [Identifier]
    AST::Expression* name = _parseIdentifier();

    this->_expectPeek(TokenType::LeftBrace); // [Identifier] -> [{]
    this->_nextToken();                      // [{] -> [Struct Body]
    LOG_TOK()
    std::vector<AST::Statement*> statements;

    while (!this->_currentTokenIs(TokenType::RightBrace) && !this->_currentTokenIs(TokenType::EndOfFile)) {
        if (this->_currentTokenIs(TokenType::Def)) {
            auto stmt = this->_parseFunctionStatement(); // [defFT] -> [Function Statement]
            if (stmt) statements.push_back(stmt);
        } else if (this->_currentTokenIs(TokenType::Identifier)) {
            auto stmt = this->_parseVariableDeclaration(); // [Identifier] ->
                                                           // [Variable Declaration]
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

    auto struct_stmt = new AST::StructStatement(first_pos, current_token, name, statements);

    if (this->peek_token.type == TokenType::Semicolon) {
        this->_nextToken();
        LOG_TOK()
    } // [}] -> [;]
    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
    return struct_stmt;
}

AST::Expression* Parser::_parseExpression(PrecedenceType precedence, AST::Expression* parsed_expression) {
    if (!parsed_expression) {
        auto iter = prefix_parse_fns.find(current_token.type);
        if (iter == prefix_parse_fns.end()) {
            errors::raiseNoPrefixParseFnError(this->file_path, current_token, this->tokens.source.string, "No prefix parse function for " + token::tokenTypeToString(current_token.type));
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
    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
    uint32_t end_col_no = current_token.getEnColNo(tokens.source) - 1;
    return parsed_expression;
}

AST::IfElseStatement* Parser::_parseIfElseStatement() {
    auto first_pos = this->current_token.pos;
    this->_expectPeek(TokenType::LeftParen); // [ifFT] -> [(]
    this->_nextToken();                      // [(] -> [Condition]
    LOG_TOK()
    auto condition = this->_parseExpression(PrecedenceType::LOWEST);
    this->_expectPeek(TokenType::RightParen); // [Condition] -> [)]
    this->_nextToken();                       // [)] -> [Consequence]
    LOG_TOK()
    auto consequence = this->_parseStatement(); // [Consequence] -> [Next Token]
    AST::Statement* alternative = nullptr;
    if (this->_peekTokenIs(TokenType::Else)) {
        this->_nextToken(); // [Consequence] -> [else]
        LOG_TOK()
        this->_nextToken(); // [else] -> [Alternative]
        LOG_TOK()
        alternative = this->_parseStatement(); // [Alternative] -> [Next Token]
    }
    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
    auto if_else_statement = new AST::IfElseStatement(first_pos, current_token, condition, consequence, alternative);
    return if_else_statement;
}

AST::EnumStatement* Parser::_parseEnumStatement() {
    auto first_pos = this->current_token.pos;
    this->_expectPeek(TokenType::Identifier); // [enumFT] -> [Identifier]
    AST::Expression* name = _parseIdentifier();

    this->_expectPeek(TokenType::LeftBrace); // [Identifier] -> [{]
    this->_nextToken();                      // [{] -> [Enum Body]
    LOG_TOK()
    std::vector<std::string> fields;

    while (!this->_currentTokenIs(TokenType::RightBrace) && !this->_currentTokenIs(TokenType::EndOfFile)) {
        if (this->_currentTokenIs(TokenType::Identifier)) {
            fields.push_back(current_token.getLiteral(tokens.source));
            this->_nextToken(); // [;] -> [} | Ident]
            LOG_TOK()
        } else {
            this->_currentTokenError(current_token.type, {TokenType::Identifier});
            return nullptr;
        }
        this->_nextToken(); // [StatementLT] -> [Next Statement or }]
        LOG_TOK()
    }
    auto enum_stmt = new AST::EnumStatement(first_pos, current_token, name, fields);

    if (this->peek_token.type == TokenType::Semicolon) {
        this->_nextToken();
        LOG_TOK()
    } // [}] -> [;]
    uint32_t end_line_no = current_token.getEnLineNo(tokens.source);
    return enum_stmt;
};

AST::Expression* Parser::_parseInfixExpression(AST::Expression* leftNode) {
    auto precedence = this->_currentPrecedence();
    auto opp = current_token;
    this->_nextToken(); // [Operator] -> [Next Expression]
    LOG_TOK()
    auto right = this->_parseExpression(precedence); // [Next Expression] remains unchanged
    auto infix_expr = new AST::InfixExpression(leftNode->firstToken, current_token,leftNode, opp.type, this->current_token.getLiteral(tokens.source));
    infix_expr->right = right;
    return infix_expr;
}

AST::Expression* Parser::_parseIndexExpression(AST::Expression* leftNode) {
    this->_nextToken(); // [LeftBracket] -> [Index Expression]
    LOG_TOK()
    auto index = this->_parseExpression(PrecedenceType::LOWEST);
    auto index_expr = new AST::IndexExpression(leftNode->firstToken, current_token, leftNode, index);
    this->_expectPeek(TokenType::RightBracket); // [Index Expression] -> [RightBracket]
    return index_expr;
}

AST::Expression* Parser::_parseGroupedExpression() {
    this->_nextToken(); // [LeftParen] -> [Grouped Expression]
    LOG_TOK()
    uint32_t st_line_no = this->current_token.getEnLineNo(tokens.source);
    auto expr = this->_parseExpression(PrecedenceType::LOWEST); // [Grouped ExpressionFT] -> [Grouped
                                                                // ExpressionLT]
    this->_expectPeek(TokenType::RightParen);                   // [Grouped Expression] -> [RightParen]
    uint32_t end_line_no = this->current_token.getEnLineNo(tokens.source);
    uint32_t end_col_no = this->current_token.getEnColNo(tokens.source);
    return expr;
}

AST::IntegerLiteral* Parser::_parseIntegerLiteral() {
    auto literal = current_token.getLiteral(tokens.source);
    auto expr = new AST::IntegerLiteral(current_token.pos, current_token, std::stoll(literal));
    return expr;
}

AST::FloatLiteral* Parser::_parseFloatLiteral() {
    auto expr = new AST::FloatLiteral(current_token.pos, current_token, std::stod(current_token.getLiteral(tokens.source)));
    return expr;
}

AST::BooleanLiteral* Parser::_parseBooleanLiteral() {
    auto expr = new AST::BooleanLiteral(current_token.pos, current_token, current_token.type == TokenType::True);
    return expr;
}

AST::Expression* Parser::_parseNew() {
    this->_expectPeek({TokenType::Identifier, TokenType::LeftBracket}); // [newFT] -> [Identifier | LeftBracket]
    if (this->_currentTokenIs(TokenType::Identifier)) is_new_call = true;
    else if (this->_currentTokenIs(TokenType::LeftBracket)) is_new_arr = true;
    auto x = this->_parseExpression(PrecedenceType::LOWEST); // [new] -> [Expression]
    if (this->_currentTokenIs(TokenType::Identifier)) is_new_call = false;
    else if (this->_currentTokenIs(TokenType::LeftBracket)) is_new_arr = false;
    return x;
}

AST::StringLiteral* Parser::_parseStringLiteral() {
    auto expr = new AST::StringLiteral(current_token.pos, current_token, current_token.getLiteral(tokens.source));
    return expr;
}

void Parser::_nextToken() {
    current_token = peek_token;
    peek_token = tokens.nextToken();
    LOG_TOK()
    if (current_token.type == TokenType::AtTheRate && peek_token.type == TokenType::Identifier && this->macros.contains(peek_token.getLiteral(tokens.source))) {
        std::cout << "Caught By _nextToken" << std::endl;
        current_token = peek_token;
        peek_token = tokens.nextToken();
        MacroInterpreter(tokens, this).interpret(this->macros[current_token.getLiteral(tokens.source)]);
        tokens.token_buffer.push_front(peek_token);
        peek_token = tokens.nextToken();
        current_token = peek_token;
        peek_token = tokens.nextToken();
    }
}

bool Parser::_currentTokenIs(TokenType type) {
    return current_token.type == type;
}

bool Parser::_peekTokenIs(TokenType type) {
    if (peek_token.type == TokenType::AtTheRate) {
        auto prev_token = peek_token;
        peek_token = tokens.nextToken();
        if (this->macros.contains(peek_token.getLiteral(tokens.source))) {
            std::cout << "Caught By _peekTokenIs" << std::endl;
            auto prev_current = current_token;
            current_token = peek_token;
            peek_token = tokens.nextToken();
            MacroInterpreter(tokens, this).interpret(this->macros[current_token.getLiteral(tokens.source)]);
            current_token = prev_token;
            peek_token = tokens.nextToken();
            return peek_token.type == type;
        }
        peek_token = prev_token;
    }
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

AST::ArrayLiteral* Parser::_parseArrayLiteral() {
    auto first_pos = this->current_token.pos;
    auto elements = std::vector<AST::Expression*>();
    bool is_new_arr_local = is_new_arr;
    is_new_arr = false;
    if (this->_peekTokenIs(TokenType::RightBracket)) {
        errors::raiseCompletionError(this->file_path,
                                     this->tokens.source.string,
                                     this->current_token.getStLineNo(tokens.source),
                                     this->current_token.getStColNo(tokens.source) + 1,
                                     this->peek_token.getEnLineNo(tokens.source),
                                     this->peek_token.getEnColNo(tokens.source) + 1,
                                     "Can initialize Empty Array",
                                     "initialize Array like `array(type, length) or vector(type)`");
    }
    for (_nextToken(); !_currentTokenIs(TokenType::RightBracket); _nextToken()) { // [LeftBracket] -> [Element]
        LOG_TOK()
        if (_currentTokenIs(TokenType::Comma)) { continue; }  // Skip commas
        auto expr = _parseExpression(PrecedenceType::LOWEST); // [Element] remains unchanged
        if (expr) { elements.push_back(expr); }
    }
    auto array = new AST::ArrayLiteral(first_pos, current_token, elements, is_new_arr_local);
    return array;
}

AST::IdentifierLiteral* Parser::_parseIdentifier() {
    auto identifier = new AST::IdentifierLiteral(current_token.pos, current_token, this->current_token.getIdentLiteral(tokens.source));
    return identifier;
}

AST::Expression* Parser::_InterpretExpresionIdentifier() {
    auto identifier = _parseIdentifier();
    if (_peekTokenIs(TokenType::LeftParen)) {
        auto functionCall = _parseFunctionCall(identifier);
        return functionCall;
    }
    return identifier;
}

void Parser::_peekTokenError(TokenType type, std::vector<TokenType> expected_types, std::string suggestedFix) {
    std::string expected_types_str;
    for (const auto& expected_type : expected_types) {
        if (!expected_types_str.empty()) { expected_types_str += ", "; }
        expected_types_str += token::tokenTypeToString(expected_type);
    }
    errors::raiseSyntaxError(this->file_path, peek_token, this->tokens.source.string, "Expected one of: " + expected_types_str + " but got " + token::tokenTypeToString(type), suggestedFix);
}

void Parser::_currentTokenError(TokenType type, std::vector<TokenType> expected_types, std::string suggestedFix) {
    std::string expected_types_str;
    for (const auto& expected_type : expected_types) {
        if (!expected_types_str.empty()) { expected_types_str += ", "; }
        expected_types_str += token::tokenTypeToString(expected_type);
    }
    errors::raiseSyntaxError(this->file_path, this->current_token, this->tokens.source.string, "Expected one of: " + expected_types_str + " but got " + token::tokenTypeToString(type), suggestedFix);
}
