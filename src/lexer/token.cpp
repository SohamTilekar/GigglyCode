#include "./token.hpp"

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <unordered_map>
#include <utility>

using namespace token;

std::string padString(const std::string& str, size_t len) {
    return (str.length() < len) ? str + std::string(len - str.length(), ' ') : str;
}

const std::string Token::toString(std::string source, bool color) const {
    const std::string color_reset = "\x1b[0m";
    const std::string color_red = "\x1b[91m";
    const std::string color_yellow = "\x1b[93m";
    const std::string color_green = "\x1b[92m";
    const std::string color_blue = "\x1b[94m";
    const std::string color_magenta = "\x1b[95m";

    std::string type_string = tokenTypeToString(type);
    std::string literal_string = this->getLiteral(source.c_str());

    // Escape newline and tab characters in the literal string
    [[unlikely]] for (const auto& [search, replace] : std::unordered_map<std::string, std::string>{
             {"\n", "\\$(n)"},
             {"\t", "\\$(t)"}}) {
        size_t pos = 0;
        [[likely]]while ((pos = literal_string.find(search, pos)) != std::string::npos) {
            literal_string.replace(pos, search.length(), replace);
            pos += replace.length();
        }
    }

    std::string st_line_no_string = std::to_string(getStLineNo(source.c_str()));
    std::string end_line_no_string = std::to_string(getEnLineNo(source.c_str()));
    std::string col_no_string = std::to_string(getStColNo(source.c_str()));
    std::string end_col_no_string = std::to_string(getEnColNo(source.c_str()));

    size_t literal_padding = (literal_string.length() < 10) ? (10 - literal_string.length()) / 2 + (literal_string.length() % 2) : 0;
    std::string padded_literal = std::string(literal_padding, ' ') + literal_string + std::string(literal_padding, ' ');

    // Pad strings to ensure consistent output formatting
    type_string = padString(type_string, 15);
    end_line_no_string = padString(end_line_no_string, 2);
    col_no_string = padString(col_no_string, 2);
    end_col_no_string = padString(end_col_no_string, 2);

    std::string output;
    if (color) {
        output = color_red + "[type: " + color_reset + color_blue + type_string + color_red + ", literal: " + color_green + "\"" + color_yellow + padded_literal + color_green + "\"" +
                 ", st_line_no: " + color_reset + color_green + st_line_no_string + color_reset + ", end_line_no: " + color_reset + color_green + end_line_no_string + color_reset +
                 ", col_no: " + color_reset + color_magenta + col_no_string + color_reset + ", end_col_no: " + color_reset + color_magenta + end_col_no_string + color_reset + color_red + "]" + color_reset;
    } else {
        output = "[type: " + type_string + ", literal: \"" + padded_literal + "\"" + ", st_line_no: " + st_line_no_string + ", end_line_no: " + end_line_no_string +
                 ", col_no: " + col_no_string + ", end_col_no: " + end_col_no_string + "]";
    }
    return output;
}

