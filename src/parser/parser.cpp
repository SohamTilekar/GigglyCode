#include <cstdlib>
#include <iostream>
#include <memory>
#include <ostream>
#include <string>
#include <vector>

#include "AST/ast.hpp"
#include "parser.hpp"

using namespace parser;

bool is_new_call = false;
bool is_new_arr  = false;

/*
Conventions for the Token Representasion
- `-i` can be intiger which can be used to state which statement, it can appended after its deceleration
- `[x]` current token can us showned by enclosing it in [].
- `stmt`: it represents the token of the statement
- `exp`: it represents the token of the expression
- `{name}`: it represents the token of the specific Kind
- `FT` Represtents First token
- `LT` Represtents Last Token
- `|`: can be used to tell diffrent input or outcome
- previous & the new state after runing the current line is shown by `->`
- stmtLT will be mostly `;`
- ! can be appended after ] to represent that it could be also unknown
*/

Parser::Parser(LexerPtr lexer) : lexer(lexer) {
    this->_nextToken();
    this->_nextToken();
}

shared_ptr<AST::Program> Parser::parseProgram() {
    auto program    = make_shared<AST::Program>();
    int startLineNo = current_token->line_no;
    int startColNo  = current_token->col_no;

    // Parse statements until the end of the file
    while (current_token->type != TokenType::EndOfFile) {
        auto statement = this->_parseStatement(); // [stmtFT-1] -> [stmtLT-1]
        if (statement) { program->statements.push_back(statement); }
        this->_nextToken(); // [stmtLT-1] -> [stmtFT-2 | EOF]
    } // [EOF]

    int endLineNo = current_token->line_no;
    int endColNo  = current_token->col_no;
    program->set_meta_data(startLineNo, startColNo, endLineNo, endColNo);
    return program;
}

StatementPtr Parser::_parseStatement() {
    switch (current_token->type) {
        case TokenType::Identifier: return this->_interpretIdentifier();    // [identifierFT] -> [`;`]
        case TokenType::LeftBrace:  return this->_parseBlockStatement();    // [`{`] -> [`;` | `}`]
        case TokenType::Return:     return this->_parseReturnStatement();   // [`return`] -> [`;`]
        case TokenType::Raise:      return this->_parseRaiseStatement();    // [`raise`] -> [`;`]
        case TokenType::Def:        return this->_parseFunctionStatement(); // [`def`] -> [`;` | `}`]
        case TokenType::AtTheRate:  return this->_parseDeco();              // [`@`] -> [`;`]
        case TokenType::If:         return this->_parseIfElseStatement();   // [`if`] -> [`}`]
        case TokenType::While:      return this->_parseWhileStatement();    // [`while`] -> [`}`]
        case TokenType::For:        return this->_parseForStatement();      // [`for`] -> [`}`]
        case TokenType::Break:      return this->_parseBreakStatement();    // [`break`] -> [`;`]
        case TokenType::Continue:   return this->_parseContinueStatement(); // [`continue`] -> [`;`]
        case TokenType::Import:     return this->_parseImportStatement();   // [`import`] -> [`;`]
        case TokenType::Struct:     return this->_parseStructStatement();   // [`struct`] -> [`}`]
        case TokenType::Try:        return this->_parseTryCatchStatement();        // [`try`] -> [`;`]
        case TokenType::Volatile:
            this->_nextToken();                                            // [`volatile`] -> [`identifier`]!
            return this->_parseVariableDeclaration(nullptr, -1, -1, true); // [`identifier`]! -> [`;`]
        default: return this->_parseExpressionStatement();                 // [exprFT] -> [`;`]
    }
}

