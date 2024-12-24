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

// === LLVM Headers ===
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Metadata.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>

// === Standard Library Headers ===
#include <filesystem>
#include <memory>
#include <variant>
#include <vector>

// === External Libraries ===
#include "../include/json.hpp"

// === Project-specific Headers ===
#include "../parser/AST/ast.hpp"
#include "./enviornment/enviornment.hpp"

// === External Variables ===

extern std::filesystem::path GC_STD_DIR;     ///< Path to standard directory
extern std::filesystem::path GC_STD_IRGCMAP; ///< Path to IR-GC map file

namespace compiler {

/**
 * @brief Alias for the environment namespace used within the compiler.
 */
using namespace enviornment;

// === Type Aliases ===

using std::vector;
using GenericStructTypeVector = vector<GenericStructTypePtr>;                                 ///< Vector of generic struct type pointers
using ResolvedValueVariant = std::variant<GenericStructTypeVector, ModulePtr, StructTypePtr>; ///< Variant type for resolved values
using LLVMValueVector = vector<llvm::Value*>;                                                 ///< Vector of LLVM values
using StructTypeVector = vector<StructTypePtr>;                                               ///< Vector of struct type pointers
using GenericFunctionVector = vector<GenericFunctionPtr>;                                     ///< Vector of generic function pointers
using llBB = llvm::BasicBlock;                                                                ///< Alias for LLVM BasicBlock

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
    Compiler(const Str& source, const std::filesystem::path& file_path, const std::filesystem::path& ir_gc_map, const std::filesystem::path& buildDir, const std::filesystem::path& relativePath);

    // === Public Methods ===

    /**
     * @brief Starts the compilation process for the given AST node.
     * @param node Shared pointer to the root AST node.
     */
    void compile(shared_ptr<AST::Node> node);

    /**
     * @brief Converts a type from one struct type to another.
     * @param from Tuple containing LLVM values and the source struct type.
     * @param to The target struct type.
     * @return The resolved value after conversion.
     */
    ResolvedValue convertType(const ResolvedValue& from, StructTypePtr to);

    /**
     * @brief Checks if one struct type can be converted to another.
     * @param from The source struct type.
     * @param to The target struct type.
     * @return True if conversion is possible, false otherwise.
     */
    static bool canConvertType(StructTypePtr from, StructTypePtr to);

    /**
     * @brief Determines the precedence of type conversion between two struct types.
     * @param from The source struct type.
     * @param to The target struct type.
     * @return Boolean indicating the precedence.
     */
    bool conversionPrecidence(StructTypePtr from, StructTypePtr to);

    // --- LLVM Components ---
    llvm::LLVMContext llvm_context;            ///< LLVM context
    std::unique_ptr<llvm::Module> llvm_module; ///< LLVM module; For code output purposes it is public
    llvm::IRBuilder<> llvm_ir_builder;         ///< LLVM IR builder

    // --- IR-GC Mapping ---
    nlohmann::json ir_gc_map_json; ///< JSON object for IR-GC mapping; For code output purposes it is public
  private:
    // === Member Variables ===

    // --- Source Information ---
    Str source;                         ///< Source code as a string
    std::filesystem::path buildDir;     ///< Build directory path
    std::filesystem::path relativePath; ///< Relative path of the source file
    std::filesystem::path file_path;    ///< Full path of the source file
    std::filesystem::path ir_gc_map;    ///< Path to the IR-GC map


    // --- Naming Prefixes ---
    Str fc_st_name_prefix; ///< Prefix for function and struct names

    // --- Compiler Environment ---
    EnviornmentPtr env; ///< Pointer to the compiler environment

    // --- Function Entry Blocks ---
    vector<llBB*> function_entry_block; ///< Entry blocks for functions

    // --- LLVM Type Pointers ---
    llvm::PointerType* ll_pointer = nullptr;   ///< LLVM pointer type
    llvm::Type* ll_int = nullptr;              ///< LLVM integer type
    llvm::Type* ll_int32 = nullptr;            ///< LLVM 32-bit integer type
    llvm::Type* ll_void = nullptr;             ///< LLVM void type
    llvm::Type* ll_uint = nullptr;             ///< LLVM unsigned integer type
    llvm::Type* ll_uint32 = nullptr;           ///< LLVM 32-bit unsigned integer type
    llvm::Type* ll_float = nullptr;            ///< LLVM float type
    llvm::Type* ll_float32 = nullptr;          ///< LLVM 32-bit float type
    llvm::Type* ll_char = nullptr;             ///< LLVM char type
    llvm::Type* ll_str = nullptr;              ///< LLVM string type
    llvm::Type* ll_bool = nullptr;             ///< LLVM boolean type
    llvm::PointerType* ll_array = nullptr;     ///< LLVM array pointer type