const std::string token::tokenTypeToString(TokenType type) {
    // Convert Token to string for Debuging
    static const std::unordered_map<TokenType, std::string> tokenTypeToStringMap = {
        {TokenType::Identifier, "Identifier"},
        {TokenType::Integer, "INT"},
        {TokenType::Float, "Float"},
        {TokenType::PlusEqual, "PlusEqual(`+=`)"},
        {TokenType::MinusEqual, "DashEqual(`-=`)"},
        {TokenType::AsteriskEqual, "AsteriskEqual(`*=`)"},
        {TokenType::ModuloEqual, "PercentEqual(`%=`)"},
        {TokenType::CaretEqual, "CaretEqual(`^=`)"},
        {TokenType::ForwardSlashEqual, "ForwardSlashEqual(`/=`)"},
        {TokenType::BackSlashEqual, "BackSlashEqual(`\\=`)"},
        {TokenType::Increment, "Increment(`++`)"},
        {TokenType::Decrement, "Decrement(`--`)"},
        {TokenType::Dot, "Dot(`.`)"},
        {TokenType::Ellipsis, "Ellipsis(`...`)"},
        {TokenType::Plus, "Plus(`+`)"},
        {TokenType::Minus, "Minus(`-`)"},
        {TokenType::Asterisk, "Asterisk(`*`)"},
        {TokenType::Exponent, "Exponent(`**`)"},
        {TokenType::Modulo, "Modulo(`%`)"},
        {TokenType::ForwardSlash, "ForwardSlash(`/`)"},
        {TokenType::BackSlash, "BackSlash(`\\`)"},
        {TokenType::LeftParen, "LeftParen(`(`)"},
        {TokenType::RightParen, "RightParen(`)`)"},
        {TokenType::LeftBrace, "LeftBrace(`{`)"},
        {TokenType::RightBrace, "RightBrace(`}`)"},
        {TokenType::LeftBracket, "LeftBracket(`[`)"},
        {TokenType::RightBracket, "RightBracket(`]`)"},
        {TokenType::Colon, "Colon(`:`)"},
        {TokenType::Semicolon, "Semicolon(`;`)"},
        {TokenType::RightArrow, "RightArrow(`->`)"},
        {TokenType::Comma, "Comma(`,`)"},
        {TokenType::At, "At(`@`)"},
        {TokenType::Pipe, "Pipe(`|`)"},
        {TokenType::Equals, "Equals(`=`)"},
        {TokenType::Illegal, "Illegal"},
        {TokenType::EndOfFile, "EndOfFile(`file ended`)"},
        {TokenType::Return, "Return"},
        {TokenType::GreaterThan, "GreaterThan(`>`)"},
        {TokenType::LessThan, "LessThan(`<`)"},
        {TokenType::GreaterThanOrEqual, "GreaterThanOrEqual(`>=`)"},
        {TokenType::LessThanOrEqual, "LessThanOrEqual(`<=`)"},
        {TokenType::EqualEqual, "EqualEqual(`==`)"},
        {TokenType::NotEquals, "NotEquals(`!=`)"},
        {TokenType::BitwiseAnd, "BitwiseAnd(`&&`)"},
        {TokenType::BitwiseOr, "BitwiseOr(`||   `)"},
        {TokenType::BitwiseXor, "BitwiseXor(`^`)"},
        {TokenType::BitwiseNot, "BitwiseNot(`~`)"},
        {TokenType::LeftShift, "LeftShift(`<<`)"},
        {TokenType::RightShift, "RightShift(`>>`)"},
        {TokenType::And, "And"},
        {TokenType::Or, "Or"},
        {TokenType::Not, "Not"},
        {TokenType::Def, "Def"},
        {TokenType::If, "If"},
        {TokenType::Else, "Else"},
        {TokenType::ElIf, "ElIf"},
        {TokenType::Is, "Is"},
        {TokenType::While, "While"},
        {TokenType::For, "For"},
        {TokenType::In, "In"},
        {TokenType::Break, "Break"},
        {TokenType::Continue, "Continue"},
        {TokenType::Struct, "Struct"},
        {TokenType::True, "True"},
        {TokenType::False, "False"},
        {TokenType::Enum, "Enum"},
        {TokenType::Volatile, "Volatile"},
        {TokenType::Const, "Const"},
        {TokenType::Use, "Use"},
        {TokenType::Import, "Import"},
        {TokenType::As, "As"},
        {TokenType::None, "None"},
        {TokenType::StringDSQ, "StringDSQ"},
        {TokenType::StringSSQ, "StringSSQ"},
        {TokenType::StringDTQ, "StringDTQ"},
        {TokenType::StringSTQ, "StringSTQ"},
        {TokenType::New, "New"},
        {TokenType::Try, "Try"},
        {TokenType::Catch, "Catch"},
        {TokenType::Raise, "Raise"},
        {TokenType::NotBreak, "NotBreak"},
        {TokenType::IfBreak, "IfBreak"},
        {TokenType::Switch, "Switch"},
        {TokenType::Case, "Case"},
        {TokenType::Other, "Other"},
        {TokenType::Comment, "Comment"}
    };

    auto it = tokenTypeToStringMap.find(type);
    if (it != tokenTypeToStringMap.end()) {
        return it->second;
    } else {
        return "unknown token type(" + std::to_string(static_cast<int>(type)) + ")";
    }
}

uint32_t Token::getStLineNo(const char* source) const {
    uint32_t line_no = 1;
    uint32_t pos = 0;
    [[likely]] while (pos < this->pos && pos < strlen(source)) {
        [[unlikely]] if (source[pos] == '\n') { line_no++; }
        pos++;
    }
    return line_no;
}

uint32_t Token::getEnLineNo(const char* source) const {
    uint32_t line_no = 1;
    uint32_t pos = 0;
    [[likely]] while (pos < this->getEnPos(source) && pos < strlen(source)) {
        [[unlikely]] if (source[pos] == '\n') { line_no++; }
        pos++;
    }
    return line_no;
}

