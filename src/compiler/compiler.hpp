#ifndef COMPILER_HPP
#define COMPILER_HPP

/**
 * @file compiler.hpp
 * @brief Defines the Compiler class responsible for compiling the AST into LLVM IR.
 *
 * This header file declares the Compiler class within the `compiler` namespace.
 * The Compiler class handles the compilation process, including type conversion,
 * AST traversal, and LLVM IR generation.
 */

// === LLVM Headers ===o
#include <filesystem>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <vector>

// === Project-specific Headers ===
#include "../compilation_state.hpp"
#include "../parser/AST/ast.hpp"
#include "./enviornment/enviornment.hpp"

namespace compiler {

/**
 * @brief Alias for the environment namespace used within the compiler.
 */
using namespace enviornment;

// === Type Aliases ===

using std::vector;
using GenericStructTypeVector = vector<RecordGenericStructType*>;                                     ///< Vector of generic struct type pointers
using ResolvedValueVariant = std::variant<GenericStructTypeVector, RecordModule*, RecordStructType*>; ///< Variant type for resolved values
using LLVMValueVector = vector<llvm::Value*>;                                                         ///< Vector of LLVM values
using StructTypeVector = vector<RecordStructType*>;                                                   ///< Vector of struct type pointers
using GenericFunctionVector = vector<RecordGenericFunction*>;                                         ///< Vector of generic function pointers
using llBB = llvm::BasicBlock;                                                                        ///< Alias for LLVM BasicBlock

/**
 * @enum resolveType
 * @brief Enumeration of different types used during type resolution.
 */
enum class resolveType {
    Module,      ///< Represents a module
    StructInst,  ///< Represents a struct instance
    StructType,  ///< Represents a struct type
    GStructType, ///< Represents a generic struct type
};

/**
 * @class NotCompiledError
 * @brief Exception thrown when a file has not been compiled.
 */
class NotCompiledError : public std::exception {
  public:
    Str path; ///< Path of the file that was not compiled

    /**
     * @brief Constructs a NotCompiledError with the given file path.
     * @param path The path of the uncompiled file.
     */
    NotCompiledError(const Str& path) : path(path) {}

    /**
     * @brief Retrieves the error message.
     * @return C-string containing the error message.
     */
    const char* what() const noexcept override {
        static Str error_message;
        error_message = "File " + path + " is not compiled";
        return error_message.c_str();
    }
};

/**
 * @class DoneRet
 * @brief Exception indicating that a return statement was not properly handled.
 */
class DoneRet : public std::exception {
  public:
    /**
     * @brief Constructs a DoneRet exception.
     */
    DoneRet() {}

    /**
     * @brief Retrieves the error message.
     * @return C-string containing the error message.
     */
    const char* what() const noexcept override { return "Return should be caught in if-else & while, but it was not (InternalCompilationError)."; }
};

/**
 * @class DoneBr
 * @brief Exception indicating that a branch statement was not properly handled.
 */
class DoneBr : public std::exception {
  public:
    /**
     * @brief Constructs a DoneBr exception.
     */
    DoneBr() {}

    /**
     * @brief Retrieves the error message.
     * @return C-string containing the error message.
     */
    const char* what() const noexcept override { return "Branching should be caught in if-else & while, but it was not (InternalCompilationError)."; }
};

/**
 * @class Compiler
 * @brief Main class responsible for compiling the abstract syntax tree (AST) into LLVM IR.
 *
 * The Compiler class traverses the AST, performs type checking and conversion, manages
 * the LLVM context and module, and generates the corresponding LLVM IR code.
 */
class Compiler {
  public:
    // === Aliases ===

    /**
     * @struct ResolvedValue
     * @brief Represents a value that has been resolved during compilation.co
     */
    struct ResolvedValue {
        llvm::Value* value;           ///< The LLVM value
        llvm::Value* alloca;          ///< The allocation instruction
        ResolvedValueVariant variant; ///< Variant holding additional resolved data
        resolveType type;             ///< Type of the resolved value
    };

    // === Constructors ===

