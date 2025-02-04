#include "./token.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unordered_map>

std::string token::Token::toString(std::string source, bool color) const {
    // Define ANSI escape codes for colors
    const std::string colorReset = "\x1b[0m";
    const std::string colorRed = "\x1b[91m";
    const std::string colorYellow = "\x1b[93m";
    const std::string colorGreen = "\x1b[92m";
    const std::string colorBlue = "\x1b[94m";
    const std::string colorMagenta = "\x1b[95m";

    // Convert variables to strings
    std::string typeString = tokenTypeToString(type);
    std::string literalString = this->getLiteral(source.c_str());
    std::unordered_map<std::string, std::string> replacements = {{"\n", "\\$(n)"}, {"\t", "\\$(t)"}};

    // Replace special characters in literalString
    for (const auto& replacement : replacements) {
        size_t pos = 0;
        while ((pos = literalString.find(replacement.first, pos)) != std::string::npos) {
            literalString.replace(pos, replacement.first.length(), replacement.second);
            pos += replacement.second.length();
        }
    }

    // Convert line and column numbers to strings
    std::string stLineNoString = std::to_string(this->getStLineNo(source.c_str()));
    std::string endLineNoString = std::to_string(this->getEnLineNo(source.c_str()));
    std::string colNoString = std::to_string(this->getStColNo(source.c_str()));
    std::string endColNoString = std::to_string(this->getEnColNo(source.c_str()));

    // Calculate padding for literals
    size_t literalPadding = literalString.length() >= 10 ? 0 : (10 - literalString.length()) / 2 + (literalString.length() % 2);
    std::string literalPaddingStr(literalPadding, ' ');

    // Apply padding to each field for alignment
    if (typeString.length() < 15) typeString += std::string(15 - typeString.length(), ' ');
    if (literalString.length() < 2) endLineNoString += std::string(2 - literalString.length(), ' ');
    if (colNoString.length() < 2) colNoString += std::string(2 - colNoString.length(), ' ');
    if (endColNoString.length() < 2) endColNoString += std::string(2 - endColNoString.length(), ' ');

    // Construct the formatted string with or without colors
    if (color) {
        // Construct the formatted string with colors
        return colorRed + "[type: " + colorReset + colorBlue + typeString + colorRed + ", literal: " + colorGreen + "\"" + colorYellow + literalPaddingStr + literalString + literalPaddingStr +
            colorGreen + "\"" + ", st_line_no: " + colorReset + colorGreen + stLineNoString + colorReset + ", end_line_no: " + colorReset + colorGreen + endLineNoString + colorReset +
            ", col_no: " + colorReset + colorMagenta + colNoString + colorReset + ", end_col_no: " + colorReset + colorMagenta + endColNoString + colorReset + colorRed + "]" + colorReset;
    } else {
        // Construct the formatted string without colors
        return "[type: " + typeString + ", literal: \"" + literalPaddingStr + literalString + literalPaddingStr + "\"" + ", st_line_no: " + stLineNoString + ", end_line_no: " + endLineNoString +
            ", col_no: " + colNoString + ", end_col_no: " + endColNoString + "]";
    }
}

