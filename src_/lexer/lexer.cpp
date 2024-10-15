#include "lexer.hpp"
#include "../errors/errors.hpp"

Lexer::Lexer(const std::string& source) {
    this->source = source;
    this->pos = -1;
    this->line_no = 1;
    this->col_no = -1;
    this->current_char = "";
    this->_readChar();
}

token::TokenType Lexer::_lookupIdent(std::shared_ptr<std::string> ident) {
    if(*ident == "and") {
        return token::TokenType::And;
    } else if(*ident == "or") {
        return token::TokenType::Or;
    } else if(*ident == "not") {
        return token::TokenType::Not;
    } else if(*ident == "def") {
        return token::TokenType::Def;
    } else if(*ident == "return") {
        return token::TokenType::Return;
    } else if(*ident == "if") {
        return token::TokenType::If;
    } else if(*ident == "else") {
        return token::TokenType::Else;
    } else if(*ident == "elif") {
        return token::TokenType::ElIf;
    } else if(*ident == "is") {
        return token::TokenType::Is;
    } else if(*ident == "while") {
        return token::TokenType::While;
    } else if(*ident == "for") {
        return token::TokenType::For;
    } else if(*ident == "in") {
        return token::TokenType::In;
    } else if(*ident == "break") {
        return token::TokenType::Break;
    } else if(*ident == "continue") {
        return token::TokenType::Continue;
    } else if(*ident == "class") {
        return token::TokenType::Class;
    } else if(*ident == "True") {
        return token::TokenType::True;
    } else if(*ident == "False") {
        return token::TokenType::False;
        // } else if(*ident == "MayBe") {
        //     return token::TokenType::Maybe;
    } else if(*ident == "None") {
        return token::TokenType::None;
    }
    return token::TokenType::Identifier;
};

std::shared_ptr<token::Token> Lexer::nextToken() {
    std::shared_ptr<token::Token> token;

    this->_skipWhitespace();

    if(this->current_char == "+") {
        if(*this->_peekChar() == "+") {
            token = this->_newToken(token::TokenType::Increment, this->current_char + *this->_peekChar());
            this->_readChar();
        } else if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::PlusEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Plus, this->current_char);
        }
    } else if(this->current_char == ".") {
        if(*this->_peekChar() == "." && *this->_peekChar(2) == ".") {
            token = this->_newToken(token::TokenType::Ellipsis, this->current_char + *this->_peekChar() + *(this->_peekChar(2)));
            this->_readChar();
            this->_readChar();
        } else
            token = this->_newToken(token::TokenType::Dot, this->current_char);
    } else if(this->current_char == "-") {
        if(*this->_peekChar() == ">") {
            token = this->_newToken(token::TokenType::RightArrow, this->current_char + *this->_peekChar());
            this->_readChar();
        } else if(*this->_peekChar() == "-") {
            token = this->_newToken(token::TokenType::Decrement, this->current_char + *this->_peekChar());
            this->_readChar();
        } else if(this->_isDigit(*(this->_peekChar()))) {
            this->_readChar();
            token = this->_readNumber();
            token->literal = "-" + token->literal;
        } else if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::DashEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Dash, this->current_char);
        }
    } else if(this->current_char == "*") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::AsteriskEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else if(*this->_peekChar() == "*") {
            token = this->_newToken(token::TokenType::AsteriskAsterisk, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Asterisk, this->current_char);
        }
    } else if(this->current_char == "/") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::ForwardSlashEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::ForwardSlash, this->current_char);
        }
    } else if(this->current_char == "%") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::PercentEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Percent, this->current_char);
        }
    } else if(this->current_char == "^") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::CaretEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::BitwiseXor, this->current_char);
        }
    } else if(this->current_char == "=") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::EqualEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Equals, this->current_char);
        }
    } else if(this->current_char == ">") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::GreaterThanOrEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else if(*this->_peekChar() == ">") {
            token = this->_newToken(token::TokenType::RightShift, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::GreaterThan, this->current_char);
        }
    } else if(this->current_char == "<") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::LessThanOrEqual, this->current_char + *this->_peekChar());
            this->_readChar();
        } else if(*this->_peekChar() == "<") {
            token = this->_newToken(token::TokenType::LeftShift, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::LessThan, this->current_char);
        }
    } else if(this->current_char == "!") {
        if(*this->_peekChar() == "=") {
            token = this->_newToken(token::TokenType::NotEquals, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Illegal, this->current_char);
        }
    } else if(this->current_char == "{") {
        token = this->_newToken(token::TokenType::LeftBrace, this->current_char);
    } else if(this->current_char == "}") {
        token = this->_newToken(token::TokenType::RightBrace, this->current_char);
    } else if(this->current_char == "(") {
        token = this->_newToken(token::TokenType::LeftParen, this->current_char);
    } else if(this->current_char == ")") {
        token = this->_newToken(token::TokenType::RightParen, this->current_char);
    } else if(this->current_char == "[") {
        token = this->_newToken(token::TokenType::LeftBracket, this->current_char);
    } else if(this->current_char == "]") {
        token = this->_newToken(token::TokenType::RightBracket, this->current_char);
    } else if(this->current_char == ":") {
        token = this->_newToken(token::TokenType::Colon, this->current_char);
    } else if(this->current_char == ";") {
        token = this->_newToken(token::TokenType::Semicolon, this->current_char);
    } else if(this->current_char == "&") {
        if(*this->_peekChar() == "&") {
            token = this->_newToken(token::TokenType::BitwiseAnd, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Illegal, this->current_char);
        }
    } else if(this->current_char == "|") {
        if(*this->_peekChar() == "|") {
            token = this->_newToken(token::TokenType::BitwiseOr, this->current_char + *this->_peekChar());
            this->_readChar();
        } else {
            token = this->_newToken(token::TokenType::Illegal, this->current_char);
        }
    } else if(this->current_char == "~") {
        token = this->_newToken(token::TokenType::BitwiseNot, this->current_char);
    } else if(this->current_char == ",") {
        token = this->_newToken(token::TokenType::Comma, this->current_char);
    } else if(this->current_char == "") {
        token = this->_newToken(token::TokenType::EndOfFile, "");
    } else {
        if(*this->_isString() != "") {
            std::shared_ptr<std::string> str = this->_readString(*this->_isString());
            token = this->_newToken(token::TokenType::String, *str);
            return token;
        } else if(this->_isLetter(this->current_char)) {
            std::shared_ptr<std::string> ident = this->_readIdentifier();
            token = this->_newToken(this->_lookupIdent(ident), *ident);
            return token;
        } else if(this->_isDigit(this->current_char)) {
            token = this->_readNumber();
            return token;
        } else {
            token = this->_newToken(token::TokenType::Illegal, this->current_char);
        }
    }
    this->_readChar();
    return token;
}