uint32_t Token::getStColNo(const char* source) const {
    uint32_t line_no = 1;
    uint32_t col_no = 1;
    uint32_t pos = 0;
    [[likely]] while (pos < this->pos && pos < strlen(source)) {
        [[unlikely]] if (source[pos] == '\n') {
            line_no++;
            col_no = 1;
        } else {
            col_no++;
        }
        pos++;
    }
    return col_no;
}

uint32_t Token::getEnColNo(const char* source) const {
    uint32_t line_no = 1;
    uint32_t col_no = 1;
    uint32_t pos = 0;
    [[likely]] while (pos < this->getEnPos(source) && pos < strlen(source)) {
        [[unlikely]] if (source[pos] == '\n') {
            line_no++;
            col_no = 1;
        } else {
            col_no++;
        }
        pos++;
    }
    return col_no;
}

uint32_t Token::getEnPos(const char* source) const {
    static const std::unordered_map<TokenType, uint32_t> tokenTypeToLengthMap = {
        {TokenType::EndOfFile, 0},
        {TokenType::Equals, 1},
        {TokenType::GreaterThan, 1},
        {TokenType::LessThan, 1},
        {TokenType::Dot, 1},
        {TokenType::Plus, 1},
        {TokenType::Minus, 1},
        {TokenType::Asterisk, 1},
        {TokenType::Modulo, 1},
        {TokenType::Ampersand, 1},
        {TokenType::LeftParen, 1},
        {TokenType::RightParen, 1},
        {TokenType::LeftBrace, 1},
        {TokenType::RightBrace, 1},
        {TokenType::LeftBracket, 1},
        {TokenType::RightBracket, 1},
        {TokenType::Colon, 1},
        {TokenType::Comma, 1},
        {TokenType::At, 1},
        {TokenType::Pipe, 1},
        {TokenType::Semicolon, 1},
        {TokenType::GreaterThanOrEqual, 2},
        {TokenType::LessThanOrEqual, 2},
        {TokenType::EqualEqual, 2},
        {TokenType::NotEquals, 2},
        {TokenType::PlusEqual, 2},
        {TokenType::MinusEqual, 2},
        {TokenType::AsteriskEqual, 2},
        {TokenType::ModuloEqual, 2},
        {TokenType::CaretEqual, 2},
        {TokenType::ForwardSlashEqual, 2},
        {TokenType::BackSlashEqual, 2},
        {TokenType::Is, 2},
        {TokenType::Increment, 2},
        {TokenType::Decrement, 2},
        {TokenType::BitwiseAnd, 2},
        {TokenType::BitwiseOr, 2},
        {TokenType::BitwiseXor, 2},
        {TokenType::BitwiseNot, 2},
        {TokenType::LeftShift, 2},
        {TokenType::RightShift, 2},
        {TokenType::Exponent, 2},
        {TokenType::ForwardSlash, 2},
        {TokenType::BackSlash, 2},
        {TokenType::RightArrow, 2},
        {TokenType::Or, 2},
        {TokenType::If, 2},
        {TokenType::In, 2},
        {TokenType::As, 2},
        {TokenType::Ellipsis, 3},
        {TokenType::And, 3},
        {TokenType::Not, 3},
        {TokenType::Def, 3},
        {TokenType::New, 3},
        {TokenType::Try, 3},
        {TokenType::Use, 3},
        {TokenType::For, 3},
        {TokenType::Else, 4},
        {TokenType::ElIf, 4},
        {TokenType::Enum, 4},
        {TokenType::True, 4},
        {TokenType::None, 4},
        {TokenType::Case, 4},
        {TokenType::Other, 5},
        {TokenType::Catch, 5},
        {TokenType::Raise, 5},
        {TokenType::False, 5},
        {TokenType::While, 5},
        {TokenType::Break, 5},
        {TokenType::Const, 5},
        {TokenType::Struct, 6},
        {TokenType::Import, 6},
        {TokenType::Return, 6},
        {TokenType::Switch, 6},
        {TokenType::IfBreak, 7},
        {TokenType::Continue, 8},
        {TokenType::Volatile, 8},
        {TokenType::NotBreak, 8},
        {TokenType::StringDSQ, 0},
        {TokenType::StringSSQ, 0},
        {TokenType::StringDTQ, 0},
        {TokenType::StringSTQ, 0},
        {TokenType::Integer, 0},
        {TokenType::Float, 0},
        {TokenType::Identifier, 0},
        {TokenType::Illegal, 0},
        {TokenType::Comment, 0}
    };

    auto it = tokenTypeToLengthMap.find(this->type);
    if (it != tokenTypeToLengthMap.end()) {
        if (it->second == 0) {
            return this->pos + this->getLiteral(source).length();
        } else {
            return this->pos + it->second;
        }
    } else {
        return this->pos; // Handle unknown token types
    }
}