StatementPtr Parser::_interpretIdentifier() {
    int stLineNo    = current_token->line_no;
    int stColNo     = current_token->col_no;
    auto identifier = _parseIdentifier();

    if (_peekTokenIs(TokenType::Colon)) {
        // Variable declaration
        return _parseVariableDeclaration(identifier, stLineNo, stColNo); // [identifierFT] -> [stmtLT]
    } else if (_peekTokenIs(TokenType::Equals)) {
        // Variable assignment
        return _parseVariableAssignment(identifier, stLineNo, stColNo); // [identifierFT] -> [stmtLT]
    } else if (_peekTokenIs(TokenType::LeftParen)) {
        // Function call
        auto functionCall = _parseFunctionCall(identifier, stLineNo, stColNo); // [identifierFT] -> [`)`]
        auto stmt         = make_shared<AST::ExpressionStatement>(functionCall);
        _expectPeek(TokenType::Semicolon); // [`)`] -> [`;`]
        return stmt;
    } else {
        // Expression statement
        return _parseExpressionStatement(identifier, stLineNo, stColNo); // [exprFT] -> [`;`]
    }
}

StatementPtr Parser::_parseDeco() {
    _expectPeek(TokenType::Identifier); // [`@`] -> [identifier]
    std::string name = this->current_token->literal;

    if (name == "generic") {
        return _parseGenericDeco(); // [identifier] -> [`}`]
    } else if (name == "autocast") {
        return _parseAutocastDeco(); // [identifier] -> [`}`]
    }
    errors::SyntaxError(
        "WrongDecoName",
        this->lexer->source,
        *this->current_token,
        "Unknown Deco type: " + name,
        "Check the Name deco name is also case sensitive. They could be `autocast` or `generic`"
    )
        .raise();
}

StatementPtr Parser::_parseGenericDeco() {
    this->_expectPeek(TokenType::LeftParen); // [identifier] -> [`(`]
    this->_nextToken();                      // [`(`] -> [identifier | `)`]!
    std::vector<shared_ptr<AST::GenericType>> generics;

    while (this->current_token->type != TokenType::RightParen) {
        if (this->_currentTokenIs(TokenType::Identifier)) {
            auto identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
            this->_expectPeek(TokenType::Colon);
            this->_nextToken();
            std::vector<shared_ptr<AST::Type>> type;

            while (this->current_token->type != TokenType::RightParen && this->current_token->type != TokenType::Comma) {
                type.push_back(this->_parseType());
                if (this->_peekTokenIs(TokenType::Pipe)) {
                    this->_nextToken();
                    this->_nextToken();
                } else break;
            }

            generics.push_back(make_shared<AST::GenericType>(identifier, type));
            if (this->_peekTokenIs(TokenType::Comma)) {
                this->_nextToken();
                this->_nextToken();
            } else break;
        } else {
            _currentTokenError(current_token->type, {TokenType::Identifier});
            break;
        }
    }

    this->_expectPeek({TokenType::RightParen});
    this->_expectPeek({TokenType::Def, TokenType::Struct, TokenType::AtTheRate});

    if (this->_currentTokenIs(TokenType::Def)) {
        auto func     = this->_parseFunctionStatement();
        func->generic = generics;
        return func;
    } else if (this->_currentTokenIs(TokenType::Struct)) {
        auto _struct      = this->_parseStructStatement();
        _struct->generics = generics;
        return _struct;
    } else if (this->_currentTokenIs(TokenType::AtTheRate)) {
        auto _deco = this->_parseDeco();
        if (_deco->type() == AST::NodeType::FunctionStatement) {
            auto deco     = std::static_pointer_cast<AST::FunctionStatement>(_deco);
            deco->generic = generics;
            return deco;
        } else if (_deco->type() == AST::NodeType::StructStatement) {
            auto deco      = std::static_pointer_cast<AST::StructStatement>(_deco);
            deco->generics = generics;
            return deco;
        }
    }

    this->_currentTokenError(this->current_token->type, {TokenType::Def, TokenType::AtTheRate});
}

StatementPtr Parser::_parseAutocastDeco() {
    this->_expectPeek({TokenType::Def, TokenType::Struct, TokenType::AtTheRate});

    if (this->_currentTokenIs(TokenType::Def)) {
        auto func                    = this->_parseFunctionStatement();
        func->extra_info["autocast"] = true;
        return func;
    } else if (this->_currentTokenIs(TokenType::AtTheRate)) {
        auto _deco = this->_parseDeco();
        if (_deco->type() == AST::NodeType::FunctionStatement) {
            auto deco                    = std::static_pointer_cast<AST::FunctionStatement>(_deco);
            deco->extra_info["autocast"] = true;
            return deco;
        }
    }

    this->_currentTokenError(this->current_token->type, {TokenType::Def, TokenType::AtTheRate});
}

