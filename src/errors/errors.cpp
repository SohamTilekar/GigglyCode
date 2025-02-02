#include "errors.hpp"
#include "../lexer/lexer.hpp"

#include <iomanip>
#include <iostream>
#include <llvm/ADT/STLExtras.h>
#include <map>
#include <random>
#include <string>
#include <sys/ioctl.h>
#include <unistd.h>
#include <vector>

// ANSI color codes for styling
const std::string RESET = "\033[0m";
const std::string BOLD = "\033[1m";
const std::string UNDERLINE = "\033[4m";
const std::string RED = "\033[1;31m";
const std::string LIGHT_RED = "\033[0;31m";
const std::string GREEN = "\033[1;32m";
const std::string YELLOW = "\033[1;33m";
const std::string BLUE = "\033[1;34m";
const std::string MAGENTA = "\033[1;35m";
const std::string CYAN = "\033[1;36m";
const std::string LIGHT_GRAY = "\033[0;37m";
const std::string DARK_GRAY = "\033[1;30m";

const std::string ICON_ERROR = "❌";
const std::string ICON_WARNING = "⚠️";
const std::string ICON_INFO = "ℹ️";
const std::string ICON_SUGGESTION = "💡";

int get_terminal_width() {
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1) {
        std::cerr << "ioctl() failed: " << strerror(errno) << std::endl;
        return 0;
    }
    return ws.ws_col;
}

std::string truncate_line(const std::string& line, size_t max_width, int inc) {
    if ((line.length() - inc) <= max_width) { return line; }
    return line.substr(0, max_width + inc - 14) + "...";
}

