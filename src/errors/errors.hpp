#ifndef ERRORS_HPP
#define ERRORS_HPP
#include "../compiler/enviornment/enviornment.hpp"
#include "../lexer/token.hpp"
#include "../parser/AST/ast.hpp"
#include <memory>

namespace errors {

class Error {
  public:
    std::string source;
    int st_line;
    int end_line;
    std::string type;
    std::string message;
    std::string suggestedFix;
    Error(const std::string& type, const std::string& source, int st_line, int end_line, const std::string& message, const std::string& suggestedFix = "")
        : type(type), source(source), st_line(st_line), end_line(end_line), message(message), suggestedFix(suggestedFix) {}
    Error() {};
    [[noreturn]] virtual void raise();
};

class CompletionError : public Error {
  public:
    token::Token token;
    CompletionError(const std::string& type, const std::string& source, int st_line, int end_line, const std::string& message = "", const std::string& suggestedFix = "")
        : Error(type, source, st_line, end_line, message, suggestedFix) {};
    [[noreturn]] void raise() override;
};

enum class outsideNodeType { Break, Continue, Retuen };

class NodeOutside : public Error {
  public:
    AST::Node node;
    outsideNodeType nodeType;
    NodeOutside(const std::string& type, const std::string& source, AST::Node node, outsideNodeType nodeType, const std::string& message = "", const std::string& suggestedFix = "")
        : Error(type, source, node.meta_data.st_line_no, node.meta_data.end_line_no, message, suggestedFix), node(node), nodeType(nodeType) {}
    [[noreturn]] void raise() override;
};

class SyntaxError : public Error {
  public:
    token::Token token;
    SyntaxError(const std::string& type, const std::string& source, token::Token token, const std::string& message = "", const std::string& suggestedFix = "")
        : Error(type, source, -1, -1, message, suggestedFix), token(token) {}
    [[noreturn]] void raise() override;
};

class NoPrefixParseFnError : public Error {
  public:
    token::Token token;
    NoPrefixParseFnError(const std::string& source, token::Token token, const std::string& message = "", const std::string& suggestedFix = "")
        : Error("No PreficParseFnError", source, -1, -1, message, suggestedFix), token(token) {}
    [[noreturn]] void raise() override;
};

class NoOverload : public Error {
  public:
    std::vector<std::vector<unsigned short>> missmatches;
    AST::Expression* func_call;
    NoOverload(const std::string& source,
               std::vector<std::vector<unsigned short>> missmatches,
               AST::Expression* func_call,
               const std::string& message = "",
               const std::string& suggestedFix = "")
        : Error("No Fucntion Overload", source, -1, -1, message, suggestedFix), missmatches(missmatches), func_call(func_call) {};
    [[noreturn]] void raise() override;
};

class DosentContain : public Error {
  public:
    AST::IdentifierLiteral* member;
    AST::Expression* from;
    DosentContain(
        const std::string& source, AST::IdentifierLiteral* member, AST::Expression* from, const std::string& message = "", const std::string& suggestedFix = "")
        : Error("Dosent Contain", source, -1, -1, message, suggestedFix), member(member), from(from) {};
    [[noreturn]] void raise() override;
};

class WrongInfix : public Error {
  public:
    AST::Expression* left;
    AST::Expression* right;
    std::string op;
    WrongInfix(const std::string& source,
               AST::Expression* left,
               AST::Expression* right,
               const std::string& op,
               const std::string& message = "",
               const std::string& suggestedFix = "")
        : Error("Wrong infix", source, left->meta_data.st_line_no, right->meta_data.end_line_no, message, suggestedFix), left(left), right(right), op(op) {};
    [[noreturn]] void raise() override;
};

class WrongType : public Error {
  public:
    AST::Expression* exp;
    std::vector<std::shared_ptr<enviornment::RecordStructType>> expected;
    WrongType(const std::string& source,
              AST::Expression* exp,
              const std::vector<std::shared_ptr<enviornment::RecordStructType>>& expected,
              const std::string& message = "",
              const std::string& suggestedFix = "")
        : Error("Wrong Type", source, exp->meta_data.st_line_no, exp->meta_data.end_line_no, message, suggestedFix), exp(exp), expected(expected) {};
    [[noreturn]] void raise() override;
};

class Cantindex : public Error {
  public:
    AST::IndexExpression* exp;
    bool wrongIDX;
    Cantindex(const std::string& source, AST::IndexExpression* exp, bool wrongIDX, const std::string& message = "", const std::string& suggestedFix = "")
        : Error("Cantindex", source, exp->meta_data.st_line_no, exp->meta_data.end_line_no, message, suggestedFix), exp(exp), wrongIDX(wrongIDX) {}
    [[noreturn]] void raise() override;
};

class NotDefined : public Error {
  public:
    AST::IdentifierLiteral* Name;
    NotDefined(const std::string& source, AST::IdentifierLiteral* Name, const std::string& message = "", const std::string& suggestedFix = "")
        : Error("Not Defined", source, -1, -1, message, suggestedFix), Name(Name) {}
    [[noreturn]] void raise() override;
};

class DuplicateVariableError : public Error {
  public:
    std::string variableName;