    // --- Garbage-Collected Struct Types ---
    StructTypePtr gc_int = nullptr;     ///< GC wrapper for int
    StructTypePtr gc_int32 = nullptr;   ///< GC wrapper for int32
    StructTypePtr gc_void = nullptr;    ///< GC wrapper for void
    StructTypePtr gc_uint = nullptr;    ///< GC wrapper for uint
    StructTypePtr gc_uint32 = nullptr;  ///< GC wrapper for uint32
    StructTypePtr gc_float = nullptr;   ///< GC wrapper for float
    StructTypePtr gc_float32 = nullptr; ///< GC wrapper for float32
    StructTypePtr gc_char = nullptr;    ///< GC wrapper for char
    StructTypePtr gc_str = nullptr;     ///< GC wrapper for string
    StructTypePtr gc_bool = nullptr;    ///< GC wrapper for boolean
    StructTypePtr gc_array = nullptr;   ///< GC wrapper for array

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

    /**
     * @brief Initializes the IR-GC mapping from JSON.
     */
    void _initializeIRGCMap();

    void _initializeArrayType();

    // --- AST Visitor Methods ---

    /**
     * @brief Visits a Program node in the AST.
     * @param program Shared pointer to the Program node.
     */
    void _visitProgram(shared_ptr<AST::Program> program);

    /**
     * @brief Visits an ExpressionStatement node in the AST.
     * @param expression_statement Shared pointer to the ExpressionStatement node.
     */
    void _visitExpressionStatement(shared_ptr<AST::ExpressionStatement> expression_statement);

    /**
     * @brief Visits a VariableDeclarationStatement node in the AST.
     * @param variable_declaration_statement Shared pointer to the VariableDeclarationStatement node.
     */
    void _visitVariableDeclarationStatement(const std::shared_ptr<AST::VariableDeclarationStatement>& variable_declaration_statement);

    /**
     * @brief Visits a VariableAssignmentStatement node in the AST.
     * @param variable_assignment_statement Shared pointer to the VariableAssignmentStatement node.
     */
    void _visitVariableAssignmentStatement(const std::shared_ptr<AST::VariableAssignmentStatement>& variable_assignment_statement);

    /**
     * @brief Visits an IfElseStatement node in the AST.
     * @param if_statement Shared pointer to the IfElseStatement node.
     */
    void _visitIfElseStatement(shared_ptr<AST::IfElseStatement> if_statement);

    /**
     * @brief Visits a FunctionDeclarationStatement node in the AST.
     * @param function_declaration_statement Shared pointer to the FunctionDeclarationStatement node.
     * @param struct_ Optional struct type if the function is a method.
     */
    void _visitFunctionDeclarationStatement(ASTFunctionStatementPtr function_declaration_statement, StructTypePtr struct_ = nullptr);

    /**
     * @brief Visits a ReturnStatement node in the AST.
     * @param return_statement Shared pointer to the ReturnStatement node.
     */
    void _visitReturnStatement(shared_ptr<AST::ReturnStatement> return_statement);

    /**
     * @brief Visits a RaiseStatement node in the AST.
     * @param raise_statement Shared pointer to the RaiseStatement node.
     */
    void _visitRaiseStatement(shared_ptr<AST::RaiseStatement> raise_statement);

    /**
     * @brief Visits a BlockStatement node in the AST.
     * @param block_statement Shared pointer to the BlockStatement node.
     */
    void _visitBlockStatement(shared_ptr<AST::BlockStatement> block_statement);

    /**
     * @brief Visits a WhileStatement node in the AST.
     * @param while_statement Shared pointer to the WhileStatement node.
     */
    void _visitWhileStatement(shared_ptr<AST::WhileStatement> while_statement);

    /**
     * @brief Visits a ForStatement node in the AST.
     * @param for_statement Shared pointer to the ForStatement node.
     */
    void _visitForStatement(shared_ptr<AST::ForStatement> for_statement);