std::string token::tokenTypeToString(TokenType type) {
    // Convert Token to string for Debuging
    switch (type) {
        case TokenType::Identifier:
            return "Identifier";
        case TokenType::Integer:
            return "INT";
        case TokenType::Float:
            return "Float";
        case TokenType::PlusEqual:
            return "PlusEqual(`+=`)";
        case TokenType::DashEqual:
            return "DashEqual(`-=`)";
        case TokenType::AsteriskEqual:
            return "AsteriskEqual(`*=`)";
        case TokenType::PercentEqual:
            return "PercentEqual(`%=`)";
        case TokenType::CaretEqual:
            return "CaretEqual(`^=`)";
        case TokenType::ForwardSlashEqual:
            return "ForwardSlashEqual(`/=`)";
        case TokenType::BackwardSlashEqual:
            return "BackwardSlashEqual(`\\=`)";
        case TokenType::Increment:
            return "Increment(`++`)";
        case TokenType::Decrement:
            return "Decrement(`--`)";
        case TokenType::Dot:
            return "Dot(`.`)";
        case TokenType::Ellipsis:
            return "Ellipsis(`...`)";
        case TokenType::Plus:
            return "Plus(`+`)";
        case TokenType::Dash:
            return "Dash(`-`)";
        case TokenType::Asterisk:
            return "Asterisk(`*`)";
        case TokenType::AsteriskAsterisk:
            return "AsteriskAsterisk(`**`)";
        case TokenType::Percent:
            return "Percent(`%`)";
        case TokenType::ForwardSlash:
            return "ForwardSlash(`/`)";
        case TokenType::BackwardSlash:
            return "BackwardSlash(`\\`)";
        case TokenType::LeftParen:
            return "LeftParen(`(`)";
        case TokenType::RightParen:
            return "RightParen(`)`)";
        case TokenType::LeftBrace:
            return "LeftBrace(`{`)";
        case TokenType::RightBrace:
            return "RightBrace(`}`)";
        case TokenType::LeftBracket:
            return "LeftBracket(`[`)";
        case TokenType::RightBracket:
            return "RightBracket(`]`)";
        case TokenType::Colon:
            return "Colon(`:`)";
        case TokenType::Semicolon:
            return "Semicolon(`;`)";
        case TokenType::RightArrow:
            return "RightArrow(`->`)";
        case TokenType::Comma:
            return "Comma(`,`)";
        case TokenType::AtTheRate:
            return "AtTheRate(`@`)";
        case TokenType::Pipe:
            return "Pipe(`|`)";
        case TokenType::Equals:
            return "Equals(`=`)";
        case TokenType::Illegal:
            return "Illegal";
        case TokenType::EndOfFile:
            return "EndOfFile(`file ended`)";
        case TokenType::Return:
            return "Return";
        case TokenType::GreaterThan:
            return "GreaterThan(`>`)";
        case TokenType::LessThan:
            return "LessThan(`<`)";
        case TokenType::GreaterThanOrEqual:
            return "GreaterThanOrEqual(`>=`)";
        case TokenType::LessThanOrEqual:
            return "LessThanOrEqual(`<=`)";
        case TokenType::EqualEqual:
            return "EqualEqual(`==`)";
        case TokenType::NotEquals:
            return "NotEquals(`!=`)";
        case TokenType::BitwiseAnd:
            return "BitwiseAnd(`&&`)";
        case TokenType::BitwiseOr:
            return "BitwiseOr(`||   `)";
        case TokenType::BitwiseXor:
            return "BitwiseXor(`^`)";
        case TokenType::BitwiseNot:
            return "BitwiseNot(`~`)";
        case TokenType::LeftShift:
            return "LeftShift(`<<`)";
        case TokenType::RightShift:
            return "RightShift(`>>`)";
        case TokenType::And:
            return "And";
        case TokenType::Or:
            return "Or";
        case TokenType::Not:
            return "Not";
        case TokenType::Def:
            return "Def";
        case TokenType::If:
            return "If";
        case TokenType::Else:
            return "Else";
        case TokenType::ElIf:
            return "ElIf";
        case TokenType::Is:
            return "Is";
        case TokenType::While:
            return "While";
        case TokenType::For:
            return "For";
        case TokenType::In:
            return "In";
        case TokenType::Break:
            return "Break";
        case TokenType::Continue:
            return "Continue";
        case TokenType::Struct:
            return "Struct";
        case TokenType::True:
            return "True";
        case TokenType::False:
            return "False";
        case TokenType::Enum:
            return "Enum";
        case TokenType::Volatile:
            return "Volatile";
        case TokenType::Const:
            return "Const";
        case TokenType::Use:
            return "Use";
        // case TokenType::Maybe:
        //     return "Maybe";
        case TokenType::Import:
            return "Import";
        case TokenType::As:
            return "As";
        case TokenType::None:
            return "None";
        case TokenType::StringDSQ:
            return "StringDSQ";
        case TokenType::StringSSQ:
            return "StringSSQ";
        case TokenType::StringDTQ:
            return "StringDTQ";
        case TokenType::StringSTQ:
            return "StringSTQ";
        case TokenType::New:
            return "New";
        case TokenType::Try:
            return "Try";
        case TokenType::Catch:
            return "Catch";
        case TokenType::Raise:
            return "Raise";
        case TokenType::NotBreak:
            return "NotBreak";
        case TokenType::IfBreak:
            return "IfBreak";
        case TokenType::Switch:
            return "Switch";
        case TokenType::Case:
            return "Case";
        case TokenType::Other:
            return "Other";
        case TokenType::Coment:
            return "Coment";
        default:
            return "unknown token type(" + std::to_string(static_cast<int>(type)) + ")";
    }
}