std::string highlight_syntax(const std::string& line) {
    Lexer lex(line, "", true);
    std::string highlighted_line = line;
    auto current_token = lex.nextToken();
    int inc = 0;
    while (current_token.type != token::TokenType::EndOfFile) {
        switch (current_token.type) {
            case token::TokenType::Identifier:
                break;
            case token::TokenType::And:
            case token::TokenType::Or:
            case token::TokenType::Not:
            case token::TokenType::Def:
            case token::TokenType::If:
            case token::TokenType::Else:
            case token::TokenType::ElIf:
            case token::TokenType::Is:
            case token::TokenType::While:
            case token::TokenType::For:
            case token::TokenType::In:
            case token::TokenType::Break:
            case token::TokenType::Continue:
            case token::TokenType::Struct:
            case token::TokenType::True:
            case token::TokenType::False:
            case token::TokenType::Enum:
            case token::TokenType::Volatile:
            case token::TokenType::Use:
            case token::TokenType::Import:
            case token::TokenType::As:
            case token::TokenType::None:
            case token::TokenType::New:
            case token::TokenType::Try:
            case token::TokenType::Catch:
            case token::TokenType::Raise:
            case token::TokenType::NotBreak:
            case token::TokenType::IfBreak:
            case token::TokenType::Switch:
            case token::TokenType::Case:
            case token::TokenType::Other:
            case token::TokenType::Return: {
                auto temp = highlighted_line;
                temp = highlighted_line.substr(0, current_token.col_no + inc);
                temp = temp + LIGHT_RED + highlighted_line.substr(current_token.col_no + inc, current_token.end_col_no - current_token.col_no);
                temp = temp + RESET + highlighted_line.substr(current_token.end_col_no + inc);
                inc += 11;
                highlighted_line = temp;
                break;
            }
            case token::TokenType::Integer:
            case token::TokenType::Float: {
                auto temp = highlighted_line;
                temp = highlighted_line.substr(0, current_token.col_no + inc);
                temp = temp + BLUE + highlighted_line.substr(current_token.col_no + inc, current_token.end_col_no - current_token.col_no);
                temp = temp + RESET + highlighted_line.substr(current_token.end_col_no + inc);
                inc += 11;
                highlighted_line = temp;
                break;
            }
            case token::TokenType::Dot:
            case token::TokenType::Plus:
            case token::TokenType::Dash:
            case token::TokenType::Colon:
            case token::TokenType::Semicolon:
            case token::TokenType::Comma:
            case token::TokenType::AtTheRate:
            case token::TokenType::Pipe:
            case token::TokenType::Equals:
            case token::TokenType::GreaterThan:
            case token::TokenType::LessThan:
            case token::TokenType::Percent:
            case token::TokenType::LeftParen:
            case token::TokenType::RightParen:
            case token::TokenType::LeftBrace:
            case token::TokenType::RightBrace:
            case token::TokenType::LeftBracket:
            case token::TokenType::RightBracket:
            case token::TokenType::Asterisk: {
                auto temp = highlighted_line;
                temp = highlighted_line.substr(0, current_token.col_no + inc + 1);
                temp = temp + YELLOW + highlighted_line.substr(current_token.col_no + inc + 1, current_token.literal.length());
                temp = temp + RESET + highlighted_line.substr(current_token.end_col_no + inc + 1);
                inc += 11;
                highlighted_line = temp;
                break;
            }
            case token::TokenType::Increment:
            case token::TokenType::Decrement:
            case token::TokenType::PlusEqual:
            case token::TokenType::DashEqual:
            case token::TokenType::AsteriskEqual:
            case token::TokenType::PercentEqual:
            case token::TokenType::CaretEqual:
            case token::TokenType::ForwardSlashEqual:
            case token::TokenType::BackwardSlashEqual:
            case token::TokenType::Ellipsis:
            case token::TokenType::AsteriskAsterisk:
            case token::TokenType::ForwardSlash:
            case token::TokenType::BackwardSlash:
            case token::TokenType::RightArrow:
            case token::TokenType::GreaterThanOrEqual:
            case token::TokenType::LessThanOrEqual:
            case token::TokenType::EqualEqual:
            case token::TokenType::NotEquals:
            case token::TokenType::BitwiseAnd:
            case token::TokenType::BitwiseOr:
            case token::TokenType::BitwiseXor:
            case token::TokenType::BitwiseNot:
            case token::TokenType::LeftShift:
            case token::TokenType::RightShift: {
                auto temp = highlighted_line;
                temp = highlighted_line.substr(0, current_token.col_no + inc + 1);
                temp = temp + YELLOW + highlighted_line.substr(current_token.col_no + inc + 1, current_token.literal.length() + 1);
                temp = temp + RESET + highlighted_line.substr(current_token.end_col_no + inc + 2);
                inc += 11;
                highlighted_line = temp;
                break;
            }
            case token::TokenType::Illegal: {
                auto temp = highlighted_line;
                temp = highlighted_line.substr(0, current_token.col_no + inc);
                temp = temp + RED + highlighted_line.substr(current_token.col_no + inc, current_token.end_col_no - current_token.col_no);
                temp = temp + RESET + highlighted_line.substr(current_token.end_col_no + inc);
                inc += 11;
                highlighted_line = temp;
                break;
            }
            case token::TokenType::String: {
                auto temp = highlighted_line;
                temp = highlighted_line.substr(0, current_token.col_no + inc);
                temp = temp + MAGENTA + highlighted_line.substr(current_token.col_no + inc, current_token.end_col_no - current_token.col_no);
                temp = temp + RESET + highlighted_line.substr(current_token.end_col_no + inc);
                inc += 11;
                highlighted_line = temp;
                break;
            }
            case token::TokenType::Coment: {
                auto temp = highlighted_line;
                temp = highlighted_line.substr(0, current_token.col_no + inc);
                temp = temp + DARK_GRAY + highlighted_line.substr(current_token.col_no + inc);
                temp = temp + RESET;
                inc += 11;
                highlighted_line = temp;
                break;
            }
            default:
                break;
        }
        current_token = lex.nextToken();
    }
    auto trunc_line = truncate_line(highlighted_line, get_terminal_width(), inc);
    std::vector<std::string> colours = {
        /*Red*/ "\033[41m",
        /*Yellow*/ "\033[43m",
        /*Green*/ "\033[42m",
        /*Cyan*/ "\033[46m",
        /*Blue*/ "\033[44m",
        /*Magenta*/ "\033[45m",
    };
    std::string coloured_line;
    int color_idx = 0;
    size_t i = 0;

    // Apply indentation coloring to leading spaces
    while (i < trunc_line.size() && trunc_line[i] == ' ') {
        size_t space_count = 4;
        space_count = std::min(space_count, trunc_line.size() - i);
        coloured_line += colours[color_idx++ % colours.size()] + trunc_line.substr(i, space_count) + "\033[0m";
        i += space_count;
    }

    coloured_line += trunc_line.substr(i);
    return coloured_line;
}