    /**
     * @brief Visits a StructStatement node in the AST.
     * @param struct_statement Shared pointer to the StructStatement node.
     */
    void _visitStructStatement(ASTStructStatementPtr struct_statement);

    /**
     * @brief Visits a TryCatchStatement node in the AST.
     * @param tc_statement Shared pointer to the TryCatchStatement node.
     */
    void _visitTryCatchStatement(shared_ptr<AST::TryCatchStatement> tc_statement);

    /**
     * @brief Visits an ImportStatement node in the AST.
     * @param import_statement Shared pointer to the ImportStatement node.
     * @param module Optional module pointer if importing into a module.
     */
    void _visitImportStatement(shared_ptr<AST::ImportStatement> import_statement, ModulePtr module = nullptr);

    /**
     * @brief Visits a BreakStatement node in the AST.
     * @param node Shared pointer to the BreakStatement node.
     */
    void _visitBreakStatement(shared_ptr<AST::BreakStatement> node);

    /**
     * @brief Visits a ContinueStatement node in the AST.
     * @param node Shared pointer to the ContinueStatement node.
     */
    void _visitContinueStatement(shared_ptr<AST::ContinueStatement> node);

    // --- Expression Visitor Methods ---

    /**
     * @brief Visits an InfixExpression node in the AST.
     * @param infix_expression Shared pointer to the InfixExpression node.
     * @return The resolved value of the expression.
     */
    ResolvedValue _visitInfixExpression(shared_ptr<AST::InfixExpression> infix_expression);

    /**
     * @brief Visits an IndexExpression node in the AST.
     * @param index_expression Shared pointer to the IndexExpression node.
     * @return The resolved value of the expression.
     */
    ResolvedValue _visitIndexExpression(shared_ptr<AST::IndexExpression> index_expression);

    /**
     * @brief Visits a CallExpression node in the AST.
     * @param call_expression Shared pointer to the CallExpression node.
     * @return The resolved value of the expression.
     */
    ResolvedValue _visitCallExpression(shared_ptr<AST::CallExpression> call_expression);

    /**
     * @brief Visits an ArrayLiteral node in the AST.
     * @param array_literal Shared pointer to the ArrayLiteral node.
     * @return The resolved value of the array literal.
     */
    ResolvedValue _visitArrayLiteral(const shared_ptr<AST::ArrayLiteral>& array_literal);

    /**
     * @brief Resolves the value of a given AST node.
     * @param node Shared pointer to the AST node.
     * @return The resolved value of the node.
     */
    ResolvedValue _resolveValue(shared_ptr<AST::Node> node);

    /**
     * @brief Handles member access expressions in the AST.
     * @param infix_expression Shared pointer to the InfixExpression node representing member access.
     * @return The resolved value after member access.
     */
    ResolvedValue _memberAccess(shared_ptr<AST::InfixExpression> infix_expression);

    // --- Import Utilities ---

    /**
     * @brief Imports a function declaration into a module.
     * @param function_declaration_statement Shared pointer to the FunctionDeclarationStatement node.
     * @param module Pointer to the target module.
     * @param ir_gc_map_json Reference to the IR-GC map JSON object.
     */
    void _importFunctionDeclarationStatement(ASTFunctionStatementPtr function_declaration_statement, ModulePtr module, nlohmann::json& ir_gc_map_json);

    /**
     * @brief Imports a struct declaration into a module.
     * @param struct_statement Shared pointer to the StructStatement node.
     * @param module Pointer to the target module.
     * @param ir_gc_map_json Reference to the IR-GC map JSON object.
     */
    void _importStructStatement(ASTStructStatementPtr struct_statement, ModulePtr module, nlohmann::json& ir_gc_map_json);

    // --- Type Parsing and Conversion ---

    /**
     * @brief Parses a type from the AST.
     * @param type Shared pointer to the Type node.
     * @return Pointer to the parsed StructType.
     */
    StructTypePtr _parseType(shared_ptr<AST::Type> type);