shared_ptr<AST::FunctionStatement> Parser::_parseFunctionStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no  = current_token->col_no;
    this->_expectPeek({TokenType::Identifier});
    auto name = make_shared<AST::IdentifierLiteral>(this->current_token);
    name->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    this->_expectPeek({TokenType::LeftParen});
    this->_nextToken();
    std::vector<shared_ptr<AST::FunctionParameter>> parameters;
    while (this->current_token->type != TokenType::RightParen) {
        if (this->current_token->type == TokenType::Identifier) {
            auto identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
            this->_expectPeek({TokenType::Colon});
            this->_nextToken();
            auto type = this->_parseType();
            parameters.push_back(make_shared<AST::FunctionParameter>(identifier, type));
            this->_expectPeek({TokenType::Comma, TokenType::RightParen});
            if (this->_currentTokenIs(TokenType::Comma)) {
                this->_nextToken();
                continue;
            } else if (this->_currentTokenIs(TokenType::RightParen)) break;
        } else {
            _currentTokenError(current_token->type, {TokenType::Identifier});
            break;
        }
    }
    std::vector<shared_ptr<AST::FunctionParameter>> closure_parameters;
    if (this->_peekTokenIs(TokenType::Use)) {
        this->_nextToken();
        this->_nextToken();
        this->_nextToken();
        while (!this->_currentTokenIs(TokenType::RightParen)) {
            if (this->_currentTokenIs(TokenType::Identifier)) {
                auto identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
                this->_expectPeek({TokenType::Colon});
                this->_nextToken();
                auto type = this->_parseType();
                closure_parameters.push_back(make_shared<AST::FunctionParameter>(identifier, type));
                this->_nextToken();
                if (this->_currentTokenIs(TokenType::Comma)) {
                    this->_nextToken();
                    continue;
                } else if (this->_currentTokenIs(TokenType::RightParen)) break;
                else {
                    _currentTokenError(current_token->type, {TokenType::Comma, TokenType::RightParen});
                    break;
                }
            } else {
                _currentTokenError(current_token->type, {TokenType::Identifier});
                break;
            }
        }
    }
    shared_ptr<AST::Type> return_type = nullptr;
    if (this->_peekTokenIs(TokenType::RightArrow)) {
        this->_nextToken();
        this->_nextToken();
        return_type = this->_parseType();
    }
    shared_ptr<AST::BlockStatement> body = nullptr;
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken();
    } else {
        this->_expectPeek({TokenType::LeftBrace});
        body = this->_parseBlockStatement();
    }
    int end_line_no         = current_token->line_no;
    int end_col_no          = current_token->col_no;
    auto function_statement = make_shared<AST::FunctionStatement>(name, parameters, closure_parameters, return_type, body, std::vector<shared_ptr<AST::GenericType>>{});
    function_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return function_statement;
}

shared_ptr<AST::WhileStatement> Parser::_parseWhileStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no  = current_token->col_no;
    this->_expectPeek({TokenType::LeftParen});
    this->_nextToken();
    auto condition = this->_parseExpression(PrecedenceType::LOWEST);
    this->_expectPeek({TokenType::RightParen});
    this->_nextToken();
    auto body             = this->_parseStatement();
    StatementPtr notbreak = nullptr;
    StatementPtr ifbreak  = nullptr;
    while (this->_peekTokenIs(TokenType::NotBreak) || this->_peekTokenIs(TokenType::IfBreak)) {
        if (this->_peekTokenIs(TokenType::NotBreak)) {
            this->_nextToken();
            this->_nextToken();
            notbreak = this->_parseStatement();
        } else if (this->_peekTokenIs(TokenType::IfBreak)) {
            this->_nextToken();
            this->_nextToken();
            ifbreak = this->_parseStatement();
        }
    }
    int end_line_no      = current_token->line_no;
    int end_col_no       = current_token->col_no;
    auto while_statement = make_shared<AST::WhileStatement>(condition, body, ifbreak, notbreak);
    while_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return while_statement;
}

