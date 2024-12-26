#include "errors.hpp"

#include <iomanip>
#include <iostream>

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

// Helper class to iterate through lines of source code
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

// Utility functions for printing
void print_banner(const std::string& label) {
    const std::string banner = "❌ " + label + " ❌";
    std::cerr << "\n" << BOLD << MAGENTA << banner << RESET << "\n\n";
}

void print_error_message(const std::string& message) {
    std::cerr << BOLD << RED << "Error:" << RESET << " " << message << "\n";
}

void print_suggested_fix(const std::string& suggestedFix) {
    if (!suggestedFix.empty()) { std::cerr << BOLD << YELLOW << "Suggested Fix:" << RESET << " " << suggestedFix << "\n"; }
    std::cerr << "\n";
}

void print_source_context(const std::string& source, int st_line, int end_line, const std::vector<std::tuple<int, std::string, std::string>>& underlines = {}) {
    std::cerr << BOLD << CYAN << "Source Context:" << RESET << "\n";
    LineIterator lineIterator(source, st_line, end_line);
    int c_line = st_line;

    // Display lines with line numbers
    while (lineIterator.has_next()) {
        auto line = lineIterator.next();
        std::cerr << BLUE << std::setw(4) << c_line << " | " << RESET << LIGHT_GRAY << line << RESET << "\n";

        // Check if current line has any underlines
        for (const auto& [line_num, underline, color] : underlines) {
            if (c_line == line_num) { std::cerr << "     | " << color << BOLD << underline << RESET << "\n"; }
        }

        c_line++;
    }

    // Optionally display lines before and after if necessary
}

// Error reporting functions
void errors::Error::raise() {
    print_banner("Error");
    print_error_message(message);
    print_source_context(source, st_line, end_line);
    print_suggested_fix(suggestedFix);
    std::cerr << std::string(50, '=') << "\n";
    exit(EXIT_FAILURE);
}