    /**
     * @brief Constructs a Compiler instance.
     * @param source The source code to compile.
     * @param file_path The file path of the source code.
     * @param ir_gc_map The path to the IR-GC map file.
     * @param buildDir The build directory path.
     * @param relativePath The relative path of the source file.
     */
    Compiler(const Str& source, const std::filesystem::path& file_path, compilationState::RecordFile* file_record, const std::filesystem::path& buildDir, const std::filesystem::path& relativePath);

    ~Compiler();

    // === Public Methods ===

    /**
     * @brief Starts the compilation process for the given AST node.
     * @param to the root AST node.
     */
    void compile(AST::Node* node);

    /**
     * @brief Converts a type from one struct type to another.
     * @param from Tuple containing LLVM values and the source struct type.
     * @param to The target struct type.
     * @return The resolved value after conversion.
     */
    ResolvedValue convertType(const ResolvedValue& from, RecordStructType* to);

    /**
     * @brief Checks if one struct type can be converted to another.
     * @param from The source struct type.
     * @param to The target struct type.
     * @return True if conversion is possible, false otherwise.
     */
    static bool canConvertType(RecordStructType* from, RecordStructType* to);

    /**
     * @brief Determines the precedence of type conversion between two struct types.
     * @param from The source struct type.
     * @param to The target struct type.
     * @return Boolean indicating the precedence.
     */
    bool conversionPrecidence(RecordStructType* from, RecordStructType* to);

    // --- LLVM Components ---
    llvm::LLVMContext llvm_context;            ///< LLVM context
    std::unique_ptr<llvm::Module> llvm_module; ///< LLVM module; For code output purposes it is public
    llvm::IRBuilder<> llvm_ir_builder;         ///< LLVM IR builder

    compilationState::RecordFile* file_record;

  private:
    // === Member Variables ===

    // --- Source Information ---
    Str source;                         ///< Source code as a string
    std::filesystem::path buildDir;     ///< Build directory path
    std::filesystem::path relativePath; ///< Relative path of the source file
    std::filesystem::path file_path;    ///< Full path of the source file


    // --- Naming Prefixes ---
    Str fc_st_name_prefix; ///< Prefix for function and struct names

    // --- Compiler Environment ---
    Enviornment* env; ///< Pointer to the compiler environment

    // --- Function Entry Blocks ---
    vector<llBB*> function_entry_block; ///< Entry blocks for functions

    // --- LLVM Type Pointers ---
    llvm::PointerType* ll_pointer = nullptr; ///< LLVM pointer type
    llvm::Type* ll_int = nullptr;            ///< LLVM integer type
    llvm::Type* ll_int32 = nullptr;          ///< LLVM 32-bit integer type
    llvm::Type* ll_void = nullptr;           ///< LLVM void type
    llvm::Type* ll_uint = nullptr;           ///< LLVM unsigned integer type
    llvm::Type* ll_uint32 = nullptr;         ///< LLVM 32-bit unsigned integer type
    llvm::Type* ll_float = nullptr;          ///< LLVM float type
    llvm::Type* ll_float32 = nullptr;        ///< LLVM 32-bit float type
    llvm::Type* ll_char = nullptr;           ///< LLVM char type
    llvm::Type* ll_str = nullptr;            ///< LLVM string type
    llvm::Type* ll_bool = nullptr;           ///< LLVM boolean type
    llvm::PointerType* ll_raw_array = nullptr;   ///< LLVM raw_array pointer type
    llvm::StructType* ll_array = nullptr;
    // --- Garbage-Collected Struct Types ---
    RecordStructType* gc_int = nullptr;     ///< GC wrapper for int
    RecordStructType* gc_int32 = nullptr;   ///< GC wrapper for int32
    RecordStructType* gc_void = nullptr;    ///< GC wrapper for void
    RecordStructType* gc_uint = nullptr;    ///< GC wrapper for uint
    RecordStructType* gc_uint32 = nullptr;  ///< GC wrapper for uint32
    RecordStructType* gc_float = nullptr;   ///< GC wrapper for float
    RecordStructType* gc_float32 = nullptr; ///< GC wrapper for float32
    RecordStructType* gc_char = nullptr;    ///< GC wrapper for char
    RecordStructType* gc_str = nullptr;     ///< GC wrapper for string
    RecordStructType* gc_bool = nullptr;    ///< GC wrapper for boolean
    RecordStructType* gc_raw_array = nullptr;   ///< GC wrapper for raw_array