std::string handleEscapeSequences(const char* source, size_t pos, StringDelimiter delimiter) {
    std::string literal;
    char endChar;
    int endCount;

    switch (delimiter) {
        case StringDelimiter::DoubleQuote:
            endChar = '"';
            endCount = 1;
            break;
        case StringDelimiter::SingleQuote:
            endChar = '\'';
            endCount = 1;
            break;
        case StringDelimiter::TripleDoubleQuote:
            endChar = '"';
            endCount = 3;
            break;
        case StringDelimiter::TripleSingleQuote:
            endChar = '\'';
            endCount = 3;
            break;
        case StringDelimiter::None:
            std::unreachable();
    }
    auto s_len = strlen(source);
    [[likely]] for (size_t i = pos; i < s_len; ++i) {
        [[unlikely]] if (source[i] == '\\') {
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
                case 'x': {
                    std::string hex_str = "";
                    i++;
                    hex_str += source[i];
                    i++;
                    hex_str += source[i];
                    try {
                        char char_val = static_cast<char>(std::stoul(hex_str, nullptr, 16));
                        literal += char_val;
                    } catch (const std::invalid_argument& e) {
                        literal += "\\x" + hex_str;
                    }
                    break;
                }
                case 'u': {
                    std::string hex_str = "";
                    for (int j = 0; j < 4; ++j) {
                        i++;
                        if (i >= strlen(source) || !isxdigit(source[i])) {
                            literal += "\\u";
                            break;
                        }
                        hex_str += source[i];
                    }
                    try {
                        char32_t char_val = std::stoul(hex_str, nullptr, 16);
                        literal += char_val;
                    } catch (const std::invalid_argument& e) {
                        literal += "\\u" + hex_str;
                    }
                    break;
                }
                case 'U': {
                    std::string hex_str = "";
                    for (int j = 0; j < 8; ++j) {
                        i++;
                        if (i >= strlen(source) || !isxdigit(source[i])) {
                            //Handle error - incomplete unicode escape sequence
                            literal += "\\U";
                            break;
                        }
                        hex_str += source[i];
                    }
                    try {
                        char32_t char_val = std::stoul(hex_str, nullptr, 16);
                        literal += char_val;
                    } catch (const std::invalid_argument& e) {
                        literal += "\\U" + hex_str;
                    }
                    break;
                }
                default:
                    literal += "\\" + std::string(1, source[i]);
                    break;
            }
        } else [[unlikely]] if (source[i] == endChar) {
            int count = 0;
            [[unlikely]] while (source[i] == endChar && i < strlen(source)) {
                count++;
                i++;
            }
            [[unlikely]] if (count == endCount) {
                i--; // Adjust index to prevent skipping characters
                break;
            } else {
                literal += endChar;
            }
        } else {
            literal += source[i];
        }
    }
    return literal;
}

