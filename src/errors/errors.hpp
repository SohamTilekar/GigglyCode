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
    virtual void raise();
};

class CompletionError : public Error {
  public:
    token::Token token;
    CompletionError(const std::string& type, const std::string& source, int st_line, int end_line, const std::string& message = "", const std::string& suggestedFix = "")
        : Error(type, source, st_line, end_line, message, suggestedFix) {};
    void raise() override;
};

enum class outsideNodeType { Break, Continue, Retuen };

class NodeOutside : public Error {
  public:
    AST::Node node;
    outsideNodeType nodeType;
    NodeOutside(const std::string& type, const std::string& source, AST::Node node, outsideNodeType nodeType, const std::string& message = "", const std::string& suggestedFix = "")
        : Error(type, source, node.meta_data.st_line_no, node.meta_data.end_line_no, message, suggestedFix), node(node), nodeType(nodeType) {}
    void raise() override;
};

class SyntaxError : public Error {
  public:
    token::Token token;
    SyntaxError(const std::string& type, const std::string& source, const token::Token& token, const std::string& message = "", const std::string& suggestedFix = "")
        : Error(type, source, -1, -1, message, suggestedFix), token(token) {}
    void raise() override;
};

class NoPrefixParseFnError : public Error {
  public:
    token::Token token;
    NoPrefixParseFnError(const std::string& source, const token::Token& token, const std::string& message = "", const std::string& suggestedFix = "")
        : Error("No PreficParseFnError", source, -1, -1, message, suggestedFix), token(token) {}
    void raise() override;
};

class NoOverload : public Error {
  public:
    std::vector<std::vector<unsigned short>> missmatches;
    std::shared_ptr<AST::Expression> func_call;
    NoOverload(const std::string& source, std::vector<std::vector<unsigned short>> missmatches, std::shared_ptr<AST::Expression> func_call, const std::string& message = "",
               const std::string& suggestedFix = "")
        : Error("No Fucntion Overload", source, -1, -1, message, suggestedFix), missmatches(missmatches), func_call(func_call) {};
    void raise() override;
};

class DosentContain : public Error {
  public:
    std::shared_ptr<AST::IdentifierLiteral> member;
    std::shared_ptr<AST::Expression> from;
    DosentContain(const std::string& source, std::shared_ptr<AST::IdentifierLiteral> member, std::shared_ptr<AST::Expression> from, const std::string& message = "",
                  const std::string& suggestedFix = "")
        : Error("Dosent Contain", source, -1, -1, message, suggestedFix), member(member), from(from) {};
    void raise() override;
};

class WrongInfix : public Error {
  public:
    std::shared_ptr<AST::Expression> left;
    std::shared_ptr<AST::Expression> right;
    std::string op;
    WrongInfix(const std::string& source, std::shared_ptr<AST::Expression> left, std::shared_ptr<AST::Expression> right, const std::string& op, const std::string& message = "",
               const std::string& suggestedFix = "")
        : Error("Wrong infix", source, left->meta_data.st_line_no, right->meta_data.end_line_no, message, suggestedFix), left(left), right(right), op(op) {};
    void raise() override;
};

class WrongType : public Error {
  public:
    std::shared_ptr<AST::Expression> exp;
    std::vector<std::shared_ptr<enviornment::RecordStructType>> expected;
    WrongType(const std::string& source, std::shared_ptr<AST::Expression> exp, std::vector<std::shared_ptr<enviornment::RecordStructType>> expected, const std::string& message = "",
              const std::string& suggestedFix = "")
        : Error("Wrong Type", source, exp->meta_data.st_line_no, exp->meta_data.end_line_no, message, suggestedFix), exp(exp), expected(expected) {};
    void raise() override;
};

class Cantindex : public Error {
  public:
    std::shared_ptr<AST::IndexExpression> exp;
    bool wrongIDX;
    Cantindex(const std::string& source, std::shared_ptr<AST::IndexExpression> exp, bool wrongIDX, const std::string& message = "", const std::string& suggestedFix = "")
        : Error("Cantindex", source, exp->meta_data.st_line_no, exp->meta_data.end_line_no, message, suggestedFix), exp(exp), wrongIDX(wrongIDX) {}
    void raise() override;
};

class NotDefined : public Error {
  public:
    std::shared_ptr<AST::IdentifierLiteral> Name;
    NotDefined(const std::string& source, const std::shared_ptr<AST::IdentifierLiteral> Name, const std::string& message = "", const std::string& suggestedFix = "")
        : Error("Not Defined", source, -1, -1, message, suggestedFix), Name(Name) {}
    void raise() override;
};

void raiseSyntaxError(const std::string& type, const std::string& source, const token::Token& token, const std::string& message, const std::string& suggestedFix);
} // namespace errors
#endif // ERRORS_HPP