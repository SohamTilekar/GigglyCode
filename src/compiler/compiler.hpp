#ifndef COMPILER_HPP
#define COMPILER_HPP

// === LLVM Headers ===
#include <filesystem>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <vector>

// === Project-specific Headers ===
#include "../config.hpp"
#include "../compilation_state.hpp"
#include "../parser/AST/ast.hpp"
#include "./enviornment/enviornment.hpp"

namespace compiler {

using namespace enviornment;

// === Type Aliases ===

using std::vector;
using GenericStructTypeVector = vector<RecordGenericStructType*>; ///< Vector of generic struct type
                                                                  ///< pointers
using ResolvedValueVariant = std::variant<GenericStructTypeVector, RecordModule*,
                                          RecordStructType*>; ///< Variant type for resolved values
using LLVMValueVector = vector<llvm::Value*>;                 ///< Vector of LLVM values
using StructTypeVector = vector<RecordStructType*>;           ///< Vector of struct type pointers
using GenericFunctionVector = vector<RecordGenericFunction*>; ///< Vector of generic function pointers
using llBB = llvm::BasicBlock;                                ///< Alias for LLVM BasicBlock

enum class resolveType : char {
    Module,
    StructInst,
    ConstStructInst,
    StructType,
    GStructType,
};

class NotCompiledError : public std::exception {
  public:
    Str path; ///< Path of the file that was not compiled

    NotCompiledError(const Str& path) : path(path) {}

    const char* what() const noexcept override {
        static Str error_message;
        error_message = "File " + path + " is not compiled";
        return error_message.c_str();
    }
};

class DoneRet : public std::exception {
  public:
    DoneRet() {}

    const char* what() const noexcept override {
        return "Return should be caught in if-else & while, but it was not "
               "(InternalCompilationError).";
    }
};

class DoneBr : public std::exception {
  public:
    DoneBr() {}

    const char* what() const noexcept override {
        return "Branching should be caught in if-else & while, but it was not "
               "(InternalCompilationError).";
    }
};

class Compiler {
  public:
    // === Aliases ===

    struct ResolvedValue {
        llvm::Value* value;
        llvm::Value* alloca;
        ResolvedValueVariant variant;
        resolveType type;
    };

    // === Constructors ===

    Compiler(str source, const std::filesystem::path& file_path, compilationState::RecordFile* file_record, const std::filesystem::path& buildDir, const std::filesystem::path& relativePath);

    ~Compiler();

    // === Public Methods ===

    void compile(AST::Node* node);

    ResolvedValue convertType(const ResolvedValue& from, RecordStructType* to);

    static bool canConvertType(RecordStructType* from, RecordStructType* to);

    bool conversionPrecidence(RecordStructType* from, RecordStructType* to);

    // --- LLVM Components ---
    llvm::LLVMContext llvm_context;
    std::unique_ptr<llvm::Module> llvm_module;
    llvm::IRBuilder<> llvm_ir_builder;

    compilationState::RecordFile* file_record;

  private:
    // === Member Variables ===

    // --- Source Information ---
    str source;
    std::filesystem::path buildDir;
    std::filesystem::path relativePath;
    std::filesystem::path file_path;

    // --- Naming Prefixes ---
    Str fc_st_name_prefix;

    // --- Compiler Environment ---
    Enviornment* env;

    // --- Function Entry Blocks ---
    vector<llBB*> function_entry_block;

    // --- LLVM Type Pointers ---
    llvm::PointerType* ll_pointer = nullptr;
    llvm::Type* ll_int = nullptr;
    llvm::Type* ll_int32 = nullptr;
    llvm::Type* ll_void = nullptr;
    llvm::Type* ll_uint = nullptr;
    llvm::Type* ll_uint32 = nullptr;
    llvm::Type* ll_float = nullptr;
    llvm::Type* ll_float32 = nullptr;
    llvm::Type* ll_char = nullptr;
    llvm::Type* ll_str = nullptr;
    llvm::Type* ll_bool = nullptr;
    llvm::PointerType* ll_raw_array = nullptr;
    llvm::StructType* ll_array = nullptr;
    // --- Garbage-Collected Struct Types ---
    RecordStructType* gc_int = nullptr;
    RecordStructType* gc_int32 = nullptr;
    RecordStructType* gc_void = nullptr;
    RecordStructType* gc_uint = nullptr;
    RecordStructType* gc_uint32 = nullptr;
    RecordStructType* gc_float = nullptr;
    RecordStructType* gc_float32 = nullptr;
    RecordStructType* gc_char = nullptr;
    RecordStructType* gc_str = nullptr;
    RecordStructType* gc_bool = nullptr;
    RecordStructType* gc_raw_array = nullptr;

