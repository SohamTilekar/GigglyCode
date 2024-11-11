#include "./token.hpp"
#include <memory>
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
    std::string typeString = *tokenTypeString(type);
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
    return (color ? colorRed : "") + "[type: " + (color ? colorReset + colorBlue : "") + typeString + (color ? colorRed : "") +
           ", literal: " + (color ? colorGreen + "\"" + colorYellow : "") + literalPaddingStr + literalString + literalPaddingStr +
           (color ? colorGreen + "\"" : "") + ", line_no: " + (color ? colorReset + colorGreen : "") + lineNoString + (color ? colorReset : "") +
           ", col_no: " + (color ? colorReset + colorMagenta : "") + colNoString + (color ? colorReset : "") +
           ", end_col_no: " + (color ? colorReset + colorMagenta : "") + endColNoString + (color ? colorReset : "") + (color ? colorRed : "") + "]" +
           (color ? colorReset : "");
}

std::shared_ptr<std::string> token::tokenTypeString(TokenType type) {
    switch(type) {
    case TokenType::Identifier:
        return std::make_shared<std::string>("Identifier");
    case TokenType::Integer:
        return std::make_shared<std::string>("INT");
    case TokenType::Float:
        return std::make_shared<std::string>("Float");
    case TokenType::PlusEqual:
        return std::make_shared<std::string>("PlusEqual");
    case TokenType::DashEqual:
        return std::make_shared<std::string>("DashEqual");
    case TokenType::AsteriskEqual:
        return std::make_shared<std::string>("AsteriskEqual");
    case TokenType::PercentEqual:
        return std::make_shared<std::string>("PercentEqual");
    case TokenType::CaretEqual:
        return std::make_shared<std::string>("CaretEqual");
    case TokenType::ForwardSlashEqual:
        return std::make_shared<std::string>("ForwardSlashEqual");
    case TokenType::BackwardSlashEqual:
        return std::make_shared<std::string>("BackwardSlashEqual");
    case TokenType::Increment:
        return std::make_shared<std::string>("Increment");
    case TokenType::Decrement:
        return std::make_shared<std::string>("Decrement");
    case TokenType::Dot:
        return std::make_shared<std::string>("Dot");
    case TokenType::Ellipsis:
        return std::make_shared<std::string>("Ellipsis");
    case TokenType::Plus:
        return std::make_shared<std::string>("Plus");
    case TokenType::Dash:
        return std::make_shared<std::string>("Dash");
    case TokenType::Asterisk:
        return std::make_shared<std::string>("Asterisk");
    case TokenType::AsteriskAsterisk:
        return std::make_shared<std::string>("AsteriskAsterisk");
    case TokenType::Percent:
        return std::make_shared<std::string>("Percent");
    case TokenType::ForwardSlash:
        return std::make_shared<std::string>("ForwardSlash");
    case TokenType::BackwardSlash:
        return std::make_shared<std::string>("BackwardSlash");
    case TokenType::LeftParen:
        return std::make_shared<std::string>("LeftParen");
    case TokenType::RightParen:
        return std::make_shared<std::string>("RightParen");
    case TokenType::LeftBrace:
        return std::make_shared<std::string>("LeftBrace");
    case TokenType::RightBrace:
        return std::make_shared<std::string>("RightBrace");
    case TokenType::LeftBracket:
        return std::make_shared<std::string>("LeftBracket");
    case TokenType::RightBracket:
        return std::make_shared<std::string>("RightBracket");
    case TokenType::Colon:
        return std::make_shared<std::string>("Colon");
    case TokenType::Semicolon:
        return std::make_shared<std::string>("Semicolon");
    case TokenType::RightArrow:
        return std::make_shared<std::string>("RightArrow");
    case TokenType::Comma:
        return std::make_shared<std::string>("Comma");
    case TokenType::Equals:
        return std::make_shared<std::string>("Equals");
    case TokenType::Illegal:
        return std::make_shared<std::string>("Illegal");
    case TokenType::EndOfFile:
        return std::make_shared<std::string>("EndOfFile");
    case TokenType::Return:
        return std::make_shared<std::string>("Return");
    case TokenType::GreaterThan:
        return std::make_shared<std::string>("GreaterThan");
    case TokenType::LessThan:
        return std::make_shared<std::string>("LessThan");
    case TokenType::GreaterThanOrEqual:
        return std::make_shared<std::string>("GreaterThanOrEqual");
    case TokenType::LessThanOrEqual:
        return std::make_shared<std::string>("LessThanOrEqual");
    case TokenType::EqualEqual:
        return std::make_shared<std::string>("EqualEqual");
    case TokenType::NotEquals:
        return std::make_shared<std::string>("NotEquals");
    case TokenType::BitwiseAnd:
        return std::make_shared<std::string>("BitwiseAnd");
    case TokenType::BitwiseOr:
        return std::make_shared<std::string>("BitwiseOr");
    case TokenType::BitwiseXor:
        return std::make_shared<std::string>("BitwiseXor");
    case TokenType::BitwiseNot:
        return std::make_shared<std::string>("BitwiseNot");
    case TokenType::LeftShift:
        return std::make_shared<std::string>("LeftShift");
    case TokenType::RightShift:
        return std::make_shared<std::string>("RightShift");
    case TokenType::And:
        return std::make_shared<std::string>("And");
    case TokenType::Or:
        return std::make_shared<std::string>("Or");
    case TokenType::Not:
        return std::make_shared<std::string>("Not");
    case TokenType::Def:
        return std::make_shared<std::string>("Def");
    case TokenType::If:
        return std::make_shared<std::string>("If");
    case TokenType::Else:
        return std::make_shared<std::string>("Else");
    case TokenType::ElIf:
        return std::make_shared<std::string>("ElIf");
    case TokenType::Is:
        return std::make_shared<std::string>("Is");
    case TokenType::While:
        return std::make_shared<std::string>("While");
    case TokenType::For:
        return std::make_shared<std::string>("For");
    case TokenType::In:
        return std::make_shared<std::string>("In");
    case TokenType::Break:
        return std::make_shared<std::string>("Break");
    case TokenType::Continue:
        return std::make_shared<std::string>("Continue");
    case TokenType::Struct:
        return std::make_shared<std::string>("Struct");
    case TokenType::True:
        return std::make_shared<std::string>("True");
    case TokenType::False:
        return std::make_shared<std::string>("False");
    case TokenType::Enum:
        return std::make_shared<std::string>("Enum");
    case TokenType::Volatile:
        return std::make_shared<std::string>("Volatile");
    case TokenType::Use:
        return std::make_shared<std::string>("Use");
    // case TokenType::Maybe:
    //     return std::make_shared<std::string>("Maybe");
    case TokenType::Import:
        return std::make_shared<std::string>("Import");
    case TokenType::None:
        return std::make_shared<std::string>("None");
    case TokenType::String:
        return std::make_shared<std::string>("String");
    default:
        return std::make_shared<std::string>("unknown token type(" + std::to_string(static_cast<int>(type)) + ")");
    }
}

void token::Token::print() { std::cout << toString() << std::endl; }