    std::vector<AST::Program*> auto_free_programs;
    std::vector<RecordStructType*> auto_free_recordStructType;

    // === Private Methods ===

    // --- Initialization Methods ---

    /**
     * @brief Initializes built-in types and functions.
     */
    void _initializeBuiltins();

    /**
     * @brief Initializes the LLVM module with the given path.
     * @param path_str The path string for the LLVM module.
     */
    void _initializeLLVMModule(const Str& path_str);

    /**
     * @brief Initializes the compiler environment.
     */
    void _initializeEnvironment();

    void _initilizeCSTDLib();

    void addFunc2Mod(RecordModule* module,
                                    const Str& name,
                                    const Str& llvm_name,
                                    llvm::FunctionType* funcType,
                                    vector<std::tuple<Str, RecordStructType*, bool>>& params,
                                    RecordStructType* returnType,
                                    bool isVarArg);

    void addFunc(const Str& name, const Str& llvm_name, llvm::FunctionType* funcType, vector<std::tuple<Str, RecordStructType*, bool>>& params, RecordStructType* returnType, bool isVarArg);

    void initilizeArray();

    // --- AST Visitor Methods ---

    /**
     * @brief Visits a Program node in the AST.
     * @param program pointer to the Program node.
     */
    void _visitProgram(AST::Program* program);

    /**
     * @brief Visits an ExpressionStatement node in the AST.
     * @param expression_statement pointer to the ExpressionStatement node.
     */
    void _visitExpressionStatement(AST::ExpressionStatement* expression_statement);

    /**
     * @brief Visits a VariableDeclarationStatement node in the AST.
     * @param variable_declaration_statement pointer to the VariableDeclarationStatement node.
     */
    void _visitVariableDeclarationStatement(AST::VariableDeclarationStatement* variable_declaration_statement);

    /**
     * @brief Visits a VariableAssignmentStatement node in the AST.
     * @param variable_assignment_statement pointer to the VariableAssignmentStatement node.
     */
    void _visitVariableAssignmentStatement(AST::VariableAssignmentStatement* variable_assignment_statement);

    /**
     * @brief Visits an IfElseStatement node in the AST.
     * @param if_statement pointer to the IfElseStatement node.
     */
    void _visitIfElseStatement(AST::IfElseStatement* if_statement);

    /**
     * @brief Visits a FunctionDeclarationStatement node in the AST.
     * @param function_declaration_statement pointer to the FunctionDeclarationStatement node.
     * @param struct_ Optional struct type if the function is a method.
     */
    void _visitFunctionDeclarationStatement(AST::FunctionStatement* function_declaration_statement, RecordStructType* struct_ = nullptr);

    /**
     * @brief Visits a ReturnStatement node in the AST.
     * @param return_statement pointer to the ReturnStatement node.
     */
    void _visitReturnStatement(AST::ReturnStatement* return_statement);

    /**
     * @brief Visits a RaiseStatement node in the AST.
     * @param raise_statement pointer to the RaiseStatement node.
     */
    void _visitRaiseStatement(AST::RaiseStatement* raise_statement);

    /**
     * @brief Visits a BlockStatement node in the AST.
     * @param block_statement pointer to the BlockStatement node.
     */
    void _visitBlockStatement(AST::BlockStatement* block_statement);

    /**
     * @brief Visits a WhileStatement node in the AST.
     * @param while_statement pointer to the WhileStatement node.
     */
    void _visitWhileStatement(AST::WhileStatement* while_statement);

    /**
     * @brief Visits a ForStatement node in the AST.
     * @param for_statement pointer to the ForStatement node.
     */
    void _visitForEachStatement(AST::ForEachStatement* for_statement);

    void _visitForStatement(AST::ForStatement* for_statement);

    /**
     * @brief Visits a StructStatement node in the AST.
     * @param struct_statement pointer to the StructStatement node.
     */
    void _visitStructStatement(AST::StructStatement* struct_statement);