void errors::NoPrefixParseFnError::raise() {
    print_banner("No Prefix Parse Function Error");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines = {{token.line_no, std::string(token.col_no, ' ') + std::string(token.end_col_no - token.col_no, '^'), RED}};

    print_source_context(source, token.line_no, token.line_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::SyntaxError::raise() {
    print_banner("Syntax Error");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines = {{token.line_no, std::string(token.col_no, ' ') + std::string(token.end_col_no - token.col_no, '^'), RED}};

    print_source_context(source, token.line_no, token.line_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::NodeOutside::raise() {
    print_banner("Invalid Node Usage");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines = {
        {node.meta_data.st_line_no, std::string(node.meta_data.st_col_no, ' ') + std::string(node.meta_data.end_col_no - node.meta_data.st_col_no, '^'), RED}};

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
    print_banner("No Function Overload Found");
    print_error_message(message);

    // Extract the start and end lines of the function call
    int start_line = func_call->meta_data.st_line_no;
    int end_line = func_call->meta_data.end_line_no;

    // Prepare the underlines for mismatched arguments
    std::vector<std::tuple<int, std::string, std::string>> underlines;
    for (const auto& mismatch : this->missmatches) {
        for (int idx : mismatch) {
            auto callExpr = func_call->castToCallExpression();
            if (!callExpr || idx >= static_cast<int>(callExpr->arguments.size())) continue;
            auto arg = callExpr->arguments[idx];
            int line_no = arg->meta_data.st_line_no;
            int col_no = arg->meta_data.st_col_no;
            int end_col_no = arg->meta_data.end_col_no;
            std::string underline = std::string(col_no, ' ') + std::string(end_col_no - col_no, '^');
            underlines.emplace_back(line_no, underline, RED);
        }
    }

    // Print the source context with underlines
    print_source_context(source, start_line, end_line, underlines);
    print_suggested_fix(suggestedFix);

    exit(EXIT_FAILURE);
}

void errors::DosentContain::raise() {
    print_banner("Missing Member");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines;

    if (from) {
        std::string from_underline = std::string(from->meta_data.st_col_no, ' ') + std::string(from->meta_data.end_col_no - from->meta_data.st_col_no, '~');
        underlines.emplace_back(from->meta_data.st_line_no, from_underline, BLUE);
    }

    if (member) {
        std::string member_underline = std::string(member->meta_data.st_col_no, ' ') + std::string(member->meta_data.end_col_no - member->meta_data.st_col_no, '^');
        underlines.emplace_back(member->meta_data.st_line_no, member_underline, RED);
    }

    print_source_context(source, from->meta_data.st_line_no, member->meta_data.end_line_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::WrongInfix::raise() {
    print_banner("Invalid Operator Usage");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines = {
        {left->meta_data.st_line_no, std::string(left->meta_data.st_col_no, ' ') + std::string(right->meta_data.end_col_no - left->meta_data.st_col_no, '~'), RED}};

    print_source_context(source, left->meta_data.st_line_no, right->meta_data.end_line_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::WrongType::raise() {
    print_banner("Type Mismatch");
    print_error_message(message);

    if (!this->expected.empty()) {
        std::string expected_types;
        for (size_t i = 0; i < this->expected.size(); ++i) {
            expected_types += this->expected[i]->name;
            if (i < this->expected.size() - 1) { expected_types += " or "; }
        }
        std::cerr << "Expected type: " << GREEN << expected_types << RESET << "\n\n";
    }

    std::vector<std::tuple<int, std::string, std::string>> underlines = {
        {exp->meta_data.st_line_no, std::string(exp->meta_data.st_col_no, ' ') + std::string(exp->meta_data.end_col_no - exp->meta_data.st_col_no, '^'), RED}};

    print_source_context(source, exp->meta_data.st_line_no, exp->meta_data.end_line_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::Cantindex::raise() {
    print_banner("Indexing Error");
    print_error_message(message);

    std::vector<std::tuple<int, std::string, std::string>> underlines;

    if (auto indexExpr = exp->castToIndexExpression()) {
        // Underline the left expression
        std::string left_underline = std::string(indexExpr->left->meta_data.st_col_no, ' ') + std::string(indexExpr->left->meta_data.end_col_no - indexExpr->left->meta_data.st_col_no, '^');
        underlines.emplace_back(indexExpr->left->meta_data.st_line_no, left_underline, BLUE);

        // Underline the index expression if present
        if (indexExpr->index) {
            std::string index_underline = std::string(indexExpr->index->meta_data.st_col_no, ' ') + std::string(indexExpr->index->meta_data.end_col_no - indexExpr->index->meta_data.st_col_no, '^');
            underlines.emplace_back(indexExpr->index->meta_data.st_line_no, index_underline, RED);
        }
    }

    print_source_context(source, exp->meta_data.st_line_no, exp->meta_data.end_line_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::NotDefined::raise() {
    print_banner("Undefined Identifier");
    print_error_message(message);

    std::string underline = std::string(Name->meta_data.st_col_no, ' ') + std::string(Name->meta_data.end_col_no - Name->meta_data.st_col_no, '^');

    std::vector<std::tuple<int, std::string, std::string>> underlines = {{Name->meta_data.st_line_no, underline, RED}};

    print_source_context(source, Name->meta_data.st_line_no, Name->meta_data.end_line_no, underlines);
    print_suggested_fix(suggestedFix);
    exit(EXIT_FAILURE);
}

void errors::DuplicateVariableError::raise() {
    print_banner("Duplicate Variable Error");
    std::string errorMsg = "Variable '" + variableName + "' is already defined in this scope.";
    print_error_message(errorMsg);

    // Since line numbers are not provided, display a general suggestion
    print_suggested_fix("Consider renaming the variable or removing the duplicate declaration.");

    std::cerr << std::string(50, '=') << "\n";
    exit(EXIT_FAILURE);
}

void errors::UnknownNodeTypeError::raise() {
    print_banner("Unknown Node Type Error");
    std::string errorMsg = "Encountered an unknown node type: '" + type + "'.";
    print_error_message(errorMsg);

    // Assuming st_line and end_line are set correctly
    if (st_line != -1 && end_line != -1 && st_col != -1 && end_col != -1) {
        std::string underline = std::string(st_col, ' ') + std::string(end_col - st_col, '^');
        std::vector<std::tuple<int, std::string, std::string>> underlines = {{st_line, underline, RED}};
        print_source_context(source, st_line, end_line, underlines);
    }

    print_suggested_fix("Ensure that all node types are correctly handled in the compiler.");
    exit(EXIT_FAILURE);
}

void errors::ReturnTypeMismatchError::raise() {
    print_banner("Return Type Mismatch Error");
    std::string errorMsg = "Return type mismatch: Expected '" + expectedType->name + "'.";
    print_error_message(errorMsg);

    std::vector<std::tuple<int, std::string, std::string>> underlines = {
        {actualType->meta_data.st_line_no, std::string(actualType->meta_data.st_col_no, ' ') + std::string(actualType->meta_data.end_col_no - actualType->meta_data.st_col_no, '^'), RED}};

    print_source_context(source, actualType->meta_data.st_line_no, actualType->meta_data.end_line_no, underlines);
    print_suggested_fix("Ensure that the return statement matches the declared return type.");
    exit(EXIT_FAILURE);
}

void errors::GenericStructResolutionError::raise() {
    print_banner("Generic Struct Resolution Error");
    print_error_message(message);

    // Provide general advice as specific context might not be available
    print_suggested_fix("Ensure that all generic parameters are correctly specified and resolved.");

    std::cerr << std::string(50, '=') << "\n";
    exit(EXIT_FAILURE);
}

void errors::raiseSyntaxError(const std::string& type, const std::string& source, token::Token token, const std::string& message, const std::string& suggestedFix) {
    errors::SyntaxError error(type, source, token, message, suggestedFix);
    error.raise();
}