class LineIterator {
  public:
    LineIterator(const std::string& multi_line_str, int start_line_no, int end_line_no) : stream(multi_line_str), current_line_no(1), start_line_no(start_line_no), end_line_no(end_line_no) {
        while (current_line_no < start_line_no && std::getline(stream, current_line)) {
            ++current_line_no;
            before_start_line += current_line + "\n";
        }
    }

    bool has_next() { return current_line_no <= end_line_no && std::getline(stream, current_line); }

    std::string next() {
        current_line_no++;
        return current_line;
    }

    const std::string& get_before_start_line() const { return before_start_line; }

    std::string get_after_end_line() {
        std::getline(stream, after_end_line);
        return after_end_line;
    }

  private:
    std::istringstream stream;
    std::string current_line;
    std::string before_start_line;
    std::string after_end_line;
    int current_line_no;
    int start_line_no;
    int end_line_no;
};

std::vector<std::tuple<int, std::string, std::string>> underline(int st_line_no, int st_col_no, int end_line_no, int end_col_no, const std::string& source, const std::string& color) {
    std::vector<std::tuple<int, std::string, std::string>> underlines;

    if (st_line_no == end_line_no) {
        std::string underline = std::string(st_col_no - 1, ' ') + std::string(end_col_no - st_col_no, '^');
        underlines.emplace_back(st_line_no, underline, color);
    } else {
        LineIterator iter(source, st_line_no, end_line_no);
        std::string underline;
        int line_no = st_line_no;
        // Process the first line
        if (iter.has_next()) {
            auto fline = iter.next();
            underline = std::string(st_col_no - 1, ' ') + std::string(fline.length() - st_col_no + 1, '^');
            underlines.emplace_back(line_no, underline, color);
            line_no++;
        }

        // Process the middle lines
        while (iter.has_next() && line_no < end_line_no) {
            auto line = iter.next();
            underline = std::string(line.length(), '^');
            underlines.emplace_back(line_no, underline, color);
            line_no++;
        }

        // Process the last line
        if (iter.has_next()) {
            auto eline = iter.next();
            underline = std::string(end_col_no, '^');
            underlines.emplace_back(end_line_no, underline, color);
        }
    }

    return underlines;
}

std::vector<std::tuple<int, std::string, std::string>> underline_node(const AST::Node* node, const std::string& source, const std::string& color) {
    return underline(node->meta_data.st_line_no, node->meta_data.st_col_no, node->meta_data.end_line_no, node->meta_data.end_col_no, source, color);
}

// Utility functions for printing
void print_banner(const std::string& label) {
    const std::string banner = ICON_ERROR + " " + label + " " + ICON_ERROR;
    std::cerr << "\n" << BOLD << MAGENTA << banner << RESET << "\n\n";
}

void print_error_message(const std::string& message) {
    std::cerr << BOLD << RED << ICON_ERROR << " Error:" << RESET << " " << message << "\n";
}

void print_warning_message(const std::string& message) {
    std::cerr << BOLD << YELLOW << ICON_WARNING << " Warning:" << RESET << " " << message << "\n";
}

