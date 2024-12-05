#include "errors.hpp"
#include "../lexer/lexer.hpp"

#include <sstream>
#include <string>

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
        while(current_line_no < start_line_no && std::getline(stream, current_line)) {
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

void print_source_context(const std::string& source, int st_line, int end_line, const std::string& underline = "") {
    std::cerr << CYAN << BOLD << "Source Context:" << RESET << "\n";
    LineIterator lineIterator(source, st_line, end_line);
    int c_line = st_line;
    if(st_line > 1) {
        std::cerr << BLUE << (st_line - 1) << " |" << RESET << " " << LIGHT_GRAY << lineIterator.get_before_start_line() << RESET;
    }
    while(lineIterator.has_next()) {
        auto line = lineIterator.next();
        std::cerr << BLUE << c_line << " |" << RESET << " " << LIGHT_GRAY << line << RESET << "\n";
        if(!underline.empty() && c_line == st_line) {
            std::cerr << BLUE << std::string(std::to_string(c_line).length(), ' ') << " |" << RESET << RED << BOLD << underline << RESET << "\n";
        }
        c_line++;
    }
    if(end_line < getNumberOfLines(source)) {
        std::cerr << BLUE << (end_line + 1) << " |" << RESET << " " << LIGHT_GRAY << lineIterator.get_after_end_line() << RESET << "\n";
    }
}

void print_suggested_fix(const std::string& suggestedFix) {
    if(!suggestedFix.empty()) {
        std::cerr << "\n" << YELLOW << BOLD << "Suggested Fix:" << RESET << " " << LIGHT_GRAY << suggestedFix << RESET << "\n\n";
    }
}

void errors::Error::raise(bool terminate) {
    print_banner("Error");
    print_error_message(message);
    print_source_context(source, st_line, end_line);
    print_suggested_fix(suggestedFix);
    if(terminate) {
        std::cerr << RED << std::string(70, '=') << RESET << "\n";
        exit(EXIT_FAILURE);
    }
}

void errors::NoPrefixParseFnError::raise(bool terminate) {
    print_banner("NoPrefixParseFnError");
    print_error_message(message);
    print_source_context(source, token.line_no, token.line_no);
    print_suggested_fix(suggestedFix);
    if(terminate)
        exit(EXIT_FAILURE);
}

void errors::SyntaxError::raise(bool terminate) {
    print_banner("Syntax Error");
    print_error_message(message);
    std::string underline = std::string(token.col_no + 1, ' ') + std::string(token.end_col_no - token.col_no, '^');
    print_source_context(source, token.line_no, token.line_no + getNumberOfLines(token.literal) - 1, underline);
    print_suggested_fix(suggestedFix);
    if(terminate)
        exit(EXIT_FAILURE);
}

void errors::NodeOutside::raise(bool terminate) {
    print_banner("Compilation Error");
    print_error_message(message);
    std::string underline = std::string(node.meta_data.st_col_no + 1, ' ') + std::string(this->nodeType == errors::outsideNodeType::Break ? 5 : this->nodeType == errors::outsideNodeType::Continue ? 8 : 6 , '^');
    print_source_context(source, node.meta_data.st_line_no, node.meta_data.end_line_no, underline);
    print_suggested_fix(suggestedFix);
    if(terminate)
        exit(EXIT_FAILURE);
}

void errors::CompletionError::raise(bool terminate) {
    print_banner("Completion Error");
    print_error_message(message);
    print_source_context(source, st_line, end_line);
    print_suggested_fix(suggestedFix);
    if(terminate)
        exit(EXIT_FAILURE);
}

void errors::NoOverload::raise(bool terminate) {
    print_banner("No Function Overload");
    print_error_message(message);
    print_source_context(source, func_call->meta_data.st_line_no, func_call->meta_data.end_line_no);
    print_suggested_fix(suggestedFix);
    if(terminate)
        exit(EXIT_FAILURE);
}

void errors::DosentContain::raise(bool terminate) {
    print_banner("Doesn't Contain");
    print_error_message(message);
    print_source_context(source, from->meta_data.st_line_no, from->meta_data.end_line_no);
    print_suggested_fix(suggestedFix);
    if(terminate)
        exit(EXIT_FAILURE);
}

void errors::WrongInfix::raise(bool terminate) {
    print_banner("Wrong Infix");
    print_error_message(message);
    std::string underline = std::string(left->meta_data.st_col_no, ' ') + std::string(right->meta_data.end_col_no - left->meta_data.st_col_no, '^');
    print_source_context(source, left->meta_data.st_line_no, right->meta_data.end_line_no, underline);
    print_suggested_fix(suggestedFix);
    if(terminate)
        exit(EXIT_FAILURE);
}

void errors::WrongType::raise(bool terminate) {
    print_banner("Wrong Type");
    print_error_message(message);
    if(!this->expected.empty()) {
        std::string x = "\bExpected ";
        for (auto y : this->expected) {
            x += y->name;
            x += " or ";
        }
        print_message(x + "But got");
    }
    std::string underline = std::string(exp->meta_data.st_col_no + 1, ' ') + std::string(exp->meta_data.end_col_no - exp->meta_data.st_col_no, '^');
    print_source_context(source, exp->meta_data.st_line_no, exp->meta_data.end_line_no, underline);
    print_suggested_fix(suggestedFix);
    if(terminate)
        exit(EXIT_FAILURE);
}

void errors::Cantindex::raise(bool terminate) {
    print_banner("Can't Index");
    print_error_message(message);
    print_source_context(source, exp->meta_data.st_line_no, exp->meta_data.end_line_no);
    print_suggested_fix(suggestedFix);
    if(terminate)
        exit(EXIT_FAILURE);
}

void errors::NotDefined::raise(bool terminate) {
    print_banner("Not Defined");
    print_error_message(message);
    std::string underline = std::string(Name->meta_data.st_col_no + 1, ' ') + std::string(Name->meta_data.end_col_no - Name->meta_data.st_col_no, '^');
    print_source_context(source, Name->meta_data.st_line_no, Name->meta_data.end_line_no, underline);
    print_suggested_fix(suggestedFix);
    if(terminate)
        exit(EXIT_FAILURE);
}

void errors::raiseSyntaxError(const std::string& type, const std::string& source, const token::Token& token, const std::string& message, const std::string& suggestedFix) {
    errors::SyntaxError error(type, source, token, message, suggestedFix);
    error.raise();
}