    /**
     * @brief Visits a SwitchCaseStatement node in the AST.
     * @param switch_statement pointer to the SwitchCaseStatement node.
     */
    void _visitSwitchCaseStatement(AST::SwitchCaseStatement* switch_statement);

    /**
     * @brief Visits a TryCatchStatement node in the AST.
     * @param tc_statement pointer to the TryCatchStatement node.
     */
    void _visitTryCatchStatement(AST::TryCatchStatement* tc_statement);

    /**
     * @brief Visits an ImportStatement node in the AST.
     * @param import_statement pointer to the ImportStatement node.
     * @param module Optional module pointer if importing into a module.
     */
    void _visitImportStatement(AST::ImportStatement* import_statement, RecordModule* module = nullptr);

    /**
     * @brief Visits a BreakStatement node in the AST.
     * @param node pointer to the BreakStatement node.
     */
    void _visitBreakStatement(AST::BreakStatement* node);

    /**
     * @brief Visits a ContinueStatement node in the AST.
     * @param node pointer to the ContinueStatement node.
     */
    void _visitContinueStatement(AST::ContinueStatement* node);

    // --- Expression Visitor Methods ---

    /**
     * @brief Visits an InfixExpression node in the AST.
     * @param infix_expression pointer to the InfixExpression node.
     * @return The resolved value of the expression.
     */
    ResolvedValue _visitInfixExpression(AST::InfixExpression* infix_expression);

    /**
     * @brief Visits an IndexExpression node in the AST.
     * @param index_expression pointer to the IndexExpression node.
     * @return The resolved value of the expression.
     */
    ResolvedValue _visitIndexExpression(AST::IndexExpression* index_expression);

    /**
     * @brief Visits a CallExpression node in the AST.
     * @param call_expression pointer to the CallExpression node.
     * @return The resolved value of the expression.
     */
    ResolvedValue _visitCallExpression(AST::CallExpression* call_expression);

    /**
     * @brief Visits an raw_arrayLiteral node in the AST.
     * @param raw_array_literal pointer to the raw_arrayLiteral node.
     * @return The resolved value of the raw_array literal.
     */
    ResolvedValue _visitArrayLiteral(AST::ArrayLiteral* raw_array_literal);

    /**
     * @brief Resolves the value of a given AST node.
     * @param node pointer to the AST node.
     * @return The resolved value of the node.
     */
    ResolvedValue _resolveValue(AST::Node* node);

    /**
     * @brief Handles member access expressions in the AST.
     * @param infix_expression pointer to the InfixExpression node representing member access.
     * @return The resolved value after member access.
     */
    ResolvedValue _memberAccess(AST::InfixExpression* infix_expression);

    // --- Import Utilities ---

    /**
     * @brief Imports a function declaration into a module.
     * @param function_declaration_statement pointer to the FunctionDeclarationStatement node.
     * @param module Pointer to the target module.
     * @param ir_gc_map_json Reference to the IR-GC map JSON object.
     */
    void _importFunctionDeclarationStatement(AST::FunctionStatement* function_declaration_statement, RecordModule* module, compilationState::RecordFile* local_file_record);

    /**
     * @brief Imports a struct declaration into a module.
     * @param struct_statement pointer to the StructStatement node.
     * @param module Pointer to the target module.
     * @param ir_gc_map_json Reference to the IR-GC map JSON object.
     */
    void _importStructStatement(AST::StructStatement* struct_statement, RecordModule* module, compilationState::RecordFile* local_file_record);

    // --- Type Parsing and Conversion ---

    /**
     * @brief Parses a type from the AST.
     * @param type pointer to the Type node.
     * @return Pointer to the parsed StructType.
     */
    RecordStructType* _parseType(AST::Type* type);

    /**
     * @brief Handles infix operations involving struct types.
     * @param op_method The method name for the operation.
     * @param op The operator as a string.
     * @param left_type The type of the left operand.
     * @param right_type The type of the right operand.
     * @param left pointer to the left expression.
     * @param right pointer to the right expression.
     * @param left_value LLVM value of the left operand.
     * @param right_value LLVM value of the right operand.
     * @return The resolved value after the operation.
     */
    ResolvedValue _StructInfixCall(const Str& op_method,
                                   const Str& op,
                                   RecordStructType* left_type,
                                   RecordStructType* right_type,
                                   AST::Expression* left,
                                   AST::Expression* right,
                                   llvm::Value* left_value,
                                   llvm::Value* left_alloca,
                                   llvm::Value* right_value,
                                   llvm::Value* right_alloca);

