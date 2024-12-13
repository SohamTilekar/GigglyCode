#include "errors.hpp"
#include "../lexer/lexer.hpp"

#include <memory>
#include <sstream>
#include <string>
#include <iostream>

// Color and style codes
const std::string RESET = "\033[0m";
const std::string BOLD = "\033[1m";
const std::string RED = "\033[1;31m";
const std::string LIGHT_GRAY = "\033[0;97m";
const std::string MAGENTA = "\033[1;35m";
const std::string CYAN = "\033[1;36m";
const std::string BLUE = "\033[1;34m";
const std::string YELLOW = "\033[1;33m";

class LineIterator {
  public:
    LineIterator(const std::string& multi_line_str, int start_line_no, int end_line_no) : stream(multi_line_str), current_line_no(1), start_line_no(start_line_no), end_line_no(end_line_no) {
        while (current_line_no < start_line_no && std::getline(stream, current_line)) {
            ++current_line_no;
            before_start_line = current_line + "\n";
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

void print_banner(const std::string& label) {
    const std::string banner = std::string(30, '=') + " " + label + " " + std::string(30, '=');
    std::cerr << "\n\n" << MAGENTA << BOLD << banner << RESET << "\n\n";
}

void print_error_message(const std::string& message) { std::cerr << RED << BOLD << "Error:" << RESET << " " << LIGHT_GRAY << message << RESET << "\n\n"; }

void print_message(const std::string& message) { std::cerr << LIGHT_GRAY << message << RESET << "\n\n"; }

void print_source_context(const std::string& source, int st_line, int end_line, const std::vector<std::tuple<int, std::string, std::string>>& underlines = {}) {
    std::cerr << CYAN << BOLD << "Source Context:" << RESET << "\n";
    LineIterator lineIterator(source, st_line, end_line);
    int c_line = st_line;
    if (st_line > 1) {
        std::cerr << BLUE << (st_line - 1) << " |" << RESET << " " << LIGHT_GRAY << lineIterator.get_before_start_line() << RESET;
    }
    while (lineIterator.has_next()) {
        auto line = lineIterator.next();
        std::cerr << BLUE << c_line << " |" << RESET << " " << LIGHT_GRAY << line << RESET << "\n";

        for (const auto& [line_num, underline, color] : underlines) {
            if (c_line == line_num) {
                std::cerr << BLUE << std::string(std::to_string(c_line).length(), ' ') << " |" << RESET << color << BOLD << underline << RESET << "\n";
            }
        }

        c_line++;
    }
    if (end_line < getNumberOfLines(source)) {
        std::cerr << BLUE << (end_line) << " |" << RESET << " " << LIGHT_GRAY << lineIterator.get_after_end_line() << RESET << "\n";
    }
}

void print_suggested_fix(const std::string& suggestedFix) {
    if (!suggestedFix.empty()) {
        std::cerr << "\n" << YELLOW << BOLD << "Suggested Fix:" << RESET << " " << LIGHT_GRAY << suggestedFix << RESET << "\n\n";
    }
}

void errors::Error::raise() {
    print_banner("Error");
    print_error_message(message);
    print_source_context(source, st_line, end_line);
    print_suggested_fix(suggestedFix);
    std::cerr << RED << std::string(70, '=') << RESET << "\n";
    exit(EXIT_FAILURE);
}

void errors::NoPrefixParseFnError::raise() {
    print_banner("NoPrefixParseFnError");
    print_error_message(message);
    print_source_context(source, token.line_no, token.line_no);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::SyntaxError::raise() {
    print_banner("Syntax Error");
    print_error_message(message);
    std::vector<std::tuple<int, std::string, std::string>> underlines = {{token.line_no, std::string(token.col_no, ' ') + std::string(token.end_col_no - token.col_no, '^'), RED}};
    print_source_context(source, token.line_no, token.line_no + getNumberOfLines(token.literal) - 1, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::NodeOutside::raise() {
    print_banner("Compilation Error");
    print_error_message(message);
    std::vector<std::tuple<int, std::string, std::string>> underlines = {{node.meta_data.st_line_no,
                                                                          std::string(node.meta_data.st_col_no + 1, ' ') + std::string(this->nodeType == errors::outsideNodeType::Break      ? 5
                                                                                                                                       : this->nodeType == errors::outsideNodeType::Continue ? 8
                                                                                                                                                                                             : 6,
                                                                                                                                       '^'),
                                                                          RED}};
    print_source_context(source, node.meta_data.st_line_no, node.meta_data.end_line_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::CompletionError::raise() {
    print_banner("Completion Error");
    print_error_message(message);
    print_source_context(source, st_line, end_line);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::NoOverload::raise() {
    print_banner("No Function Overload");
    print_error_message(message);

    // Extract the start and end lines of the function call
    int start_line = func_call->meta_data.st_line_no;
    int end_line = func_call->meta_data.end_line_no;

    // Prepare the underlines for mismatched arguments
    std::vector<std::tuple<int, std::string, std::string>> underlines;
    for (const auto& mismatch : this->missmatches) {
        for (int idx : mismatch) {
            int line_no = start_line;
            int col_no = std::static_pointer_cast<AST::CallExpression>(func_call)->arguments[idx]->meta_data.st_col_no;
            int end_col_no = std::static_pointer_cast<AST::CallExpression>(func_call)->arguments[idx]->meta_data.end_col_no;
            std::string underline = std::string(col_no, ' ') + std::string(end_col_no - col_no, '^');
            underlines.push_back({line_no, underline, RED});
        }
    }

    // Print the source context with underlines
    print_source_context(source, start_line, end_line, underlines);
    print_suggested_fix(suggestedFix);

    exit(EXIT_FAILURE);
}

void errors::DosentContain::raise() {
    print_banner("Doesn't Contain");
    print_error_message(message);

    // Extract the start and end lines of the `from` and `member`
    int start_line = from->meta_data.st_line_no;
    int end_line = from->meta_data.end_line_no;

    // Prepare the underlines for `from` and `member`
    std::vector<std::tuple<int, std::string, std::string>> underlines;

    // Blue underline for `from`
    std::string from_underline = std::string(from->meta_data.st_col_no + 1, ' ') + std::string(from->meta_data.end_col_no - from->meta_data.st_col_no + 1, '^');
    underlines.push_back({from->meta_data.st_line_no, from_underline, BLUE});

    // Red underline for `member`
    std::string member_underline = std::string(member->meta_data.st_col_no + 1, ' ') + std::string(member->meta_data.end_col_no - member->meta_data.st_col_no + 1, '^');
    underlines.push_back({member->meta_data.st_line_no, member_underline, RED});

    // Print the source context with underlines
    print_source_context(source, start_line, end_line, underlines);
    print_suggested_fix(suggestedFix);

    exit(EXIT_FAILURE);
}

void errors::WrongInfix::raise() {
    print_banner("Wrong Infix");
    print_error_message(message);
    std::vector<std::tuple<int, std::string, std::string>> underlines = {
        {left->meta_data.st_line_no, std::string(left->meta_data.st_col_no + 1, ' ') + std::string(right->meta_data.end_col_no - left->meta_data.st_col_no + 1, '^'), RED}};
    print_source_context(source, left->meta_data.st_line_no, right->meta_data.end_line_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::WrongType::raise() {
    print_banner("Wrong Type");
    print_error_message(message);
    if (!this->expected.empty()) {
        std::string x = "Expected ";
        for (size_t i = 0; i < this->expected.size(); ++i) {
            x += this->expected[i]->name;
            if (i < this->expected.size() - 1) {
                x += " or ";
            }
        }
        print_message(x + " but got");
    }
    std::vector<std::tuple<int, std::string, std::string>> underlines = {
        {exp->meta_data.st_line_no, std::string(exp->meta_data.st_col_no + 1, ' ') + std::string(exp->meta_data.end_col_no - exp->meta_data.st_col_no + 1, '^'), RED}};
    print_source_context(source, exp->meta_data.st_line_no, exp->meta_data.end_line_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::Cantindex::raise() {
    print_banner("Can't Index");
    print_error_message(message);

    // Extract the start and end lines of the `exp`
    int start_line = exp->meta_data.st_line_no;
    int end_line = exp->meta_data.end_line_no;

    // Prepare the underlines for `exp->left` and `exp->index`
    std::vector<std::tuple<int, std::string, std::string>> underlines;

    if (wrongIDX) {
        std::string index_underline = std::string(exp->index->meta_data.st_col_no + 1, ' ') + std::string(exp->index->meta_data.end_col_no - exp->index->meta_data.st_col_no + 1, '^');
        underlines.push_back({exp->index->meta_data.st_line_no, index_underline, RED});
        std::string left_underline = std::string(exp->left->meta_data.st_col_no + 1, ' ') + std::string(exp->left->meta_data.end_col_no - exp->left->meta_data.st_col_no + 1, '~');
        underlines.push_back({exp->left->meta_data.st_line_no, left_underline, BLUE});
    } else {
        std::string left_underline = std::string(exp->left->meta_data.st_col_no + 1, ' ') + std::string(exp->left->meta_data.end_col_no - exp->left->meta_data.st_col_no + 1, '^');
        underlines.push_back({exp->left->meta_data.st_line_no, left_underline, RED});
    }

    // Print the source context with underlines
    print_source_context(source, start_line, end_line, underlines);
    print_suggested_fix(suggestedFix);

    exit(EXIT_FAILURE);
}

void errors::NotDefined::raise() {
    print_banner("Not Defined");
    print_error_message(message);
    std::string underline = std::string(Name->meta_data.st_col_no + 1, ' ') + std::string(Name->meta_data.end_col_no - Name->meta_data.st_col_no + 1, '^');
    print_source_context(source, Name->meta_data.st_line_no, Name->meta_data.end_line_no, {{Name->meta_data.st_line_no, underline, RED}});
    print_suggested_fix(suggestedFix);

    exit(EXIT_FAILURE);
}

void errors::raiseSyntaxError(const std::string& type, const std::string& source, const token::Token& token, const std::string& message, const std::string& suggestedFix) {
    errors::SyntaxError error(type, source, token, message, suggestedFix);
    error.raise();
}