shared_ptr<AST::ForStatement> Parser::_parseForStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no  = current_token->col_no;
    this->_expectPeek({TokenType::LeftParen});
    this->_expectPeek({TokenType::Identifier});
    auto get = make_shared<AST::IdentifierLiteral>(this->current_token);
    this->_expectPeek({TokenType::In});
    this->_nextToken();
    auto from = this->_parseExpression(PrecedenceType::LOWEST);
    this->_expectPeek({TokenType::RightParen});
    this->_nextToken();
    auto body             = this->_parseStatement();
    StatementPtr notbreak = nullptr;
    StatementPtr ifbreak  = nullptr;
    while (this->_peekTokenIs(TokenType::NotBreak) || this->_peekTokenIs(TokenType::IfBreak)) {
        if (this->_peekTokenIs(TokenType::NotBreak)) {
            this->_nextToken();
            this->_nextToken();
            notbreak = this->_parseStatement();
        } else if (this->_peekTokenIs(TokenType::IfBreak)) {
            this->_nextToken();
            this->_nextToken();
            ifbreak = this->_parseStatement();
        }
    }
    int end_line_no    = current_token->line_no;
    int end_col_no     = current_token->col_no;
    auto for_statement = make_shared<AST::ForStatement>(get, from, body, ifbreak, notbreak);
    for_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return for_statement;
}

shared_ptr<AST::BreakStatement> Parser::_parseBreakStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no  = current_token->col_no;
    this->_nextToken();
    int loopNum = 0;
    if (this->_currentTokenIs(TokenType::Integer)) {
        loopNum = std::stoi(current_token->literal);
        this->_nextToken();
    }
    int end_line_no      = current_token->line_no;
    int end_col_no       = current_token->col_no;
    auto break_statement = make_shared<AST::BreakStatement>(loopNum);
    break_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return break_statement;
}

shared_ptr<AST::ContinueStatement> Parser::_parseContinueStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no  = current_token->col_no;
    this->_nextToken();
    int loopNum = 0;
    if (this->_currentTokenIs(TokenType::Integer)) {
        loopNum = std::stoi(current_token->literal);
        this->_nextToken();
    }
    int end_line_no         = current_token->line_no;
    int end_col_no          = current_token->col_no;
    auto continue_statement = make_shared<AST::ContinueStatement>(loopNum);
    continue_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return continue_statement;
}

shared_ptr<AST::ImportStatement> Parser::_parseImportStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no  = current_token->col_no;
    this->_expectPeek({TokenType::String});
    auto import_statement = make_shared<AST::ImportStatement>(this->current_token->literal);
    this->_expectPeek({TokenType::Semicolon});
    int end_line_no = current_token->line_no;
    int end_col_no  = current_token->col_no;
    import_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return import_statement;
}

ExpressionPtr Parser::_parseFunctionCall(ExpressionPtr identifier, int st_line_no, int st_col_no) {
    if (!identifier) {
        st_line_no = current_token->line_no;
        st_col_no  = current_token->col_no;
        identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
    }
    identifier->set_meta_data(st_line_no, st_col_no, current_token->line_no, current_token->end_col_no);
    std::cout << "Before: ";
    this->current_token->print();
    this->_nextToken();
    bool is_new_call_local = is_new_call;
    is_new_call            = false;
    auto args              = this->_parse_expression_list(TokenType::RightParen);
    std::cout << "After: ";
    this->current_token->print();
    int end_line_no       = current_token->line_no;
    int end_col_no        = current_token->col_no;
    auto call_expression  = make_shared<AST::CallExpression>(identifier, args);
    call_expression->_new = is_new_call_local;
    call_expression->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return call_expression;
}