    /**
     * @brief Manages a function call within the compiler.
     * @param func_record Pointer to the function record.
     * @param func_call pointer to the CallExpression node.
     * @param args Vector of LLVM values representing the arguments.
     * @param params_types Vector of struct types representing parameter types.
     * @return The resolved value after the function call.
     */
    ResolvedValue _manageFuncCall(RecordFunction* func_record, AST::CallExpression* func_call, LLVMValueVector args, const StructTypeVector& params_types);

    /**
     * @brief Checks the types of arguments in a function call.
     * @param func_record Pointer to the function record.
     * @param func_call pointer to the CallExpression node.
     * @param args Reference to the vector of LLVM values representing the arguments.
     * @param params_types Vector of struct types representing parameter types.
     */
    void _checkAndConvertCallType(std::vector<RecordFunction*> func_record, AST::CallExpression* func_call, LLVMValueVector& args, const StructTypeVector& params_types);

    /**
     * @brief Calls a generic function.
     * @param gfuncs Vector of generic function pointers.
     * @param func_call pointer to the CallExpression node.
     * @param name The name of the function.
     * @param args Reference to the vector of LLVM argument values.
     * @param params_types Vector of struct types representing parameter types.
     * @return The resolved value after the function call.
     */
    ResolvedValue _CallGfunc(const vector<RecordGenericFunction*>& gfuncs, AST::CallExpression* func_call, const Str& name, LLVMValueVector& args, const StructTypeVector& params_types);

    /**
     * @brief Calls a generic struct method.
     * @param gstructs Vector of generic struct type pointers.
     * @param func_call pointer to the CallExpression node.
     * @param name The name of the struct method.
     * @param args Reference to the vector of LLVM argument values.
     * @param params_types Vector of struct types representing parameter types.
     * @return The resolved value after the struct method call.
     */
    ResolvedValue _CallGstruct(const vector<RecordGenericStructType*>& gstructs, AST::CallExpression* func_call, const Str& name, LLVMValueVector& args, const StructTypeVector& params_types);

    // --- Helper Methods ---

    /**
     * @brief Extracts a prefix from a given path string.
     * @param path_str The path string.
     * @return The extracted prefix.
     */
    Str extractPrefix(const Str& path_str);

    /**
     * @brief Replaces delimiters in a given string with appropriate characters.
     * @param prefix Reference to the string whose delimiters are to be replaced.
     */
    void replaceDelimiters(Str& prefix);

    /**
     * @brief Creates a function record in the compiler environment.
     * @param function_declaration_statement pointer to the FunctionDeclarationStatement node.
     * @param struct_ Optional struct type if the function is a method.
     * @param module Optional pointer to the module where the function is declared.
     * @param ir_gc_map_json Optional JSON object for IR-GC mapping.
     */
    void _createFunctionRecord(AST::FunctionStatement* function_declaration_statement,
                               RecordStructType* struct_ = nullptr,
                               RecordModule* module = nullptr,
                               compilationState::RecordFile* local_file_record = nullptr);

    /**
     * @brief Creates a struct record in the compiler environment.
     * @param struct_statement pointer to the StructStatement node.
     * @param module Pointer to the module where the struct is declared.
     * @param ir_gc_map_json Reference to the IR-GC map JSON object.
     */
    void _createStructRecord(AST::StructStatement* struct_statement, RecordModule* module, compilationState::RecordFile* local_file_record = nullptr);

    /**
     * @brief Handles calling a struct method.
     * @param struct_record Pointer to the struct type record.
     * @param call_expression pointer to the CallExpression node.
     * @param params_types Vector of struct types representing parameter types.
     * @param args Vector of LLVM values representing the arguments.
     * @return The resolved value after the struct method call.
     */
    ResolvedValue _callStruct(RecordStructType* struct_record, AST::CallExpression* call_expression, vector<RecordStructType*> params_types, LLVMValueVector args);