    /**
     * @brief Constructs a DuplicateVariableError.
     * @param source The source code where the error occurred.
     * @param variableName The name of the duplicated variable.
     * @param message An optional error message.
     */
    DuplicateVariableError(const std::string& source, const std::string& variableName, const std::string& message = "")
        : Error("DuplicateVariableError", source, -1, -1, message), variableName(variableName) {}

    /**
     * @brief Raises the DuplicateVariableError.
     */
    [[noreturn]] void raise() override;
};

// New Custom Error Type
class UnknownNodeTypeError : public Error {
  public:
    int st_col = -1;
    int end_col = -1;
    UnknownNodeTypeError(const std::string& type, const std::string& source, int st_line, int st_col, int end_line, int end_col, const std::string& message, const std::string& suggestedFix = "")
        : Error(type, source, st_line, end_line, message, suggestedFix) {}
    [[noreturn]] void raise() override;
};

class ArrayTypeError : public Error {
  public:
    /**
     * @brief Constructs an ArrayTypeError.
     * @param source The source code where the error occurred.
     * @param element The AST node related to the error.
     * @param expected_type The expected struct type for array elements.
     * @param message An optional error message.
     */
    ArrayTypeError(const std::string& source, AST::Node* element, enviornment::StructTypePtr expected_type, const std::string& message = "")
        : Error("ArrayTypeError", source, element->meta_data.st_line_no, element->meta_data.end_line_no, message), element(element), expected_type(expected_type) {}

    /**
     * @brief Raises the ArrayTypeError by throwing an exception.
     */
    [[noreturn]] void raise() override { throw *this; }

    // Additional members if needed
  private:
    AST::Node* element;       ///< The AST node that caused the error
    enviornment::StructTypePtr expected_type; ///< The expected type for the array elements
};

class ReturnTypeMismatchError : public Error {
  public:
    enviornment::StructTypePtr expectedType;
    AST::Node* actualType;

    /**
     * @brief Constructs a ReturnTypeMismatchError.
     * @param source The source code where the error occurred.
     * @param expected The expected return type.
     * @param actual The actual return type provided.
     * @param message An optional error message.
     */
    ReturnTypeMismatchError(const std::string& source, enviornment::StructTypePtr expected, AST::Node* actual, const std::string& message = "")
        : Error("ReturnTypeMismatchError", source, -1, -1, message), expectedType(expected), actualType(actual) {}

    /**
     * @brief Raises the ReturnTypeMismatchError.
     */
    [[noreturn]] void raise() override;
};

class GenericStructResolutionError : public Error {
  public:
    GenericStructResolutionError(const std::string& source, const std::string& message = "", const std::string& suggestedFix = "")
        : Error("GenericStructResolutionError", source, -1, -1, message, suggestedFix) {}
    [[noreturn]] void raise() override;
};

[[noreturn]] void raiseSyntaxError(const std::string& type, const std::string& source, token::Token token, const std::string& message, const std::string& suggestedFix);
} // namespace errors
#endif // ERRORS_HPPast