void print_info_message(const std::string& message) {
    std::cerr << BOLD << CYAN << ICON_INFO << " Info:" << RESET << " " << message << "\n";
}

void print_suggested_fix(const std::string& suggestedFix) {
    if (!suggestedFix.empty()) { std::cerr << BOLD << GREEN << ICON_SUGGESTION << " Suggested Fix:" << RESET << " " << suggestedFix << "\n"; }
    std::cerr << "\n";
}

void print_funny_message(int line_no, const std::string& line, const std::vector<std::tuple<int, std::string, std::string>>& underlines) {
    std::vector<std::string> messages = {"😡: Why is this line so long? Why? You silly goose!",
                                         "😡: Why is this line so long? Why? You code monster!",
                                         "😡: Why is this line so long? Why? You code gremlin!",
                                         "😡: Why is this line so long? Why? You code wizard!",
                                         "😡: This line is longer than my patience!",
                                         "😡: Are you trying to write a novel in one line?",
                                         "😡: This line is longer than a Monday morning meeting!",
                                         "😡: This line is longer than a giraffe's neck!",
                                         "😡: This line is longer than a CVS receipt!",
                                         "😡: This line is longer than a summer day!",
                                         "😡: This line is longer than a traffic jam!",
                                         "😡: This line is longer than a horror movie sequel!",
                                         "😡: This line is longer than a never-ending story!",
                                         "😡: This line is longer than a politician's speech!"};

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, messages.size() - 1);

    std::string message = messages[dis(gen)];
    size_t box_width = message.length() + 4;

    std::cerr << BLUE << "   " << line_no - 1 << " │ " << line << RESET << "\n";
    for (const auto& [line_num, underline, color] : underlines) {
        if (line_no - 1 == line_num) {
            std::string updated_underline = underline;
            if (underline.length() < box_width) {
                updated_underline += std::string(box_width - underline.length() - 1, ' ') + "^";
            } else {
                updated_underline.replace(box_width, 1, "^");
            }
            std::cerr << BLUE << "     │ " << RESET << color << BOLD << updated_underline << RESET << "\n";
        }
    }
    std::cerr << BLUE << "     ╭";
    for (size_t i = 0; i < box_width; ++i) std::cerr << "─";
    std::cerr << "⎨\n" << RESET;
    std::cerr << BLUE << "     │ " << RESET << message << BLUE << std::string(box_width - message.length(), ' ') << " │\n";
    std::cerr << "     ╰";
    for (size_t i = 0; i < box_width; ++i) std::cerr << "─";
    std::cerr << "╯\n" << RESET;
}

void print_source_context(
    const std::string& source, const std::string& file_path, int st_line, int st_col, int end_line, int end_col, const std::vector<std::tuple<int, std::string, std::string>>& underlines = {}) {
    std::cerr << BOLD << DARK_GRAY << file_path << ":" << st_line << ":" << st_col << RESET << "\n\n";

    // Merge multiple underlines on the same line into one
    std::map<int, std::string> merged_underlines;
    for (const auto& [line_num, underline, color] : underlines) {
        if (merged_underlines.find(line_num) == merged_underlines.end()) {
            merged_underlines[line_num] = underline;
        } else {
            // Merge the existing underline with the new one
            std::string& existing = merged_underlines[line_num];
            size_t max_length = std::max(existing.length(), underline.length());
            existing.resize(max_length, ' ');
            std::string merged;
            for (size_t i = 0; i < max_length; ++i) {
                char c1 = (i < existing.length()) ? existing[i] : ' ';
                char c2 = (i < underline.length()) ? underline[i] : ' ';
                if (c1 == '^' || c2 == '^') {
                    merged += '^';
                } else {
                    merged += ' ';
                }
            }
            merged_underlines[line_num] = merged;
        }
    }

    LineIterator lineIterator(source, st_line > 1 ? st_line - 1 : st_line, end_line + 1);
    int current_line = st_line > 1 ? st_line - 1 : st_line;

    // Display lines with line numbers
    while (lineIterator.has_next()) {
        auto line = lineIterator.next();
        if (line.length() > get_terminal_width()) {
            current_line++;
            print_funny_message(current_line, highlight_syntax(line), underlines);
            continue;
        } else {
            std::cerr << BLUE << std::setw(4) << current_line << " │ " << RESET << highlight_syntax(line) << "\n";
        }
        // Check if current line has any underlines
        auto it = merged_underlines.find(current_line);
        if (it != merged_underlines.end()) {
            std::cerr << BLUE << "     │ " << RESET;
            std::cerr << BOLD << RED << it->second << RESET << '\n';
        }

        current_line++;
    }

    std::cerr << "\n";
}

