#include "./token.hpp"

#include <string>
#include <unordered_map>

std::string token::Token::toString(bool color) {
    // Define ANSI escape codes for colors
    const std::string colorReset = "\x1b[0m";
    const std::string colorRed = "\x1b[91m";
    const std::string colorYellow = "\x1b[93m";
    const std::string colorGreen = "\x1b[92m";
    const std::string colorBlue = "\x1b[94m";
    const std::string colorMagenta = "\x1b[95m";

    // Convert variables to strings
    std::string typeString = tokenTypeString(type);
    std::string literalString = literal;
    std::unordered_map<std::string, std::string> replacements = {{"\n", "\\$(n)"}, {"\t", "\\$(t)"}};

    // Replace special characters in literalString
    for(auto& replacement : replacements) {
        size_t pos = literalString.find(replacement.first);
        while(pos != std::string::npos) {
            literalString.replace(pos, 1, replacement.second);
            pos = literalString.find(replacement.first, pos + replacement.second.size());
        }
    }

    std::string lineNoString = std::to_string(line_no);
    std::string colNoString = std::to_string(col_no);
    std::string endColNoString = std::to_string(end_col_no);

    // Calculate padding for literals
    size_t literalPadding = literalString.length() >= 10 ? 0 : (10 - literalString.length()) / 2 + (literalString.length() % 2);
    std::string literalPaddingStr(literalPadding, ' ');

    // Apply padding to each field for alignment
    if(typeString.length() < 15)
        typeString += std::string(15 - typeString.length(), ' ');
    if(literalString.length() < 2)
        lineNoString += std::string(2 - lineNoString.length(), ' ');
    if(colNoString.length() < 2)
        colNoString += std::string(2 - colNoString.length(), ' ');
    if(endColNoString.length() < 2)
        endColNoString += std::string(2 - endColNoString.length(), ' ');

    // Construct the formatted string with colors
    return (color ? colorRed : "") + "[type: " + (color ? colorReset + colorBlue : "") + typeString + (color ? colorRed : "") + ", literal: " + (color ? colorGreen + "\"" + colorYellow : "") +
           literalPaddingStr + literalString + literalPaddingStr + (color ? colorGreen + "\"" : "") + ", line_no: " + (color ? colorReset + colorGreen : "") + lineNoString +
           (color ? colorReset : "") + ", col_no: " + (color ? colorReset + colorMagenta : "") + colNoString + (color ? colorReset : "") + ", end_col_no: " + (color ? colorReset + colorMagenta : "") +
           endColNoString + (color ? colorReset : "") + (color ? colorRed : "") + "]" + (color ? colorReset : "");
}

std::string token::tokenTypeString(TokenType type) {
    switch(type) {
    case TokenType::Identifier:
        return "Identifier";
    case TokenType::Integer:
        return "INT";
    case TokenType::Float:
        return "Float";
    case TokenType::PlusEqual:
        return "PlusEqual";
    case TokenType::DashEqual:
        return "DashEqual";
    case TokenType::AsteriskEqual:
        return "AsteriskEqual";
    case TokenType::PercentEqual:
        return "PercentEqual";
    case TokenType::CaretEqual:
        return "CaretEqual";
    case TokenType::ForwardSlashEqual:
        return "ForwardSlashEqual";
    case TokenType::BackwardSlashEqual:
        return "BackwardSlashEqual";
    case TokenType::Increment:
        return "Increment";
    case TokenType::Decrement:
        return "Decrement";
    case TokenType::Dot:
        return "Dot";
    case TokenType::Ellipsis:
        return "Ellipsis";
    case TokenType::Plus:
        return "Plus";
    case TokenType::Dash:
        return "Dash";
    case TokenType::Asterisk:
        return "Asterisk";
    case TokenType::AsteriskAsterisk:
        return "AsteriskAsterisk";
    case TokenType::Percent:
        return "Percent";
    case TokenType::ForwardSlash:
        return "ForwardSlash";
    case TokenType::BackwardSlash:
        return "BackwardSlash";
    case TokenType::LeftParen:
        return "LeftParen";
    case TokenType::RightParen:
        return "RightParen";
    case TokenType::LeftBrace:
        return "LeftBrace";
    case TokenType::RightBrace:
        return "RightBrace";
    case TokenType::LeftBracket:
        return "LeftBracket";
    case TokenType::RightBracket:
        return "RightBracket";
    case TokenType::Colon:
        return "Colon";
    case TokenType::Semicolon:
        return "Semicolon";
    case TokenType::RightArrow:
        return "RightArrow";
    case TokenType::Comma:
        return "Comma";
    case TokenType::AtTheRate:
        return "AtTheRate";
    case TokenType::Pipe:
        return "Pipe";
    case TokenType::Equals:
        return "Equals";
    case TokenType::Illegal:
        return "Illegal";
    case TokenType::EndOfFile:
        return "EndOfFile";
    case TokenType::Return:
        return "Return";
    case TokenType::GreaterThan:
        return "GreaterThan";
    case TokenType::LessThan:
        return "LessThan";
    case TokenType::GreaterThanOrEqual:
        return "GreaterThanOrEqual";
    case TokenType::LessThanOrEqual:
        return "LessThanOrEqual";
    case TokenType::EqualEqual:
        return "EqualEqual";
    case TokenType::NotEquals:
        return "NotEquals";
    case TokenType::BitwiseAnd:
        return "BitwiseAnd";
    case TokenType::BitwiseOr:
        return "BitwiseOr";
    case TokenType::BitwiseXor:
        return "BitwiseXor";
    case TokenType::BitwiseNot:
        return "BitwiseNot";
    case TokenType::LeftShift:
        return "LeftShift";
    case TokenType::RightShift:
        return "RightShift";
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
    case TokenType::Use:
        return "Use";
    // case TokenType::Maybe:
    //     return "Maybe";
    case TokenType::Import:
        return "Import";
    case TokenType::None:
        return "None";
    case TokenType::String:
        return "String";
    default:
        return "unknown token type(" + std::to_string(static_cast<int>(type)) + ")";
    }
}

void token::Token::print() { std::cout << toString() << std::endl; }