std::vector<ExpressionPtr> Parser::_parse_expression_list(TokenType end) {
    std::vector<ExpressionPtr> args;
    if (this->_peekTokenIs(end)) {
        this->_nextToken();
        return args;
    }
    this->_nextToken();
    args.push_back(this->_parseExpression(PrecedenceType::LOWEST));
    while (this->_peekTokenIs(TokenType::Comma)) {
        this->_nextToken();
        this->_nextToken();
        args.push_back(this->_parseExpression(PrecedenceType::LOWEST));
    }
    this->_expectPeek({end});
    return args;
}

shared_ptr<AST::ReturnStatement> Parser::_parseReturnStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no  = current_token->col_no;
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken();
        int end_line_no       = current_token->line_no;
        int end_col_no        = current_token->col_no;
        auto return_statement = make_shared<AST::ReturnStatement>();
        return_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
        return return_statement;
    }
    this->_nextToken();
    auto expr = this->_parseExpression(PrecedenceType::LOWEST);
    if (this->_peekTokenIs(TokenType::Semicolon)) this->_nextToken();
    int end_line_no       = current_token->line_no;
    int end_col_no        = current_token->col_no;
    auto return_statement = make_shared<AST::ReturnStatement>(expr);
    return_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return return_statement;
}

shared_ptr<AST::RaiseStatement> Parser::_parseRaiseStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no  = current_token->col_no;
    this->_nextToken();
    auto expr = this->_parseExpression(PrecedenceType::LOWEST);
    if (this->_peekTokenIs(TokenType::Semicolon)) { this->_nextToken(); }
    int end_line_no       = current_token->line_no;
    int end_col_no        = current_token->col_no;
    auto return_statement = make_shared<AST::RaiseStatement>(expr);
    return_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return return_statement;
}

shared_ptr<AST::BlockStatement> Parser::_parseBlockStatement() {
    this->_nextToken();

    std::vector<StatementPtr> statements;
    while (!this->_currentTokenIs(TokenType::RightBrace) && !this->_currentTokenIs(TokenType::EndOfFile)) {
        auto stmt = this->_parseStatement();
        if (stmt) { statements.push_back(stmt); }
        this->_nextToken();
    }
    if (this->_peekTokenIs(TokenType::Semicolon)) this->_nextToken();

    auto block_statement = make_shared<AST::BlockStatement>(statements);
    return block_statement;
}

StatementPtr Parser::_parseExpressionStatement(ExpressionPtr identifier, int st_line_no, int st_col_no) {
    if (identifier == nullptr) {
        st_line_no = current_token->line_no;
        st_col_no  = current_token->col_no;
        identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
    }
    auto expr = this->_parseExpression(PrecedenceType::LOWEST, identifier, st_line_no, st_col_no);
    if (this->_peekTokenIs(TokenType::Equals)) return this->_parseVariableAssignment(expr, expr->meta_data.st_col_no, expr->meta_data.end_col_no);
    this->_expectPeek(TokenType::Semicolon);
    auto stmt       = make_shared<AST::ExpressionStatement>(expr);
    int end_line_no = current_token->line_no;
    int end_col_no  = current_token->col_no;
    stmt->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return stmt;
}

StatementPtr Parser::_parseVariableDeclaration(ExpressionPtr identifier, int st_line_no, int st_col_no, bool is_volatile) {
    if (!identifier) {
        st_line_no = current_token->line_no;
        st_col_no  = current_token->col_no;
        identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
    }
    this->_expectPeek({TokenType::Colon});
    this->_nextToken();
    auto type = this->_parseType();
    if (this->_peekTokenIs(TokenType::Semicolon)) {
        this->_nextToken();
        int end_line_no                   = current_token->line_no;
        int end_col_no                    = current_token->col_no;
        auto variableDeclarationStatement = make_shared<AST::VariableDeclarationStatement>(identifier, type, nullptr, is_volatile);
        variableDeclarationStatement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
        variableDeclarationStatement->meta_data.more_data["name_line_no"]    = st_line_no;
        variableDeclarationStatement->meta_data.more_data["name_col_no"]     = st_col_no;
        variableDeclarationStatement->meta_data.more_data["name_end_col_no"] = current_token->end_col_no;
        return variableDeclarationStatement;
    } else if (this->_expectPeek({TokenType::Equals, TokenType::Semicolon})) {
        this->_nextToken();
        auto expr = this->_parseExpression(PrecedenceType::LOWEST);
        this->_nextToken();
        int end_line_no                   = current_token->line_no;
        int end_col_no                    = current_token->col_no;
        auto variableDeclarationStatement = make_shared<AST::VariableDeclarationStatement>(identifier, type, expr, is_volatile);
        variableDeclarationStatement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
        variableDeclarationStatement->meta_data.more_data["name_line_no"]    = st_line_no;
        variableDeclarationStatement->meta_data.more_data["name_col_no"]     = st_col_no;
        variableDeclarationStatement->meta_data.more_data["name_end_col_no"] = current_token->end_col_no;
        return variableDeclarationStatement;
    }
    return nullptr;
}

