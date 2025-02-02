#ifndef ERRORS_HPP
#define ERRORS_HPP

#include "../compiler/enviornment/enviornment.hpp"
#include "../lexer/token.hpp"
#include "../parser/AST/ast.hpp"

#include <string>
#include <vector>

namespace errors {

/**
 * @brief Raise a "File Not Found" error.
 *
 * @param message The error message to display.
 * @param suggestedFix An optional suggested fix for the error.
 */
[[noreturn]] void raiseFileNotFoundError(const std::string& message, const std::string& suggestedFix = "");

/**
 * @brief Raise a "Compilation Error".
 *
 * @param message The error message to display.
 * @param suggestedFix An optional suggested fix for the error.
 */
[[noreturn]] void raiseCompilationError(const std::string& message, const std::string& suggestedFix = "");

/**
 * @brief Raise a generic runtime error.
 *
 * @param message The error message to display.
 * @param suggestedFix An optional suggested fix for the error.
 */
[[noreturn]] void raiseRuntimeError(const std::string& message, const std::string& suggestedFix = "");

/**
 * @brief Raise an unknown error with a custom label.
 *
 * @param label The label for the error (e.g., "Unknown Error").
 * @param message The error message to display.
 * @param suggestedFix An optional suggested fix for the error.
 */
[[noreturn]] void raiseUnknownError(const std::string& label, const std::string& message, const std::string& suggestedFix = "");

// Enum for different types of node usage outside allowed contexts
enum class OutsideNodeType { Break, Continue, Return };

/**
 * @brief General error raiser.
 *
 * @param file_path The path of the file where the error occurred.
 * @param source The entire source code as a string.
 * @param st_line Starting line number of the error.
 * @param st_col Starting column number of the error.
 * @param end_line Ending line number of the error.
 * @param end_col Ending column number of the error.
 * @param message The error message to display.
 * @param suggestedFix An optional suggested fix for the error.
 */
[[noreturn]] void
raiseError(const std::string& file_path, const std::string& source, int st_line, int st_col, int end_line, int end_col, const std::string& message, const std::string& suggestedFix = "");

/**
 * @brief Raise a completion-specific error.
 */
[[noreturn]] void raiseCompletionError(
    const std::string& file_path, const std::string& source, int st_line, int st_col, int end_line, int end_col, const std::string& message = "", const std::string& suggestedFix = "");

/**
 * @brief Raise an error when a node is used outside its allowed context.
 */
[[noreturn]] void
raiseNodeOutsideError(const std::string& file_path, const std::string& source, const AST::Node* node, OutsideNodeType nodeType, const std::string& message = "", const std::string& suggestedFix = "");

/**
 * @brief Raise a syntax error.
 */
[[noreturn]] void raiseSyntaxError(const std::string& file_path, const token::Token& token, const std::string& source, const std::string& message = "", const std::string& suggestedFix = "");

/**
 * @brief Raise an error when no prefix parse function is found.
 */
[[noreturn]] void raiseNoPrefixParseFnError(const std::string& file_path, const token::Token& token, const std::string& source, const std::string& message = "", const std::string& suggestedFix = "");

/**
 * @brief Raise an error when no function overload is found.
 */
[[noreturn]] void raiseNoOverloadError(const std::string& file_path,
                                       const std::string& source,
                                       const std::vector<std::vector<unsigned short>>& mismatches,
                                       AST::Expression* func_call,
                                       const std::string& message = "",
                                       const std::string& suggestedFix = "");

/**
 * @brief Raise an error when a member is missing in a struct or object.
 */
[[noreturn]] void raiseDoesntContainError(
    const std::string& file_path, const std::string& source, AST::IdentifierLiteral* member, AST::Expression* from, const std::string& message = "", const std::string& suggestedFix = "");

/**
 * @brief Raise an error for invalid operator usage.
 */
[[noreturn]] void raiseWrongInfixError(const std::string& file_path,
                                       const std::string& source,
                                       AST::Expression* left,
                                       AST::Expression* right,
                                       const std::string& op,
                                       const std::string& message = "",
                                       const std::string& suggestedFix = "");

/**
 * @brief Raise a type mismatch error.
 */
[[noreturn]] void raiseWrongTypeError(const std::string& file_path,
                                      const std::string& source,
                                      AST::Expression* exp,
                                      enviornment::RecordStructType* got,
                                      const std::vector<enviornment::RecordStructType*>& expected,
                                      const std::string& message = "",
                                      const std::string& suggestedFix = "",
                                      bool is_const = false);

/**
 * @brief Raise an indexing error.
 */
[[noreturn]] void
raiseCantIndexError(const std::string& file_path, const std::string& source, AST::IndexExpression* exp, bool wrongIDX, const std::string& message = "", const std::string& suggestedFix = "");

/**
 * @brief Raise an error for undefined identifiers.
 */
[[noreturn]] void raiseNotDefinedError(const std::string& file_path, const std::string& source, AST::Node* Name, const std::string& message = "", const std::string& suggestedFix = "");

/**
 * @brief Raise a duplicate variable declaration error.
 */
[[noreturn]] void
raiseDuplicateVariableError(const std::string& file_path, const std::string& source, const std::string& variableName, const AST::Node* declarationNode, const std::string& message = "");

/**
 * @brief Raise an error for unknown node types.
 */
[[noreturn]] void raiseUnknownNodeTypeError(const std::string& file_path,
                                            const std::string& source,
                                            const std::string& type,
                                            int st_line,
                                            int st_col,
                                            int end_line,
                                            int end_col,
                                            const std::string& message,
                                            const std::string& suggestedFix = "");

/**
 * @brief Raise an array type error.
 */
[[noreturn]] void raiseArrayTypeError(const std::string& file_path, const std::string& source, AST::Node* element, enviornment::RecordStructType* expected_type, const std::string& message = "");

/**
 * @brief Raise a generic struct resolution error.
 */
[[noreturn]] void raiseGenericStructResolutionError(const std::string& file_path, const std::string& source, const std::string& message = "", const std::string& suggestedFix = "");

} // namespace errors

#endif // ERRORS_HPP