    /**
     * @brief Handles infix operations involving struct types.
     * @param op_method The method name for the operation.
     * @param op The operator as a string.
     * @param left_type The type of the left operand.
     * @param right_type The type of the right operand.
     * @param left Shared pointer to the left expression.
     * @param right Shared pointer to the right expression.
     * @param left_value LLVM value of the left operand.
     * @param right_value LLVM value of the right operand.
     * @return The resolved value after the operation.
     */
    ResolvedValue _StructInfixCall(const Str& op_method,
                                   const Str& op,
                                   StructTypePtr left_type,
                                   StructTypePtr right_type,
                                   shared_ptr<AST::Expression> left,
                                   shared_ptr<AST::Expression> right,
                                   llvm::Value* left_value,
                                   llvm::Value* right_value);

    /**
     * @brief Manages a function call within the compiler.
     * @param func_record Pointer to the function record.
     * @param func_call Shared pointer to the CallExpression node.
     * @param args Vector of LLVM values representing the arguments.
     * @param params_types Vector of struct types representing parameter types.
     * @return The resolved value after the function call.
     */
    ResolvedValue _manageFuncCall(FunctionPtr func_record, shared_ptr<AST::CallExpression> func_call, LLVMValueVector args, const StructTypeVector& params_types);

    /**
     * @brief Checks the types of arguments in a function call.
     * @param func_record Pointer to the function record.
     * @param func_call Shared pointer to the CallExpression node.
     * @param args Reference to the vector of LLVM values representing the arguments.
     * @param params_types Vector of struct types representing parameter types.
     */
    void _checkAndConvertCallType(FunctionPtr func_record, shared_ptr<AST::CallExpression> func_call, LLVMValueVector& args, const StructTypeVector& params_types);

    /**
     * @brief Calls a generic function.
     * @param gfuncs Vector of generic function pointers.
     * @param func_call Shared pointer to the CallExpression node.
     * @param name The name of the function.
     * @param args Reference to the vector of LLVM argument values.
     * @param params_types Vector of struct types representing parameter types.
     * @return The resolved value after the function call.
     */
    ResolvedValue _CallGfunc(const vector<GenericFunctionPtr>& gfuncs, const shared_ptr<AST::CallExpression> func_call, const Str& name, LLVMValueVector& args, const StructTypeVector& params_types);

    /**
     * @brief Calls a generic struct method.
     * @param gstructs Vector of generic struct type pointers.
     * @param func_call Shared pointer to the CallExpression node.
     * @param name The name of the struct method.
     * @param args Reference to the vector of LLVM argument values.
     * @param params_types Vector of struct types representing parameter types.
     * @return The resolved value after the struct method call.
     */
    ResolvedValue
    _CallGstruct(const vector<GenericStructTypePtr>& gstructs, const shared_ptr<AST::CallExpression> func_call, const Str& name, LLVMValueVector& args, const StructTypeVector& params_types);

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
     * @param function_declaration_statement Shared pointer to the FunctionDeclarationStatement node.
     * @param struct_ Optional struct type if the function is a method.
     * @param module Optional pointer to the module where the function is declared.
     * @param ir_gc_map_json Optional JSON object for IR-GC mapping.
     */
    void _createFunctionRecord(ASTFunctionStatementPtr function_declaration_statement,
                               StructTypePtr struct_ = nullptr,
                               std::shared_ptr<RecordModule> module = nullptr,
                               const nlohmann::json& ir_gc_map_json = nlohmann::json());

    /**
     * @brief Creates a struct record in the compiler environment.
     * @param struct_statement Shared pointer to the StructStatement node.
     * @param module Pointer to the module where the struct is declared.
     * @param ir_gc_map_json Reference to the IR-GC map JSON object.
     */
    void _createStructRecord(ASTStructStatementPtr struct_statement, std::shared_ptr<RecordModule> module, const nlohmann::json& ir_gc_map_json);

    /**
     * @brief Handles calling a struct method.
     * @param struct_record Pointer to the struct type record.
     * @param call_expression Shared pointer to the CallExpression node.
     * @param params_types Vector of struct types representing parameter types.
     * @param args Vector of LLVM values representing the arguments.
     * @return The resolved value after the struct method call.
     */
    ResolvedValue _callStruct(StructTypePtr struct_record, shared_ptr<AST::CallExpression> call_expression, vector<StructTypePtr> params_types, LLVMValueVector args);

    /**
     * @brief Processes a function declaration within a struct.
     * @param field The AST node for the field.
     * @param struct_record The record for the struct.
     * @param ir_gc_map_json The JSON object for IR-GC mapping.
     */
    void _processFieldFunction(AST::NodePtr field, std::shared_ptr<RecordStructType> struct_record, const nlohmann::json& ir_gc_map_json);