shared_ptr<AST::TryCatchStatement> Parser::_parseTryCatchStatement() {
    this->_nextToken();
    auto try_block = this->_parseStatement();
    std::vector<std::tuple<shared_ptr<AST::Type>, shared_ptr<AST::IdentifierLiteral>, StatementPtr>> catch_blocks;

    while (this->_peekTokenIs(TokenType::Catch)) {
        this->_nextToken();
        this->_expectPeek({TokenType::LeftParen});
        this->_nextToken();
        auto exception_type = this->_parseType();
        this->_expectPeek({TokenType::Identifier});
        auto exception_var = make_shared<AST::IdentifierLiteral>(this->current_token);
        this->_expectPeek({TokenType::RightParen});
        this->_nextToken();
        auto catch_block = this->_parseStatement();
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
        std::cerr << "Cant parser infixIdentifier Expression cz: " << token::tokenTypeString(this->current_token->type) << std::endl;
        exit(1);
    }
    if (!this->_peekTokenIs(TokenType::Dot)) { return make_shared<AST::IdentifierLiteral>(this->current_token); }
    auto li = make_shared<AST::IdentifierLiteral>(this->current_token);
    this->_nextToken();
    this->_nextToken();
    return make_shared<AST::InfixExpression>(li, TokenType::Dot, ".", this->_parseInfixIdenifier());
}

shared_ptr<AST::Type> Parser::_parseType() {
    int st_line_no = current_token->line_no;
    int st_col_no  = current_token->col_no;
    ExpressionPtr name;
    name = this->_parseInfixIdenifier();
    std::vector<shared_ptr<AST::Type>> generics;
    bool ref = false;
    if (this->_peekTokenIs(TokenType::LeftBracket)) {
        this->_nextToken();
        this->_nextToken();
        while (this->current_token->type != TokenType::RightBracket) {
            auto generic = this->_parseType();
            generics.push_back(generic);
            this->_nextToken();
            if (this->current_token->type == TokenType::Comma) { this->_nextToken(); }
        }
    }
    if (this->_peekTokenIs(TokenType::Refrence)) {
        this->_nextToken();
        ref = true;
    }
    int end_line_no        = current_token->line_no;
    int end_col_no         = current_token->col_no;
    auto generic_type_node = make_shared<AST::Type>(name, generics, ref);
    generic_type_node->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return generic_type_node;
}

StatementPtr Parser::_parseVariableAssignment(ExpressionPtr identifier, int st_line_no, int st_col_no) {
    if (!identifier) {
        st_line_no = current_token->line_no;
        st_col_no  = current_token->col_no;
        identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
    }
    this->_expectPeek({TokenType::Equals});
    this->_nextToken();
    auto expr = this->_parseExpression(PrecedenceType::LOWEST);
    this->_nextToken();
    auto stmt       = make_shared<AST::VariableAssignmentStatement>(identifier, expr);
    int end_line_no = current_token->line_no;
    int end_col_no  = current_token->col_no;
    stmt->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return stmt;
}