    /**
     * @brief Processes a function declaration within a struct.
     * @param field The AST node for the field.
     * @param struct_record The record for the struct.
     * @param ir_gc_map_json The JSON object for IR-GC mapping.
     */
    void _processFieldFunction(AST::Node* field, RecordStructType* struct_record, compilationState::RecordFile* local_file_record = nullptr);

    /**
     * @brief Handles generic subtypes for a field.
     * @param field The AST node for the field.
     * @param struct_statement The struct's AST statement.
     * @param struct_record The record for the struct.
     * @param field_type The parsed type of the field.
     */
    void _handleGenericSubType(AST::Node* field, AST::StructStatement* struct_statement, RecordStructType* struct_record, RecordStructType* field_type);

    /**
     * @brief Processes a variable declaration within a struct.
     * @param field The AST node for the field.
     * @param struct_statement The struct's AST statement.
     * @param struct_record The record for the struct.
     * @param field_types The vector of LLVM field types being built.
     */
    void _processFieldDeclaration(AST::Node* field, AST::StructStatement* struct_statement, RecordStructType* struct_record, vector<llvm::Type*>& field_types);

    /**
     * @brief Resolves and validates the left operand of the index expression.
     * @param index_expression pointer to the IndexExpression node.
     * @return A tuple containing the loaded LLVM value, allocation LLVM value,
     *         the generic struct type pointer, and the resolve type.
     */
    Compiler::ResolvedValue _resolveAndValidateLeftOperand(AST::IndexExpression* index_expression);

    /**
     * @brief Resolves and validates the index operand of the index expression.
     * @param index_expression pointer to the IndexExpression node.
     * @return A tuple containing the loaded LLVM value, allocation LLVM value,
     *         the generic struct type pointer, and the resolve type.
     */
    Compiler::ResolvedValue _resolveAndValidateIndexOperand(AST::IndexExpression* index_expression);

    /**
     * @brief Handles raw_array indexing logic.
     * @param left Loaded LLVM value of the raw_array.
     * @param left_generic RecordStructType* of the raw_array.
     * @param index Loaded LLVM value of the index.
     * @param index_generic RecordStructType* of the index.
     * @param index_expression pointer to the IndexExpression node.
     * @return ResolvedValue containing the result of the indexing operation.
     */
    ResolvedValue _handleraw_arrayIndexing(llvm::Value* left, RecordStructType* left_generic, llvm::Value* index, RecordStructType* index_generic, AST::IndexExpression* index_expression);

    /**
     * @brief Handles struct indexing logic by invoking the `__index__` method.
     * @param left_alloca Allocation LLVM value of the struct.
     * @param index Loaded LLVM value of the index.
     * @param index_generic RecordStructType* of the index.
     * @param left_generic RecordStructType* of the struct being indexed.
     * @param index_expression pointer to the IndexExpression node.
     * @return ResolvedValue containing the result of the `__index__` method call.
     */
    ResolvedValue _handleStructIndexing(llvm::Value* left_alloca, llvm::Value* index, RecordStructType* index_generic, RecordStructType* left_generic, AST::IndexExpression* index_expression);

    /**
     * @brief Raises a NoOverload error indicating the absence of the `__index__` method.
     * @param left_generic RecordStructType* of the struct being indexed.
     * @param index_expression pointer to the IndexExpression node.
     */
    [[noreturn]] void _raiseNoIndexMethodError(RecordStructType* left_generic, AST::IndexExpression* index_expression);

    // --- Resolve Functions for AST Node Types ---

    /**
     * @brief Resolves an IntegerLiteral node.
     * @param integer_literal pointer to the IntegerLiteral node.
     * @return The resolved value.
     */
    ResolvedValue _resolveIntegerLiteral(AST::IntegerLiteral* integer_literal);

    /**
     * @brief Resolves a FloatLiteral node.
     * @param float_literal pointer to the FloatLiteral node.
     * @return The resolved value.
     */
    ResolvedValue _resolveFloatLiteral(AST::FloatLiteral* float_literal);