    std::vector<AST::Program*> auto_free_programs;
    std::vector<RecordStructType*> auto_free_recordStructType;

    // === Private Methods ===

    // --- Initialization Methods ---

    void _initializeBuiltins();

    void _initializeLLVMModule(const Str& path_str);

    void _initializeEnvironment();

    void _initilizeCSTDLib();

    void addFunc2Mod(RecordModule* module,
                     const Str& name,
                     const Str& llvm_name,
                     llvm::FunctionType* funcType,
                     vector<std::tuple<Str, RecordStructType*, bool, bool>>& params,
                     RecordStructType* returnType,
                     bool isVarArg);

    void addFunc(const Str& name, const Str& llvm_name, llvm::FunctionType* funcType, vector<std::tuple<Str, RecordStructType*, bool, bool>>& params, RecordStructType* returnType, bool isVarArg);

    void initilizeArray();

    // --- AST Visitor Methods ---

    void _visitProgram(AST::Program* program);

    void _visitExpressionStatement(AST::ExpressionStatement* expression_statement);

    void _visitVariableDeclarationStatement(AST::VariableDeclarationStatement* variable_declaration_statement, bool global = false);

    void _visitVariableAssignmentStatement(AST::VariableAssignmentStatement* variable_assignment_statement);

    void _visitIfElseStatement(AST::IfElseStatement* if_statement);

    void _visitFunctionDeclarationStatement(AST::FunctionStatement* function_declaration_statement, RecordStructType* struct_ = nullptr);

    void _visitReturnStatement(AST::ReturnStatement* return_statement);

    void _visitRaiseStatement(AST::RaiseStatement* raise_statement);

    void _visitBlockStatement(AST::BlockStatement* block_statement);

    void _visitWhileStatement(AST::WhileStatement* while_statement);

    void _visitForEachStatement(AST::ForEachStatement* for_statement);

    void _visitForStatement(AST::ForStatement* for_statement);

    void _visitStructStatement(AST::StructStatement* struct_statement);

    void _visitEnumStatement(AST::EnumStatement* enum_statement);

    void _visitSwitchCaseStatement(AST::SwitchCaseStatement* switch_statement);

    void _visitTryCatchStatement(AST::TryCatchStatement* tc_statement);

    void _visitImportStatement(AST::ImportStatement* import_statement, RecordModule* module = nullptr);

    void _visitBreakStatement(AST::BreakStatement* node);

    void _visitContinueStatement(AST::ContinueStatement* node);

    // --- Expression Visitor Methods ---

    ResolvedValue _visitInfixExpression(AST::InfixExpression* infix_expression);

    ResolvedValue _visitIndexExpression(AST::IndexExpression* index_expression);

    ResolvedValue _visitCallExpression(AST::CallExpression* call_expression);

    ResolvedValue _visitArrayLiteral(AST::ArrayLiteral* raw_array_literal);

    ResolvedValue _resolveValue(AST::Node* node);

    ResolvedValue _memberAccess(AST::InfixExpression* infix_expression);

    // --- Import Utilities ---

    void _importFunctionDeclarationStatement(AST::FunctionStatement* function_declaration_statement, RecordModule* module, compilationState::RecordFile* local_file_record);

    void _importStructStatement(AST::StructStatement* struct_statement, RecordModule* module, compilationState::RecordFile* local_file_record);

    // --- Type Parsing and Conversion ---

    RecordStructType* _parseType(AST::Type* type);

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

    ResolvedValue _manageFuncCall(RecordFunction* func_record, AST::CallExpression* func_call, LLVMValueVector args, const StructTypeVector& params_types);

    void _checkAndConvertCallType(RecordFunction* func_record, AST::CallExpression* func_call, LLVMValueVector& args, const StructTypeVector& params_types);