shared_ptr<AST::StructStatement> Parser::_parseStructStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no  = current_token->col_no;

    this->_expectPeek({TokenType::Identifier});
    ExpressionPtr name = make_shared<AST::IdentifierLiteral>(this->current_token);

    this->_expectPeek({TokenType::LeftBrace});
    this->_nextToken();
    std::vector<StatementPtr> statements;

    while (!this->_currentTokenIs(TokenType::RightBrace) && !this->_currentTokenIs(TokenType::EndOfFile)) {
        if (this->_currentTokenIs(TokenType::Def)) {
            auto stmt = this->_parseFunctionStatement();
            if (stmt) statements.push_back(stmt);
        } else if (this->_currentTokenIs(TokenType::Identifier)) {
            auto stmt = this->_parseVariableDeclaration();
            if (stmt) statements.push_back(stmt);
        } else if (this->_currentTokenIs(TokenType::AtTheRate)) {
            auto stmt = this->_parseDeco();
            if (stmt) statements.push_back(stmt);
        } else {
            this->_currentTokenError(this->current_token->type, {TokenType::Identifier, TokenType::Def, TokenType::AtTheRate});
            return nullptr;
        }
        this->_nextToken();
    }
    if (this->peek_token->type == TokenType::Semicolon) { this->_nextToken(); }
    int end_line_no = current_token->line_no;
    int end_col_no  = current_token->col_no;

    auto struct_stmt = make_shared<AST::StructStatement>(name, statements);
    struct_stmt->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return struct_stmt;
}

ExpressionPtr Parser::_parseExpression(PrecedenceType precedence, ExpressionPtr parsed_expression, int st_line_no, int st_col_no) {
    if (parsed_expression == nullptr) {
        st_line_no = current_token->line_no;
        st_col_no  = current_token->col_no;
        auto iter  = prefix_parse_fns.find(current_token->type);
        if (iter == prefix_parse_fns.end()) {
            this->_noPrefixParseFnError(current_token->type);
            return nullptr;
        }
        auto prefix_fn    = iter->second;
        parsed_expression = prefix_fn();
    }
    while (!_peekTokenIs(TokenType::Semicolon) && precedence < _peekPrecedence()) {
        auto iter = infix_parse_Fns.find(peek_token->type);
        if (iter == infix_parse_Fns.end()) { return parsed_expression; }
        this->_nextToken();
        auto infix_fn     = iter->second;
        parsed_expression = infix_fn(parsed_expression);
    }
    int end_line_no = current_token->line_no;
    int end_col_no  = current_token->col_no;
    parsed_expression->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return parsed_expression;
}

StatementPtr Parser::_parseIfElseStatement() {
    int st_line_no = current_token->line_no;
    int st_col_no  = current_token->col_no;
    this->_expectPeek({TokenType::LeftParen});
    this->_nextToken();
    auto condition = this->_parseExpression(PrecedenceType::LOWEST);
    this->_expectPeek({TokenType::RightParen});
    this->_nextToken();
    auto consequence         = this->_parseStatement();
    StatementPtr alternative = nullptr;
    if (this->_peekTokenIs(TokenType::Else)) {
        this->_nextToken();
        this->_nextToken();
        alternative = this->_parseStatement();
    }
    int end_line_no        = current_token->line_no;
    int end_col_no         = current_token->col_no;
    auto if_else_statement = make_shared<AST::IfElseStatement>(condition, consequence, alternative);
    if_else_statement->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return if_else_statement;
}

ExpressionPtr Parser::_parseInfixExpression(ExpressionPtr leftNode) {
    int st_line_no                                         = leftNode->meta_data.st_line_no;
    int st_col_no                                          = leftNode->meta_data.st_col_no;
    auto infix_expr                                        = make_shared<AST::InfixExpression>(leftNode, this->current_token->type, this->current_token->literal);
    infix_expr->meta_data.more_data["operator_line_no"]    = this->current_token->line_no;
    infix_expr->meta_data.more_data["operator_st_col_no"]  = this->current_token->col_no;
    infix_expr->meta_data.more_data["operator_end_col_no"] = this->current_token->end_col_no;
    auto precedence                                        = this->_currentPrecedence();
    this->_nextToken();
    infix_expr->right = this->_parseExpression(precedence);
    int end_line_no   = infix_expr->right->meta_data.end_line_no;
    int end_col_no    = infix_expr->right->meta_data.end_col_no;
    infix_expr->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return infix_expr;
}

