#ifndef COMPILER_HPP
#define COMPILER_HPP

#include <llvm/IR/DataLayout.h>
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

#include <filesystem>
#include <memory>
#include <string>
#include <variant>
#include <vector>

#include "../parser/AST/ast.hpp"
#include "./enviornment/enviornment.hpp"


extern std::filesystem::path GC_STD_DIR;
extern std::filesystem::path GC_STD_IRGCMAP;
namespace compiler {

class NotCompiledError : public std::exception {
  public:
    std::string path;
    NotCompiledError(const std::string& path) : path(path) {}
    const char* what() const throw() { return ("File " + path + " is not compiled").c_str(); }
};

class DoneRet : public std::exception {
  public:
    DoneRet() {}
    const char* what() const throw() { return "Rety Should Be get Catch in the ifelse & while but it not InternalCompilationError"; }
};

enum class resolveType {
    Module,
    StructInst,
    StructType,
    GStructType,
};

class Compiler {
  public:
    llvm::LLVMContext llvm_context;
    std::unique_ptr<llvm::Module> llvm_module;
    llvm::IRBuilder<> llvm_ir_builder; // Move the declaration here

    std::string source;
    std::filesystem::path buildDir;
    std::filesystem::path relativePath;
    std::filesystem::path file_path;
    std::filesystem::path ir_gc_map;
    nlohmann::json ir_gc_map_json;

    std::string fc_st_name_prefix;

    std::shared_ptr<enviornment::Enviornment> enviornment;

    std::vector<llvm::BasicBlock*> function_entery_block = {};

    Compiler(const std::string& source, std::filesystem::path file_path, std::filesystem::path ir_gc_map, std::filesystem::path buildDir, std::filesystem::path relativePath);

    void compile(std::shared_ptr<AST::Node> node);

    std::tuple<llvm::Value*, llvm::Value*,
               std::variant<std::vector<std::shared_ptr<enviornment::RecordGStructType>>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, resolveType>
    convertType(std::tuple<llvm::Value*, llvm::Value*, std::shared_ptr<enviornment::RecordStructType>> of, std::shared_ptr<enviornment::RecordStructType> to);
    static bool canConvertType(std::shared_ptr<enviornment::RecordStructType> from, std::shared_ptr<enviornment::RecordStructType> to);
    bool conversionPrecidence(std::shared_ptr<enviornment::RecordStructType> from, std::shared_ptr<enviornment::RecordStructType> to);

  private:
    void _initializeBuiltins();

    void _visitProgram(std::shared_ptr<AST::Program> program);

    void _visitExpressionStatement(std::shared_ptr<AST::ExpressionStatement> expression_statement);

    std::tuple<llvm::Value*, llvm::Value*,
               std::variant<std::vector<std::shared_ptr<enviornment::RecordGStructType>>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, resolveType>
    _visitInfixExpression(std::shared_ptr<AST::InfixExpression> infixed_expression);
    std::tuple<llvm::Value*, llvm::Value*,
               std::variant<std::vector<std::shared_ptr<enviornment::RecordGStructType>>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, resolveType>
    _visitIndexExpression(std::shared_ptr<AST::IndexExpression> index_expression);

    void _visitVariableDeclarationStatement(std::shared_ptr<AST::VariableDeclarationStatement> variable_declaration_statement);
    void _visitVariableAssignmentStatement(std::shared_ptr<AST::VariableAssignmentStatement> variable_assignment_statement);

    void _visitIfElseStatement(std::shared_ptr<AST::IfElseStatement> if_statement);