const std::string Token::getLiteral(const char* source) const {
    static const std::unordered_map<TokenType, std::string> tokenLiteralMap = {
        {TokenType::Illegal, ""},
        {TokenType::EndOfFile, ""},
        {TokenType::Equals, "="},
        {TokenType::GreaterThan, ">"},
        {TokenType::LessThan, "<"},
        {TokenType::Dot, "."},
        {TokenType::Plus, "+"},
        {TokenType::Minus, "-"},
        {TokenType::Asterisk, "*"},
        {TokenType::Modulo, "%"},
        {TokenType::Ampersand, "&"},
        {TokenType::LeftParen, "("},
        {TokenType::RightParen, ")"},
        {TokenType::LeftBrace, "{"},
        {TokenType::RightBrace, "}"},
        {TokenType::LeftBracket, "["},
        {TokenType::RightBracket, "]"},
        {TokenType::Colon, ":"},
        {TokenType::Comma, ","},
        {TokenType::At, "@"},
        {TokenType::Pipe, "|"},
        {TokenType::Semicolon, ";"},
        {TokenType::GreaterThanOrEqual, ">="},
        {TokenType::LessThanOrEqual, "<="},
        {TokenType::EqualEqual, "=="},
        {TokenType::NotEquals, "!="},
        {TokenType::PlusEqual, "+="},
        {TokenType::MinusEqual, "-="},
        {TokenType::AsteriskEqual, "*="},
        {TokenType::ModuloEqual, "%="},
        {TokenType::CaretEqual, "^="},
        {TokenType::ForwardSlashEqual, "/="},
        {TokenType::BackSlashEqual, "\\="},
        {TokenType::Is, "is"},
        {TokenType::Increment, "++"},
        {TokenType::Decrement, "--"},
        {TokenType::BitwiseAnd, "&&"},
        {TokenType::BitwiseOr, "||"},
        {TokenType::BitwiseXor, "^"},
        {TokenType::BitwiseNot, "~"},
        {TokenType::LeftShift, "<<"},
        {TokenType::RightShift, ">>"},
        {TokenType::Exponent, "**"},
        {TokenType::ForwardSlash, "/"},
        {TokenType::BackSlash, "\\"},
        {TokenType::RightArrow, "->"},
        {TokenType::Or, "or"},
        {TokenType::If, "if"},
        {TokenType::In, "in"},
        {TokenType::As, "as"},
        {TokenType::Ellipsis, "..."},
        {TokenType::And, "and"},
        {TokenType::Not, "not"},
        {TokenType::Def, "def"},
        {TokenType::New, "new"},
        {TokenType::Try, "try"},
        {TokenType::Use, "use"},
        {TokenType::For, "for"},
        {TokenType::Else, "else"},
        {TokenType::ElIf, "elif"},
        {TokenType::Enum, "enum"},
        {TokenType::True, "true"},
        {TokenType::None, "none"},
        {TokenType::Case, "case"},
        {TokenType::Other, "other"},
        {TokenType::Catch, "catch"},
        {TokenType::Raise, "raise"},
        {TokenType::False, "false"},
        {TokenType::While, "while"},
        {TokenType::Break, "break"},
        {TokenType::Const, "const"},
        {TokenType::Struct, "struct"},
        {TokenType::Import, "import"},
        {TokenType::Return, "return"},
        {TokenType::Switch, "switch"},
        {TokenType::IfBreak, "ifbreak"},
        {TokenType::Continue, "continue"},
        {TokenType::Volatile, "volatile"},
        {TokenType::NotBreak, "notbreak"}
    };

    auto it = tokenLiteralMap.find(this->type);
    [[likely]] if (it != tokenLiteralMap.end()) {
        return it->second;
    } else {
        [[unlikely]] if (this->type == TokenType::StringDSQ) return handleEscapeSequences(source, this->pos, StringDelimiter::DoubleQuote);
        [[unlikely]] if (this->type == TokenType::StringSSQ) return handleEscapeSequences(source, this->pos, StringDelimiter::SingleQuote);
        [[unlikely]] if (this->type == TokenType::StringDTQ) return handleEscapeSequences(source, this->pos, StringDelimiter::TripleDoubleQuote);
        [[unlikely]] if (this->type == TokenType::StringSTQ) return handleEscapeSequences(source, this->pos, StringDelimiter::TripleSingleQuote);
        [[unlikely]] if (this->type == TokenType::Integer) {
            std::string literal;
            [[likely]] for (size_t i = this->pos; i < strlen(source); ++i) {
                [[unlikely]] if (!_isDigit(source[i])) break;
                literal += source[i];
            }
            return literal;
        }
        [[unlikely]] if (this->type == TokenType::Float) {
            std::string literal;
            [[likely]] for (size_t i = this->pos; i < strlen(source); ++i) {
                [[unlikely]] if (!_isDigit(source[i]) && source[i] != '.') break;
                literal += source[i];
            }
            return literal;
        }
        [[unlikely]] if (this->type == TokenType::Identifier) {
            std::string literal;
            [[likely]] for (size_t i = this->pos; i < strlen(source); ++i) {
                [[unlikely]] if (!_isLetter(source[i]) && source[i] != '_') break;
                literal += source[i];
            }
            return literal;
        }
        [[unlikely]] if (this->type == TokenType::Comment) {
            std::string literal;
            [[likely]] for (size_t i = this->pos; i < strlen(source); ++i) {
                [[unlikely]] if (source[i] == '\n') break;
                literal += source[i];
            }
            return literal;
        }
        std::unreachable();
    }
}

void Token::print(std::string source) const {
    std::cout << toString(source) << std::endl;
}

Token Tokens::nextToken() {
    if (!tokenBuffer.empty()) {
        auto tok = tokenBuffer.back();
        tokenBuffer.pop_back();
        return tok;
    }
    [[unlikely]] if (currentTokenIDX >= this->tokens.size())
        return Token(TokenType::EndOfFile, strlen(source));
    auto tok = this->tokens[currentTokenIDX];
    currentTokenIDX++;
    return tok;
}