namespace errors {

void raiseFileNotFoundError(const std::string& message, const std::string& suggestedFix) {
    print_banner("File Not Found");
    print_error_message(message);
    print_suggested_fix(suggestedFix);
    std::cerr << std::string(50, '=') << "\n";
    exit(EXIT_FAILURE);
}

void raiseCompilationError(const std::string& message, const std::string& suggestedFix) {
    print_banner("Compilation Error");
    print_error_message(message);
    print_suggested_fix(suggestedFix);
    std::cerr << std::string(50, '=') << "\n";
    exit(EXIT_FAILURE);
}

void raiseUnknownError(const std::string& label, const std::string& message, const std::string& suggestedFix) {
    print_banner(label);
    print_error_message(message);
    print_suggested_fix(suggestedFix);
    std::cerr << std::string(50, '=') << "\n";
    exit(EXIT_FAILURE);
}

// General error raiser
void raiseError(const std::string& file_path, const std::string& source, int st_line, int st_col, int end_line, int end_col, const std::string& message, const std::string& suggestedFix) {
    print_banner("Error");
    print_error_message(message);
    print_source_context(source, file_path, st_line, st_col, end_line, end_col);
    print_suggested_fix(suggestedFix);
    std::cerr << std::string(50, '=') << "\n";
    exit(EXIT_FAILURE);
}

void raiseCompletionError(const std::string& file_path, const std::string& source, int st_line, int st_col, int end_line, int end_col, const std::string& message, const std::string& suggestedFix) {
    print_banner("Completion Error");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines = underline(st_line, st_col, end_line, end_col, source, RED);

    print_source_context(source, file_path, st_line, st_col, end_line, end_col, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void raiseNodeOutsideError(const std::string& file_path, const std::string& source, const AST::Node* node, OutsideNodeType nodeType, const std::string& message, const std::string& suggestedFix) {
    print_banner("Invalid Node Usage");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines = underline_node(node, source, RED);

    print_source_context(source, file_path, node->meta_data.st_line_no, node->meta_data.st_col_no, node->meta_data.end_line_no, node->meta_data.end_col_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void raiseSyntaxError(const std::string& file_path, const token::Token& token, const std::string& source, const std::string& message, const std::string& suggestedFix) {
    print_banner("Syntax Error");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines = underline(token.st_line_no, token.col_no, token.end_line_no, token.end_col_no, source, RED);

    print_source_context(source, file_path, token.end_line_no, token.col_no, token.end_line_no, token.end_col_no - 1, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void raiseNoPrefixParseFnError(const std::string& file_path, const token::Token& token, const std::string& source, const std::string& message, const std::string& suggestedFix) {
    print_banner("No Prefix Parse Function Error");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines = underline(token.st_line_no, token.col_no, token.end_line_no, token.end_col_no, source, RED);

    print_source_context(source, file_path, token.end_line_no, token.col_no, token.end_line_no, token.end_col_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void raiseNoOverloadError(const std::string& file_path,
                          const std::string& source,
                          const std::vector<std::vector<unsigned short>>& mismatches,
                          AST::Expression* func_call,
                          const std::string& message,
                          const std::string& suggestedFix) {
    print_banner("No Function Overload Found");
    print_error_message(message);

    // Extract the start and end lines of the function call
    int start_line = func_call->meta_data.st_line_no;
    int end_line = func_call->meta_data.end_line_no;

    // Prepare the underlines for mismatched arguments
    std::vector<std::tuple<int, std::string, std::string>> underlines;
    for (const auto& mismatch : mismatches) {
        for (int idx : mismatch) {
            auto callExpr = func_call->castToCallExpression();
            if (!callExpr || idx >= static_cast<int>(callExpr->arguments.size())) continue;
            auto arg = callExpr->arguments[idx];
            auto arg_underlines = underline_node(arg, source, RED);
            underlines.insert(underlines.end(), arg_underlines.begin(), arg_underlines.end());
        }
    }

    // Print the source context with underlines
    print_source_context(source, file_path, start_line, 1, end_line, 1, underlines);
    print_suggested_fix(suggestedFix);

    exit(EXIT_FAILURE);
}

void raiseDoesntContainError(
    const std::string& file_path, const std::string& source, AST::IdentifierLiteral* member, AST::Expression* from, const std::string& message, const std::string& suggestedFix) {
    print_banner("Missing Member");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines;

    if (from) {
        auto from_underlines = underline_node(from, source, BLUE);
        underlines.insert(underlines.end(), from_underlines.begin(), from_underlines.end());
    }

    if (member) {
        auto member_underlines = underline_node(member, source, RED);
        underlines.insert(underlines.end(), member_underlines.begin(), member_underlines.end());
    }

    print_source_context(source, file_path, from->meta_data.st_line_no, from->meta_data.st_col_no, member->meta_data.end_line_no, member->meta_data.end_col_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void raiseWrongInfixError(
    const std::string& file_path, const std::string& source, AST::Expression* left, AST::Expression* right, const std::string& op, const std::string& message, const std::string& suggestedFix) {
    print_banner("Invalid Operator Usage");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines =
        underline(left->meta_data.st_line_no, left->meta_data.st_col_no, right->meta_data.end_line_no, right->meta_data.end_col_no, source, RED);

    print_source_context(source, file_path, left->meta_data.st_line_no, left->meta_data.st_col_no, right->meta_data.end_line_no, right->meta_data.end_col_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void raiseWrongTypeError(const std::string& file_path,
                         const std::string& source,
                         AST::Expression* exp,
                         enviornment::RecordStructType* got,
                         const std::vector<enviornment::RecordStructType*>& expected,
                         const std::string& message,
                         const std::string& suggestedFix,
                         bool is_const) {
    print_banner("Type Mismatch");
    print_error_message(message);

    if (!expected.empty()) {
        std::string expected_types;
        for (size_t i = 0; i < expected.size(); ++i) {
            expected_types += expected[i]->name;
            if (i < expected.size() - 1) { expected_types += " or "; }
        }
        std::cerr << "Expected type: " << GREEN << expected_types << RESET << " but got " << RED << (is_const ? "const " : "") << (got ? got->name : "Module or Type") << RESET << "\n\n";
    }

    std::vector<std::tuple<int, std::string, std::string>> underlines = underline_node(exp, source, RED);
    print_source_context(source, file_path, exp->meta_data.st_line_no, exp->meta_data.st_col_no, exp->meta_data.end_line_no, exp->meta_data.end_col_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void raiseCantIndexError(const std::string& file_path, const std::string& source, AST::IndexExpression* exp, bool wrongIDX, const std::string& message, const std::string& suggestedFix) {
    print_banner("Indexing Error");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines;

    if (auto indexExpr = exp->castToIndexExpression()) {
        // Underline the left expression
        auto left_underlines = underline_node(indexExpr->left, source, BLUE);
        underlines.insert(underlines.end(), left_underlines.begin(), left_underlines.end());

        // Underline the index expression if present
        if (indexExpr->index) {
            auto index_underlines = underline_node(indexExpr->index, source, RED);
            underlines.insert(underlines.end(), index_underlines.begin(), index_underlines.end());
        }
    }

    print_source_context(source, file_path, exp->meta_data.st_line_no, exp->meta_data.st_col_no, exp->meta_data.end_line_no, exp->meta_data.end_col_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void raiseNotDefinedError(const std::string& file_path, const std::string& source, AST::Node* Name, const std::string& message, const std::string& suggestedFix) {
    print_banner("Undefined Identifier");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines = underline_node(Name, source, RED);

    print_source_context(source, file_path, Name->meta_data.st_line_no, Name->meta_data.st_col_no, Name->meta_data.end_line_no, Name->meta_data.end_col_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void raiseDuplicateVariableError(const std::string& file_path, const std::string& source, const std::string& variableName, const AST::Node* declarationNode, const std::string& message) {
    print_banner("Duplicate Variable Error");
    std::string errorMsg = "Variable '" + variableName + "' is already defined in this scope.";
    print_error_message(errorMsg);

    if (declarationNode) {
        std::vector<std::tuple<int, std::string, std::string>> underlines = underline_node(declarationNode, source, RED);
        print_source_context(source,
                             file_path,
                             declarationNode->meta_data.st_line_no,
                             declarationNode->meta_data.st_col_no,
                             declarationNode->meta_data.end_line_no,
                             declarationNode->meta_data.end_col_no,
                             underlines);
    } else {
        // If no node is provided, display the file path as general context
        std::cerr << BOLD << DARK_GRAY << file_path << RESET << "\n";
    }

    print_suggested_fix("Consider renaming the variable or removing the duplicate declaration.");
    std::cerr << std::string(50, '=') << "\n";
    exit(EXIT_FAILURE);
}

void raiseUnknownNodeTypeError(
    const std::string& file_path, const std::string& source, const std::string& type, int st_line, int st_col, int end_line, int end_col, const std::string& message, const std::string& suggestedFix) {
    print_banner("Unknown Node Type Error");
    std::string errorMsg = "Encountered an unknown node type: '" + type + "'.";
    print_error_message(errorMsg);

    // Assuming st_line and end_line are set correctly
    if (st_line != -1 && end_line != -1 && st_col != -1 && end_col != -1) {
        std::vector<std::tuple<int, std::string, std::string>> underlines = underline(st_line, st_col, end_line, end_col, source, RED);
        print_source_context(source, file_path, st_line, st_col, end_line, end_col, underlines);
    }

    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void raiseArrayTypeError(const std::string& file_path, const std::string& source, AST::Node* element, enviornment::RecordStructType* expected_type, const std::string& message) {
    print_banner("Array Type Error");
    print_error_message(message);

    std::string expected_type_str = expected_type ? expected_type->name : "unknown";
    std::cerr << "Expected type for array elements: " << GREEN << expected_type_str << RESET << "\n\n";

    std::vector<std::tuple<int, std::string, std::string>> underlines = underline_node(element, source, RED);

    print_source_context(source, file_path, element->meta_data.st_line_no, element->meta_data.st_col_no, element->meta_data.end_line_no, element->meta_data.end_col_no, underlines);
    print_suggested_fix("Ensure that all elements in the array match the expected type.");
    exit(EXIT_FAILURE);
}

void raiseGenericStructResolutionError(const std::string& file_path, const std::string& source, const std::string& message, const std::string& suggestedFix) {
    print_banner("Generic Struct Resolution Error");
    print_error_message(message);

    // Provide general advice as specific context might not be available
    print_suggested_fix("Ensure that all generic parameters are correctly "
                        "specified and resolved.");

    std::cerr << BOLD << DARK_GRAY << file_path << RESET << "\n";
    std::cerr << std::string(50, '=') << "\n";
    exit(EXIT_FAILURE);
}

} // namespace errors
