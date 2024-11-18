#include "../parser/AST/ast.hpp"
#include "enviornment/enviornment.hpp"
#include "../include/json.hpp"
#include <filesystem>
#include <memory>
#include <string>
#include <variant>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
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
#include <llvm/IR/Metadata.h>


namespace compiler {

class NotCompiledError : public std::exception {
  public:
    std::string path;
    NotCompiledError(std::string path) : path(path) {}
    const char* what() const throw() {
        return ("File " + path + " is not compiled").c_str();
    }
};

class Compiler {
  public:
    llvm::LLVMContext llvm_context;
    std::unique_ptr<llvm::Module> llvm_module;
    llvm::IRBuilder<> llvm_ir_builder; // Move the declaration here

    std::string source;
    std::filesystem::path file_path;
    std::filesystem::path ir_gc_map;
    nlohmann::json ir_gc_map_json;

    enviornment::Enviornment enviornment;

    std::vector<llvm::BasicBlock*> function_entery_block = {};

    Compiler();
    Compiler(const std::string& source, std::filesystem::path file_path, std::filesystem::path ir_gc_map);

    void compile(std::shared_ptr<AST::Node> node);

  private:
    void _initializeBuiltins();

    void _visitProgram(std::shared_ptr<AST::Program> program);

    void _visitExpressionStatement(std::shared_ptr<AST::ExpressionStatement> expression_statement);

    std::tuple<std::vector<llvm::Value*>, std::variant<std::shared_ptr<enviornment::RecordStructInstance>, std::shared_ptr<enviornment::RecordModule>>> _visitInfixExpression(
        std::shared_ptr<AST::InfixExpression> infixed_expression);
    std::tuple<std::vector<llvm::Value*>, std::variant<std::shared_ptr<enviornment::RecordStructInstance>, std::shared_ptr<enviornment::RecordModule>>> _visitIndexExpression(
        std::shared_ptr<AST::IndexExpression> index_expression);

    void _visitVariableDeclarationStatement(std::shared_ptr<AST::VariableDeclarationStatement> variable_declaration_statement);
    void _visitVariableAssignmentStatement(std::shared_ptr<AST::VariableAssignmentStatement> variable_assignment_statement);

    void _visitIfElseStatement(std::shared_ptr<AST::IfElseStatement> if_statement);

    void _visitFunctionDeclarationStatement(std::shared_ptr<AST::FunctionStatement> function_declaration_statement);
    std::tuple<std::vector<llvm::Value*>, std::variant<std::shared_ptr<enviornment::RecordStructInstance>, std::shared_ptr<enviornment::RecordModule>>> _visitCallExpression(std::shared_ptr<AST::CallExpression>);
    std::tuple<std::vector<llvm::Value*>, std::variant<std::shared_ptr<enviornment::RecordStructInstance>, std::shared_ptr<enviornment::RecordModule>>> _visitArrayLiteral(std::shared_ptr<AST::ArrayLiteral> array_literal);
    void _visitReturnStatement(std::shared_ptr<AST::ReturnStatement> return_statement);
    void _visitBlockStatement(std::shared_ptr<AST::BlockStatement> block_statement);
    void _visitWhileStatement(std::shared_ptr<AST::WhileStatement> while_statement);
    void _visitStructStatement(std::shared_ptr<AST::StructStatement> struct_statement);

    void _visitImportStatement(std::shared_ptr<AST::ImportStatement> import_statement, std::shared_ptr<enviornment::RecordModule> module = nullptr);

    std::tuple<std::vector<llvm::Value*>, std::variant<std::shared_ptr<enviornment::RecordStructInstance>, std::shared_ptr<enviornment::RecordModule>>> _resolveValue(std::shared_ptr<AST::Node> node);

    void _importFunctionDeclarationStatement(std::shared_ptr<AST::FunctionStatement> function_declaration_statement, std::shared_ptr<enviornment::RecordModule> module);
    void _importStructStatement(std::shared_ptr<AST::StructStatement> struct_statement, std::shared_ptr<enviornment::RecordModule> module);

    std::shared_ptr<enviornment::RecordStructInstance> _parseType(std::shared_ptr<AST::GenericType> type);
    bool _checkType(std::shared_ptr<enviornment::RecordStructInstance> type1, std::shared_ptr<enviornment::RecordStructInstance> type2);
    bool _checkType(std::shared_ptr<enviornment::RecordStructInstance> type1, std::shared_ptr<enviornment::RecordStructType> type2);
    bool _checkType(std::shared_ptr<enviornment::RecordStructType> type1, std::shared_ptr<enviornment::RecordStructType> type2);
    bool _checkFunctionParameterType(std::shared_ptr<enviornment::RecordFunction> func_record, std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params);
};
} // namespace compiler