    /**
     * @brief Resolves a StringLiteral node.
     * @param string_literal pointer to the StringLiteral node.
     * @return The resolved value.
     */
    ResolvedValue _resolveStringLiteral(AST::StringLiteral* string_literal);

    /**
     * @brief Resolves an IdentifierLiteral node.
     * @param identifier_literal pointer to the IdentifierLiteral node.
     * @return The resolved value.
     */
    ResolvedValue _resolveIdentifierLiteral(AST::IdentifierLiteral* identifier_literal);

    /**
     * @brief Resolves an InfixExpression node.
     * @param infix_expression pointer to the InfixExpression node.
     * @return The resolved value.
     */
    ResolvedValue _resolveInfixExpression(AST::InfixExpression* infix_expression);

    /**
     * @brief Resolves an IndexExpression node.
     * @param index_expression pointer to the IndexExpression node.
     * @return The resolved value.
     */
    ResolvedValue _resolveIndexExpression(AST::IndexExpression* index_expression);

    /**
     * @brief Resolves a CallExpression node.
     * @param call_expression pointer to the CallExpression node.
     * @return The resolved value.
     */
    ResolvedValue _resolveCallExpression(AST::CallExpression* call_expression);

    /**
     * @brief Resolves a BooleanLiteral node.
     * @param boolean_literal pointer to the BooleanLiteral node.
     * @return The resolved value.
     */
    ResolvedValue _resolveBooleanLiteral(AST::BooleanLiteral* boolean_literal);

    /**
     * @brief Resolves an raw_arrayLiteral node.
     * @param raw_array_literal pointer to the raw_arrayLiteral node.
     * @return The resolved value.
     */
    ResolvedValue _resolveArrayLiteral(AST::ArrayLiteral* raw_array_literal);

    /**
     * @brief Validates an raw_array element's type.
     * @param element pointer to the raw_array element.
     * @param first_generic The expected struct type of the element.
     */
    void _validateraw_arrayElement(AST::Node* element, RecordStructType*& first_generic);

    /**
     * @brief Handles type conversion for an raw_array element.
     * @param element pointer to the raw_array element expression.
     * @param resolved_value Reference to the ResolvedValue to be updated.
     * @param first_generic The expected struct type of the element.
     */
    void _handleTypeConversion(AST::Expression* element, ResolvedValue& resolved_value, RecordStructType*& first_generic);

    // --- Return Statement Handlers ---

    /**
     * @brief Handles the return statement when there is a return value.
     * @param return_statement pointer to the ReturnStatement node.
     */
    void _handleValueReturnStatement(AST::ReturnStatement* return_statement);

    /**
     * @brief Resolves and validates the return value.
     * @param value pointer to the expression representing the return value.
     * @return A tuple containing the LLVM return value, allocation, and the RecordStructType*.
     */
    ResolvedValue _resolveAndValidateReturnValue(AST::Expression* value);

    /**
     * @brief Checks and performs type conversion if necessary.
     * @param value pointer to the expression representing the return value.
     * @param return_type Reference to the RecordStructType* of the return value.
     */
    void _checkAndConvertReturnType(AST::Expression* value, llvm::Value*& return_value, llvm::Value*& return_alloca, RecordStructType*& return_type);

    /**
     * @brief Creates the appropriate LLVM return instruction based on types.
     * @param return_value LLVM value to return.
     * @param return_alloca LLVM allocation value if needed.
     * @param return_type RecordStructType* of the return value.
     */
    void _createReturnInstruction(llvm::Value* return_value, llvm::Value* return_alloca, RecordStructType* return_type);

    /**
     * @brief Handles field declarations within a struct.
     * @param gstruct Pointer to the generic struct type.
     * @param field The AST node for the field.
     * @param struct_record The record for the struct.
     * @param field_types The vector of LLVM field types being built.
     * @param struct_name The name of the struct.
     */
    void _handleFieldDeclaration(RecordGenericStructType* gstruct, AST::Node* field, RecordStructType* struct_record, vector<llvm::Type*>& field_types, const Str& struct_name);
    void addBuiltinType(const Str& name, llvm::Type* type);
};

} // namespace compiler

#endif // COMPILER_HPP