    void _visitFunctionDeclarationStatement(std::shared_ptr<AST::FunctionStatement> function_declaration_statement, std::shared_ptr<enviornment::RecordStructType> struct_ = nullptr);
    std::tuple<llvm::Value*, llvm::Value*,
               std::variant<std::vector<std::shared_ptr<enviornment::RecordGStructType>>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, resolveType>
        _visitCallExpression(std::shared_ptr<AST::CallExpression>);
    std::tuple<llvm::Value*, llvm::Value*,
               std::variant<std::vector<std::shared_ptr<enviornment::RecordGStructType>>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, resolveType>
    _CallGfunc(std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> gfuncs, std::shared_ptr<AST::CallExpression> func_call, std::string name, std::vector<llvm::Value*> args,
               std::vector<std::shared_ptr<enviornment::RecordStructType>> params_types);
    std::tuple<llvm::Value*, llvm::Value*,
               std::variant<std::vector<std::shared_ptr<enviornment::RecordGStructType>>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, resolveType>
    _CallGstruct(std::vector<std::shared_ptr<enviornment::RecordGStructType>> gstructs, std::shared_ptr<AST::CallExpression> func_call, std::string name, std::vector<llvm::Value*> args,
                 std::vector<std::shared_ptr<enviornment::RecordStructType>> params_types);
    std::tuple<llvm::Value*, llvm::Value*,
               std::variant<std::vector<std::shared_ptr<enviornment::RecordGStructType>>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, resolveType>
    _visitArrayLiteral(std::shared_ptr<AST::ArrayLiteral> array_literal);
    void _visitReturnStatement(std::shared_ptr<AST::ReturnStatement> return_statement);
    void _visitBlockStatement(std::shared_ptr<AST::BlockStatement> block_statement);
    void _visitWhileStatement(std::shared_ptr<AST::WhileStatement> while_statement);
    void _visitForStatement(std::shared_ptr<AST::ForStatement> for_statement);
    void _visitStructStatement(std::shared_ptr<AST::StructStatement> struct_statement);

    void _visitImportStatement(std::shared_ptr<AST::ImportStatement> import_statement, std::shared_ptr<enviornment::RecordModule> module = nullptr);

    std::tuple<llvm::Value*, llvm::Value*,
               std::variant<std::vector<std::shared_ptr<enviornment::RecordGStructType>>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, resolveType>
    _resolveValue(std::shared_ptr<AST::Node> node);

    void _importFunctionDeclarationStatement(std::shared_ptr<AST::FunctionStatement> function_declaration_statement, std::shared_ptr<enviornment::RecordModule> module, nlohmann::json& ir_gc_map_json);
    void _importStructStatement(std::shared_ptr<AST::StructStatement> struct_statement, std::shared_ptr<enviornment::RecordModule> module, nlohmann::json& ir_gc_map_json);

    std::shared_ptr<enviornment::RecordStructType> _parseType(std::shared_ptr<AST::Type> type);

    std::tuple<llvm::Value*, llvm::Value*,
               std::variant<std::vector<std::shared_ptr<enviornment::RecordGStructType>>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>,
               compiler::resolveType>
    _memberAccess(std::shared_ptr<AST::InfixExpression> infixed_expression);
    std::tuple<llvm::Value*, llvm::Value*,
               std::variant<std::vector<std::shared_ptr<enviornment::RecordGStructType>>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>,
               compiler::resolveType>
    _StructInfixCall(const std::string& op_method, const std::string& op, std::shared_ptr<enviornment::RecordStructType> left_type, std::shared_ptr<enviornment::RecordStructType> right_type,
                     std::shared_ptr<AST::Expression> left, std::shared_ptr<AST::Expression> right, llvm::Value* left_value, llvm::Value* right_value);
    std::tuple<llvm::Value*, llvm::Value*,
               std::variant<std::vector<std::shared_ptr<enviornment::RecordGStructType>>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>,
               compiler::resolveType>
    _manageFuncCall(std::shared_ptr<enviornment::RecordFunction> func_record, std::shared_ptr<AST::CallExpression> func_call, std::vector<llvm::Value*> args, const std::vector<std::shared_ptr<enviornment::RecordStructType>>& params_types);
    void _checkCallType(std::shared_ptr<enviornment::RecordFunction> func_record, std::shared_ptr<AST::CallExpression> func_call, std::vector<llvm::Value*>& args, const std::vector<std::shared_ptr<enviornment::RecordStructType>>& params_types);
};
} // namespace compiler
#endif