    ResolvedValue _CallGfunc(const vector<RecordGenericFunction*>& gfuncs, AST::CallExpression* func_call, const Str& name, LLVMValueVector& args, const StructTypeVector& params_types);

    ResolvedValue _CallGstruct(const vector<RecordGenericStructType*>& gstructs, AST::CallExpression* func_call, const Str& name, LLVMValueVector& args, const StructTypeVector& params_types);

    // --- Helper Methods ---

    Str extractPrefix(const Str& path_str);

    void replaceDelimiters(Str& prefix);

    void _createFunctionRecord(AST::FunctionStatement* function_declaration_statement,
                               RecordStructType* struct_ = nullptr,
                               RecordModule* module = nullptr,
                               compilationState::RecordFile* local_file_record = nullptr);

    void _createStructRecord(AST::StructStatement* struct_statement, RecordModule* module, compilationState::RecordFile* local_file_record = nullptr);

    ResolvedValue _callStruct(RecordStructType* struct_record, AST::CallExpression* call_expression, vector<RecordStructType*> params_types, LLVMValueVector args);

    void _processFieldFunction(AST::Node* field, RecordStructType* struct_record, compilationState::RecordFile* local_file_record = nullptr);

    void _handleGenericSubType(AST::Node* field, AST::StructStatement* struct_statement, RecordStructType* struct_record, RecordStructType* field_type);

    void _processFieldDeclaration(AST::Node* field, AST::StructStatement* struct_statement, RecordStructType* struct_record, vector<llvm::Type*>& field_types);

    Compiler::ResolvedValue _resolveAndValidateLeftOperand(AST::IndexExpression* index_expression);

    Compiler::ResolvedValue _resolveAndValidateIndexOperand(AST::IndexExpression* index_expression);

    ResolvedValue _handleraw_arrayIndexing(llvm::Value* left, RecordStructType* left_generic, llvm::Value* index, RecordStructType* index_generic, AST::IndexExpression* index_expression);

    ResolvedValue _handleStructIndexing(llvm::Value* left_alloca, llvm::Value* index, RecordStructType* index_generic, RecordStructType* left_generic, AST::IndexExpression* index_expression);

    [[noreturn]] void _raiseNoIndexMethodError(RecordStructType* left_generic, AST::IndexExpression* index_expression);

    // --- Resolve Functions for AST Node Types ---

    ResolvedValue _resolveIntegerLiteral(AST::IntegerLiteral* integer_literal);

    ResolvedValue _resolveFloatLiteral(AST::FloatLiteral* float_literal);

    ResolvedValue _resolveStringLiteral(AST::StringLiteral* string_literal);

    ResolvedValue _resolveIdentifierLiteral(AST::IdentifierLiteral* identifier_literal);

    ResolvedValue _resolveInfixExpression(AST::InfixExpression* infix_expression);

    ResolvedValue _resolveIndexExpression(AST::IndexExpression* index_expression);

    ResolvedValue _resolveCallExpression(AST::CallExpression* call_expression);

    ResolvedValue _resolveBooleanLiteral(AST::BooleanLiteral* boolean_literal);

    ResolvedValue _resolveArrayLiteral(AST::ArrayLiteral* raw_array_literal);

    void _validateraw_arrayElement(AST::Node* element, RecordStructType*& first_generic);

    void _handleTypeConversion(AST::Expression* element, ResolvedValue& resolved_value, RecordStructType*& first_generic);

    // --- Return Statement Handlers ---

    void _handleValueReturnStatement(AST::ReturnStatement* return_statement);

    ResolvedValue _resolveAndValidateReturnValue(AST::Expression* value);

    void _checkAndConvertReturnType(AST::Expression* value, llvm::Value*& return_value, llvm::Value*& return_alloca, RecordStructType*& return_type);

    void _createReturnInstruction(llvm::Value* return_value, llvm::Value* return_alloca, RecordStructType* return_type);

    void _handleFieldDeclaration(RecordGenericStructType* gstruct, AST::Node* field, RecordStructType* struct_record, vector<llvm::Type*>& field_types, const Str& struct_name);
    void addBuiltinType(const Str& name, llvm::Type* type);
};

} // namespace compiler

#endif // COMPILER_HPP