    /**
     * @brief Handles generic subtypes for a field.
     * @param field The AST node for the field.
     * @param struct_statement The struct's AST statement.
     * @param struct_record The record for the struct.
     * @param field_type The parsed type of the field.
     */
    void _handleGenericSubType(AST::NodePtr field, ASTStructStatementPtr struct_statement, std::shared_ptr<RecordStructType> struct_record, StructTypePtr field_type);

    /**
     * @brief Processes a variable declaration within a struct.
     * @param field The AST node for the field.
     * @param struct_statement The struct's AST statement.
     * @param struct_record The record for the struct.
     * @param field_types The vector of LLVM field types being built.
     */
    void _processFieldDeclaration(AST::NodePtr field, ASTStructStatementPtr struct_statement, std::shared_ptr<RecordStructType> struct_record, vector<llvm::Type*>& field_types);

    /**
     * @brief Resolves and validates the left operand of the index expression.
     * @param index_expression Shared pointer to the IndexExpression node.
     * @return A tuple containing the loaded LLVM value, allocation LLVM value,
     *         the generic struct type pointer, and the resolve type.
     */
    Compiler::ResolvedValue _resolveAndValidateLeftOperand(const std::shared_ptr<AST::IndexExpression>& index_expression);

    /**
     * @brief Resolves and validates the index operand of the index expression.
     * @param index_expression Shared pointer to the IndexExpression node.
     * @return A tuple containing the loaded LLVM value, allocation LLVM value,
     *         the generic struct type pointer, and the resolve type.
     */
    Compiler::ResolvedValue _resolveAndValidateIndexOperand(const std::shared_ptr<AST::IndexExpression>& index_expression);

    /**
     * @brief Handles array indexing logic.
     * @param left Loaded LLVM value of the array.
     * @param left_generic StructTypePtr of the array.
     * @param index Loaded LLVM value of the index.
     * @param index_generic StructTypePtr of the index.
     * @param index_expression Shared pointer to the IndexExpression node.
     * @return ResolvedValue containing the result of the indexing operation.
     */
    ResolvedValue _handleArrayIndexing(llvm::Value* left, StructTypePtr left_generic, llvm::Value* index, StructTypePtr index_generic, const std::shared_ptr<AST::IndexExpression>& index_expression);

    /**
     * @brief Handles struct indexing logic by invoking the `__index__` method.
     * @param left_alloca Allocation LLVM value of the struct.
     * @param index Loaded LLVM value of the index.
     * @param index_generic StructTypePtr of the index.
     * @param left_generic StructTypePtr of the struct being indexed.
     * @param index_expression Shared pointer to the IndexExpression node.
     * @return ResolvedValue containing the result of the `__index__` method call.
     */
    ResolvedValue
    _handleStructIndexing(llvm::Value* left_alloca, llvm::Value* index, StructTypePtr index_generic, StructTypePtr left_generic, const std::shared_ptr<AST::IndexExpression>& index_expression);

    /**
     * @brief Raises a NoOverload error indicating the absence of the `__index__` method.
     * @param left_generic StructTypePtr of the struct being indexed.
     * @param index_expression Shared pointer to the IndexExpression node.
     */
    [[noreturn]] void _raiseNoIndexMethodError(StructTypePtr left_generic, const std::shared_ptr<AST::IndexExpression>& index_expression);

    // --- Resolve Functions for AST Node Types ---

    /**
     * @brief Resolves an IntegerLiteral node.
     * @param integer_literal Shared pointer to the IntegerLiteral node.
     * @return The resolved value.
     */
    ResolvedValue _resolveIntegerLiteral(const std::shared_ptr<AST::IntegerLiteral>& integer_literal);

    /**
     * @brief Resolves a FloatLiteral node.
     * @param float_literal Shared pointer to the FloatLiteral node.
     * @return The resolved value.
     */
    ResolvedValue _resolveFloatLiteral(const std::shared_ptr<AST::FloatLiteral>& float_literal);

    /**
     * @brief Resolves a StringLiteral node.
     * @param string_literal Shared pointer to the StringLiteral node.
     * @return The resolved value.
     */
    ResolvedValue _resolveStringLiteral(const std::shared_ptr<AST::StringLiteral>& string_literal);