uint32_t token::Token::getStLineNo(const char* source) const {
    uint32_t line_no = 1;
    uint32_t pos = 0;
    while (pos < this->pos && pos < strlen(source)) {
        if (source[pos] == '\n') {
            line_no++;
        }
        pos++;
    }
    return line_no;
}

uint32_t token::Token::getEnLineNo(const char* source) const {
    uint32_t line_no = 1;
    uint32_t pos = 0;
    while (pos < this->getEnPos(source) && pos < strlen(source)) {
        if (source[pos] == '\n') {
            line_no++;
        }
        pos++;
    }
    return line_no;
}

uint32_t token::Token::getStColNo(const char* source) const {
    uint32_t line_no = 1;
    uint32_t col_no = 1;
    uint32_t pos = 0;
    while (pos < this->pos && pos < strlen(source)) {
        if (source[pos] == '\n') {
            line_no++;
            col_no = 1;
        } else {
            col_no++;
        }
        pos++;
    }
    return col_no;
}

uint32_t token::Token::getEnColNo(const char* source) const {
    uint32_t line_no = 1;
    uint32_t col_no = 1;
    uint32_t pos = 0;
    while (pos < this->getEnPos(source) && pos < strlen(source)) {
        if (source[pos] == '\n') {
            line_no++;
            col_no = 1;
        } else {
            col_no++;
        }
        pos++;
    }
    return col_no;
}

uint32_t token::Token::getEnPos(const char* source) const {
    switch (this->type) {
        case TokenType::EndOfFile:
            return this->pos;
        case TokenType::Equals:
        case TokenType::GreaterThan:
        case TokenType::LessThan:
        case TokenType::Dot:
        case TokenType::Plus:
        case TokenType::Dash:
        case TokenType::Asterisk:
        case TokenType::Percent:
        case TokenType::Refrence:
        case TokenType::LeftParen:
        case TokenType::RightParen:
        case TokenType::LeftBrace:
        case TokenType::RightBrace:
        case TokenType::LeftBracket:
        case TokenType::RightBracket:
        case TokenType::Colon:
        case TokenType::Comma:
        case TokenType::AtTheRate:
        case TokenType::Pipe:
        case TokenType::Semicolon:
            return this->pos + 1;
        case TokenType::GreaterThanOrEqual:
        case TokenType::LessThanOrEqual:
        case TokenType::EqualEqual:
        case TokenType::NotEquals:
        case TokenType::PlusEqual:
        case TokenType::DashEqual:
        case TokenType::AsteriskEqual:
        case TokenType::PercentEqual:
        case TokenType::CaretEqual:
        case TokenType::ForwardSlashEqual:
        case TokenType::BackwardSlashEqual:
        case TokenType::Is:
        case TokenType::Increment:
        case TokenType::Decrement:
        case TokenType::BitwiseAnd:
        case TokenType::BitwiseOr:
        case TokenType::BitwiseXor:
        case TokenType::BitwiseNot:
        case TokenType::LeftShift:
        case TokenType::RightShift:
        case TokenType::AsteriskAsterisk:
        case TokenType::ForwardSlash:
        case TokenType::BackwardSlash:
        case TokenType::RightArrow:
        case TokenType::Or:
        case TokenType::If:
        case TokenType::In:
        case TokenType::As:
            return this->pos + 2;
        case TokenType::Ellipsis:
        case TokenType::And:
        case TokenType::Not:
        case TokenType::Def:
        case TokenType::New:
        case TokenType::Try:
        case TokenType::Use:
        case TokenType::For:
            return this->pos + 3;
        case TokenType::Else:
        case TokenType::ElIf:
        case TokenType::Enum:
        case TokenType::True:
        case TokenType::None:
        case TokenType::Case:
            return this->pos + 4;
        case TokenType::Other:
        case TokenType::Catch:
        case TokenType::Raise:
        case TokenType::False:
        case TokenType::While:
        case TokenType::Break:
        case TokenType::Const:
            return this->pos + 5;
        case TokenType::Struct:
        case TokenType::Import:
        case TokenType::Return:
        case TokenType::Switch:
            return this->pos + 6;
        case TokenType::IfBreak:
            return this->pos + 7;
        case TokenType::Continue:
        case TokenType::Volatile:
        case TokenType::NotBreak:
            return this->pos + 8;
        case TokenType::StringDSQ:
        case TokenType::StringSSQ:
        case TokenType::StringDTQ:
        case TokenType::StringSTQ:
        case TokenType::Integer:
        case TokenType::Float:
        case TokenType::Identifier:
        case TokenType::Illegal:
        case TokenType::Coment:
            return this->pos + this->getLiteral(source).length();
    }
}

