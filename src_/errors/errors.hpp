#ifndef ERRORS_HPP
#define ERRORS_HPP
#include "../lexer/lexer.hpp"
#include "../lexer/tokens.hpp"
#include "../parser/AST/ast.hpp"
#include <iostream>

namespace errors {

class Error {
  public:
    std::string source;
    int st_line;
    int end_line;
    std::string message;
    std::string suggestedFix;
    Error(const std::string& source, int st_line, int end_line, const std::string& message, const std::string& suggestedFix = "")
        : source(source), st_line(st_line), end_line(end_line), message(message), suggestedFix(suggestedFix) {}
    Error() {};
    virtual void raise(bool terminate = true);
};

class SyntaxError : public Error {
  public:
    token::Token token;
    SyntaxError(const std::string& source, const token::Token& token, const std::string& message = "", const std::string& suggestedFix = "")
        : Error(source, -1, -1, message, suggestedFix), token(token) {}
    void raise(bool terminate = true) override;
};

class CompletionError : public Error {
  public:
    CompletionError(const std::string& source, int st_line, int end_line, const std::string& message = "", const std::string& suggestedFix = "")
        : Error(source, st_line, end_line, message, suggestedFix) {}
    void raise(bool terminate = true) override;
};

class VariableRedeclarationError : public Error {
  public:
    AST::MetaData previous_var_dec_meta_data;
    AST::VariableDeclarationStatement re_var_dec_node;
    VariableRedeclarationError(const std::string& source, AST::MetaData previous_var_dec_meta_data, AST::VariableDeclarationStatement re_var_dec_node,
                               const std::string& suggestedFix = "")
        : Error(source, -1, -1, "", ""), re_var_dec_node(re_var_dec_node), previous_var_dec_meta_data(previous_var_dec_meta_data) {}
    void raise(bool terminate = true) override;
};

class UndefinedVariableError : public Error {
  public:
    AST::MetaData meta_data;
    UndefinedVariableError(const std::string& source, AST::MetaData meta_data, const std::string& message = "", const std::string& suggestedFix = "")
        : Error(source, meta_data.st_line_no, meta_data.end_line_no, message, suggestedFix) {
        this->meta_data = meta_data;
    }
    void raise(bool terminate = true) override;
};

class UndefinedFunctionError : public Error {
  public:
    AST::MetaData meta_data;
    UndefinedFunctionError(const std::string& source, AST::MetaData meta_data, const std::string& message = "", const std::string& suggestedFix = "")
        : Error(source, meta_data.st_line_no, meta_data.end_line_no, message, suggestedFix) {
        this->meta_data = meta_data;
    }
    void raise(bool terminate = true) override;
};

class InternalCompilationError : public Error {
  public:
    std::string name;
    int st_col_no;
    InternalCompilationError(const std::string& name, const std::string& source, int st_line, int end_line, const std::string& message = "")
        : Error(source, st_line, end_line, message) {
        this->name = name;
    }
    void raise(bool terminate = true) override;
};

class UnsupportedOperationError : public Error {
  public:
    AST::MetaData meta_data;
    UnsupportedOperationError(const std::string& source, AST::MetaData meta_data, const std::string& message = "",
                              const std::string& suggestedFix = "")
        : Error(source, meta_data.st_line_no, meta_data.end_line_no, message, suggestedFix) {
        this->meta_data = meta_data;
    }
    void raise(bool terminate = true) override;
};

class NoPrefixParseFnError : public Error {
  public:
    token::Token token;
    NoPrefixParseFnError(const std::string& source, const token::Token& token, const std::string& message = "", const std::string& suggestedFix =
    "")
        : Error(source, -1, -1, message, suggestedFix), token(token) {}
    void raise(bool terminate = true) override;
};

void raiseSyntaxError(const std::string& source, const token::Token& token, const std::string& message, const std::string& suggestedFix);
} // namespace errors
#endif // ERRORS_HPP