    /**
     * @brief Resolves an IdentifierLiteral node.
     * @param identifier_literal Shared pointer to the IdentifierLiteral node.
     * @return The resolved value.
     */
    ResolvedValue _resolveIdentifierLiteral(const std::shared_ptr<AST::IdentifierLiteral>& identifier_literal);

    /**
     * @brief Resolves an InfixExpression node.
     * @param infix_expression Shared pointer to the InfixExpression node.
     * @return The resolved value.
     */
    ResolvedValue _resolveInfixExpression(const std::shared_ptr<AST::InfixExpression>& infix_expression);

    /**
     * @brief Resolves an IndexExpression node.
     * @param index_expression Shared pointer to the IndexExpression node.
     * @return The resolved value.
     */
    ResolvedValue _resolveIndexExpression(const std::shared_ptr<AST::IndexExpression>& index_expression);

    /**
     * @brief Resolves a CallExpression node.
     * @param call_expression Shared pointer to the CallExpression node.
     * @return The resolved value.
     */
    ResolvedValue _resolveCallExpression(const std::shared_ptr<AST::CallExpression>& call_expression);

    /**
     * @brief Resolves a BooleanLiteral node.
     * @param boolean_literal Shared pointer to the BooleanLiteral node.
     * @return The resolved value.
     */
    ResolvedValue _resolveBooleanLiteral(const std::shared_ptr<AST::BooleanLiteral>& boolean_literal);

    /**
     * @brief Resolves an ArrayLiteral node.
     * @param array_literal Shared pointer to the ArrayLiteral node.
     * @return The resolved value.
     */
    ResolvedValue _resolveArrayLiteral(const std::shared_ptr<AST::ArrayLiteral>& array_literal);

    /**
     * @brief Validates an array element's type.
     * @param element Shared pointer to the array element.
     * @param first_generic The expected struct type of the element.
     */
    void _validateArrayElement(const std::shared_ptr<AST::Node>& element, const StructTypePtr& first_generic);

    /**
     * @brief Handles type conversion for an array element.
     * @param element Shared pointer to the array element expression.
     * @param resolved_value Reference to the ResolvedValue to be updated.
     * @param first_generic The expected struct type of the element.
     */
    void _handleTypeConversion(shared_ptr<AST::Expression> element, ResolvedValue& resolved_value, const StructTypePtr& first_generic);

    // --- Return Statement Handlers ---

    /**
     * @brief Handles the return statement when there is a return value.
     * @param return_statement Shared pointer to the ReturnStatement node.
     */
    void _handleValueReturnStatement(const std::shared_ptr<AST::ReturnStatement>& return_statement);

    /**
     * @brief Resolves and validates the return value.
     * @param value Shared pointer to the expression representing the return value.
     * @return A tuple containing the LLVM return value, allocation, and the StructTypePtr.
     */
    ResolvedValue _resolveAndValidateReturnValue(const std::shared_ptr<AST::Expression>& value);

    /**
     * @brief Checks and performs type conversion if necessary.
     * @param value Shared pointer to the expression representing the return value.
     * @param return_type Reference to the StructTypePtr of the return value.
     */
    void _checkAndConvertReturnType(AST::ExpressionPtr value, StructTypePtr& return_type);

    /**
     * @brief Creates the appropriate LLVM return instruction based on types.
     * @param return_value LLVM value to return.
     * @param return_alloca LLVM allocation value if needed.
     * @param return_type StructTypePtr of the return value.
     */
    void _createReturnInstruction(llvm::Value* return_value, llvm::Value* return_alloca, StructTypePtr return_type);

    /**
     * @brief Handles field declarations within a struct.
     * @param gstruct Pointer to the generic struct type.
     * @param field The AST node for the field.
     * @param struct_record The record for the struct.
     * @param field_types The vector of LLVM field types being built.
     * @param struct_name The name of the struct.
     */
    void _handleFieldDeclaration(GenericStructTypePtr gstruct, AST::NodePtr field, std::shared_ptr<RecordStructType> struct_record, vector<llvm::Type*>& field_types, const Str& struct_name);
    void addBuiltinType(const Str& name, llvm::Type* type);
    void addBuiltinFunction(const Str& name, llvm::FunctionType* funcType, const vector<std::tuple<Str, StructTypePtr, bool>>& params, StructTypePtr returnType);
};

} // namespace compiler

#endif // COMPILER_HPP