// Create helpwer functions first

const std::string token::Token::getLiteral(const char* source) const {
    switch (this->type) {
        case TokenType::EndOfFile:
            return "";
        case TokenType::Equals:
            return "=";
        case TokenType::GreaterThan:
            return ">";
        case TokenType::LessThan:
            return "<";
        case TokenType::Dot:
            return ".";
        case TokenType::Plus:
            return "+";
        case TokenType::Dash:
            return "-";
        case TokenType::Asterisk:
            return "*";
        case TokenType::Percent:
            return "%";
        case TokenType::Refrence:
            return "&";
        case TokenType::LeftParen:
            return "(";
        case TokenType::RightParen:
            return ")";
        case TokenType::LeftBrace:
            return "{";
        case TokenType::RightBrace:
            return "}";
        case TokenType::LeftBracket:
            return "[";
        case TokenType::RightBracket:
            return "]";
        case TokenType::Colon:
            return ":";
        case TokenType::Comma:
            return ",";
        case TokenType::AtTheRate:
            return "@";
        case TokenType::Pipe:
            return "|";
        case TokenType::Semicolon:
            return ";";
        case TokenType::GreaterThanOrEqual:
            return ">=";
        case TokenType::LessThanOrEqual:
            return "<=";
        case TokenType::EqualEqual:
            return "==";
        case TokenType::NotEquals:
            return "!=";
        case TokenType::PlusEqual:
            return "+=";
        case TokenType::DashEqual:
            return "-=";
        case TokenType::AsteriskEqual:
            return "*=";
        case TokenType::PercentEqual:
            return "%=";
        case TokenType::CaretEqual:
            return "^=";
        case TokenType::ForwardSlashEqual:
            return "/=";
        case TokenType::BackwardSlashEqual:
            return "\\=";
        case TokenType::Is:
            return "is";
        case TokenType::Increment:
            return "++";
        case TokenType::Decrement:
            return "--";
        case TokenType::BitwiseAnd:
            return "&&";
        case TokenType::BitwiseOr:
            return "||";
        case TokenType::BitwiseXor:
            return "^";
        case TokenType::BitwiseNot:
            return "~";
        case TokenType::LeftShift:
            return "<<";
        case TokenType::RightShift:
            return ">>";
        case TokenType::AsteriskAsterisk:
            return "**";
        case TokenType::ForwardSlash:
            return "/";
        case TokenType::BackwardSlash:
            return "\\";
        case TokenType::RightArrow:
            return "->";
        case TokenType::Or:
            return "or";
        case TokenType::If:
            return "if";
        case TokenType::In:
            return "in";
        case TokenType::As:
            return "as";
        case TokenType::Ellipsis:
            return "...";
        case TokenType::And:
            return "and";
        case TokenType::Not:
            return "not";
        case TokenType::Def:
            return "def";
        case TokenType::New:
            return "new";
        case TokenType::Try:
            return "try";
        case TokenType::Use:
            return "use";
        case TokenType::For:
            return "for";
        case TokenType::Else:
            return "else";
        case TokenType::ElIf:
            return "elif";
        case TokenType::Enum:
            return "enum";
        case TokenType::True:
            return "true";
        case TokenType::None:
            return "none";
        case TokenType::Case:
            return "case";
        case TokenType::Other:
            return "other";
        case TokenType::Catch:
            return "catch";
        case TokenType::Raise:
            return "raise";
        case TokenType::False:
            return "false";
        case TokenType::While:
            return "while";
        case TokenType::Break:
            return "break";
        case TokenType::Const:
            return "const";
        case TokenType::Struct:
            return "struct";
        case TokenType::Import:
            return "import";
        case TokenType::Return:
            return "return";
        case TokenType::Switch:
            return "switch";
        case TokenType::IfBreak:
            return "ifbreak";
        case TokenType::Continue:
            return "continue";
        case TokenType::Volatile:
            return "volatile";
        case TokenType::NotBreak:
            return "notbreak";
        case TokenType::StringDSQ: {
            std::string literal;
            for (size_t i = this->pos; i < strlen(source); ++i) {
                if (source[i] == '\\') {

                    i++;
                        switch (source[i]) {
                            case '"':
                                literal += "\"";
                                break;
                            case '\'':
                                literal += "'";
                                break;
                            case 'n':
                                literal += "\n";
                                break;
                            case 't':
                                literal += "\t";
                                break;
                            case 'r':
                                literal += "\r";
                                break;
                            case 'b':
                                literal += "\b";
                                break;
                            case 'f':
                                literal += "\f";
                                break;
                            case 'v':
                                literal += "\v";
                                break;
                            case '\\':
                                literal += "\\";
                                break;
                            case 'x': { // Hexadecimal escape sequence \xHH
                                std::string hex_str = "";
                                i++;
                                hex_str += source[i];
                                i++;
                                hex_str += source[i];
                                char char_val = static_cast<char>(std::stoul(hex_str, nullptr, 16));
                                literal += char_val;
                                break;
                            }
                            case 'u': // Unicode escape sequences (UTF-8 encoding) are more complex.
                                        // \uHHHH
                            case 'U': // \UHHHHHHHH
                                // For simplicity and consistency with how other unhandled escapes are
                                // dealt with, we treat these as literal characters for now
                                [[fallthrough]];
                            default: // If not a recognized escape sequence, treat literally.
                                literal += std::string("\\") + source[i];
                                break;
                        }
                } else if (source[i] == '"') {
                    break;
                } else {
                    literal += source[i];
                }
            }
            return literal;
        }
        case TokenType::StringSSQ: {
            std::string literal;
            for (size_t i = this->pos; i < strlen(source); ++i) {
                if (source[i] == '\\') {
                    i++;
                    switch (source[i]) {
                        case '"':
                            literal += "\"";
                            break;
                        case '\'':
                            literal += "'";
                            break;
                        case 'n':
                            literal += "\n";
                            break;
                        case 't':
                            literal += "\t";
                            break;
                        case 'r':
                            literal += "\r";
                            break;
                        case 'b':
                            literal += "\b";
                            break;
                        case 'f':
                            literal += "\f";
                            break;
                        case 'v':
                            literal += "\v";
                            break;
                        case '\\':
                            literal += "\\";
                            break;
                        case 'x': { // Hexadecimal escape sequence \xHH
                            std::string hex_str = "";
                            i++;
                            hex_str += source[i];
                            i++;
                            hex_str += source[i];
                            char char_val = static_cast<char>(std::stoul(hex_str, nullptr, 16));
                            literal += char_val;
                            break;
                        }
                        case 'u': // Unicode escape sequences (UTF-8 encoding) are more complex.
                                    // \uHHHH
                        case 'U': // \UHHHHHHHH
                            // For simplicity and consistency with how other unhandled escapes are
                            // dealt with, we treat these as literal characters for now
                            [[fallthrough]];
                        default: // If not a recognized escape sequence, treat literally.
                            literal += std::string("\\") + source[i];
                            break;
                    }
                } else if (source[i] == '\'') {
                    break;
                } else {
                    literal += source[i];
                }
            }
            return literal;
        }
        case TokenType::StringDTQ: {
            std::string literal;
            for (size_t i = this->pos; i < strlen(source); ++i) {
                if (source[i] == '\\') {

                    i++;
                        switch (source[i]) {
                            case '"':
                                literal += "\"";
                                break;
                            case '\'':
                                literal += "'";
                                break;
                            case 'n':
                                literal += "\n";
                                break;
                            case 't':
                                literal += "\t";
                                break;
                            case 'r':
                                literal += "\r";
                                break;
                            case 'b':
                                literal += "\b";
                                break;
                            case 'f':
                                literal += "\f";
                                break;
                            case 'v':
                                literal += "\v";
                                break;
                            case '\\':
                                literal += "\\";
                                break;
                            case 'x': { // Hexadecimal escape sequence \xHH
                                std::string hex_str = "";
                                i++;
                                hex_str += source[i];
                                i++;
                                hex_str += source[i];
                                char char_val = static_cast<char>(std::stoul(hex_str, nullptr, 16));
                                literal += char_val;
                                break;
                            }
                            case 'u': // Unicode escape sequences (UTF-8 encoding) are more complex.
                                        // \uHHHH
                            case 'U': // \UHHHHHHHH
                                // For simplicity and consistency with how other unhandled escapes are
                                // dealt with, we treat these as literal characters for now
                                [[fallthrough]];
                            default: // If not a recognized escape sequence, treat literally.
                                literal += std::string("\\") + source[i];
                                break;
                        }
                } else if (source[i] == '"') {
                    size_t count = 0;
                    while (source[i] == '"') {
                        count++;
                        i++;
                    }
                    if (count == 3) break;
                } else {
                    literal += source[i];
                }
            }
            return literal;
        }
        case TokenType::StringSTQ: {
            std::string literal;
            for (size_t i = this->pos; i < strlen(source); ++i) {
                if (source[i] == '\\') {

                    i++;
                        switch (source[i]) {
                            case '"':
                                literal += "\"";
                                break;
                            case '\'':
                                literal += "'";
                                break;
                            case 'n':
                                literal += "\n";
                                break;
                            case 't':
                                literal += "\t";
                                break;
                            case 'r':
                                literal += "\r";
                                break;
                            case 'b':
                                literal += "\b";
                                break;
                            case 'f':
                                literal += "\f";
                                break;
                            case 'v':
                                literal += "\v";
                                break;
                            case '\\':
                                literal += "\\";
                                break;
                            case 'x': { // Hexadecimal escape sequence \xHH
                                std::string hex_str = "";
                                i++;
                                hex_str += source[i];
                                i++;
                                hex_str += source[i];
                                char char_val = static_cast<char>(std::stoul(hex_str, nullptr, 16));
                                literal += char_val;
                                break;
                            }
                            case 'u': // Unicode escape sequences (UTF-8 encoding) are more complex.
                                        // \uHHHH
                            case 'U': // \UHHHHHHHH
                                // For simplicity and consistency with how other unhandled escapes are
                                // dealt with, we treat these as literal characters for now
                                [[fallthrough]];
                            default: // If not a recognized escape sequence, treat literally.
                                literal += std::string("\\") + source[i];
                                break;
                        }
                } else if (source[i] == '\'') {
                    size_t count = 0;
                    while (source[i] == '\'') {
                        count++;
                        i++;
                    }
                    if (count == 3) break;
                } else {
                    literal += source[i];
                }
            }
            return literal;
        }
        case TokenType::Integer: {
            std::string literal;
            for (size_t i = this->pos; i < strlen(source); ++i) {
                if (!_isDigit(source[i])) break;
                literal += source[i];
            }
            return literal;
        }
        case TokenType::Float: {
            std::string literal;
            for (size_t i = this->pos; i < strlen(source); ++i) {
                if (!_isDigit(source[i]) && source[i] != '.') break;
                literal += source[i];
            }
            return literal;
        }
        case TokenType::Identifier: {
            std::string literal;
            for (size_t i = this->pos; i < strlen(source); ++i) {
                if (!_isLetter(source[i]) && source[i] != '_') break;
                literal += source[i];
            }
            return literal;
        }
        case TokenType::Illegal:
            return "";
        case TokenType::Coment: {
            std::string literal;
            for (size_t i = this->pos; i < strlen(source); ++i) {
                if (source[i] == '\n') break;
                literal += source[i];
            }
            return literal;
        }
        default:
            return "";
    }
}

void token::Token::print(std::string source) const {
    std::cout << toString(source) << std::endl;
}