ExpressionPtr Parser::_parseIndexExpression(ExpressionPtr leftNode) {
    int st_line_no                                      = leftNode->meta_data.st_line_no;
    int st_col_no                                       = leftNode->meta_data.st_col_no;
    auto index_expr                                     = make_shared<AST::IndexExpression>(leftNode);
    index_expr->meta_data.more_data["index_line_no"]    = this->current_token->line_no;
    index_expr->meta_data.more_data["index_st_col_no"]  = this->current_token->col_no;
    index_expr->meta_data.more_data["index_end_col_no"] = this->current_token->end_col_no;
    this->_nextToken();
    index_expr->index = this->_parseExpression(PrecedenceType::INDEX);
    int end_line_no   = index_expr->index->meta_data.end_line_no;
    int end_col_no    = index_expr->index->meta_data.end_col_no;
    index_expr->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    this->_expectPeek({TokenType::RightBracket});
    return index_expr;
}

ExpressionPtr Parser::_parseGroupedExpression() {
    this->_nextToken();
    int st_line_no = this->current_token->line_no;
    int st_col_no  = this->current_token->col_no;
    auto expr      = this->_parseExpression(PrecedenceType::LOWEST);
    this->_expectPeek({TokenType::RightParen});
    int end_line_no = this->current_token->line_no;
    int end_col_no  = this->current_token->end_col_no;
    expr->set_meta_data(st_line_no, st_col_no, end_line_no, end_col_no);
    return expr;
}

ExpressionPtr Parser::_parseIntegerLiteral() {
    auto expr                   = make_shared<AST::IntegerLiteral>(std::stoll(current_token->literal));
    expr->meta_data.st_line_no  = current_token->line_no;
    expr->meta_data.st_col_no   = current_token->col_no;
    expr->meta_data.end_line_no = current_token->line_no;
    expr->meta_data.end_col_no  = current_token->end_col_no;
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
    this->_expectPeek({TokenType::Identifier, TokenType::LeftBracket});
    if (this->_currentTokenIs(TokenType::Identifier)) is_new_call = true;
    else if (this->_currentTokenIs(TokenType::LeftBracket)) is_new_arr = true;
    return this->_parseExpression(PrecedenceType::LOWEST);
};

ExpressionPtr Parser::_parseStringLiteral() {
    auto expr = make_shared<AST::StringLiteral>(current_token->literal);
    expr->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return expr;
}

void Parser::_nextToken() {
    current_token = peek_token;
    peek_token    = lexer->nextToken();
}

bool Parser::_currentTokenIs(TokenType type) { return current_token->type == type; }

bool Parser::_peekTokenIs(TokenType type) { return peek_token->type == type; }

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
    for (_nextToken(); !_currentTokenIs(TokenType::RightBracket); _nextToken()) {
        if (_currentTokenIs(TokenType::Comma)) { continue; }
        auto expr = _parseExpression(PrecedenceType::LOWEST);
        if (expr) { elements.push_back(expr); }
    }
    auto array = make_shared<AST::ArrayLiteral>(elements, is_new_arr);
    is_new_arr = false;
    array->set_meta_data(current_token->line_no, current_token->col_no, current_token->line_no, current_token->end_col_no);
    return array;
};

ExpressionPtr Parser::_parseIdentifier() {
    if (this->current_token->type != TokenType::Identifier) {
        std::cerr << this->current_token->literal + " is not Identifier" << std::endl;
        exit(1);
    }
    auto identifier = make_shared<AST::IdentifierLiteral>(this->current_token);
    // if (_peekTokenIs(TokenType::LeftParen)) {
    //     int st_line_no = this->current_token->line_no;
    //     int st_col_no  = this->current_token->col_no;
    //     return _parseFunctionCall(make_shared<AST::IdentifierLiteral>(this->current_token), st_line_no, st_col_no);
    // }
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

void Parser::_noPrefixParseFnError(TokenType type) { errors::NoPrefixParseFnError(this->lexer->source, *peek_token, "No prefix parse function for " + token::tokenTypeString(type)).raise(); }
