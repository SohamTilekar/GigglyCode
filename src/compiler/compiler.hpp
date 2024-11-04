#include "../parser/AST/ast.hpp"
#include "enviornment/enviornment.hpp"
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
#include <memory>
#include <string>


namespace compiler {
class Compiler {
  public:
    llvm::LLVMContext llvm_context;
    std::unique_ptr<llvm::Module> llvm_module;
    llvm::IRBuilder<> llvm_ir_builder; // Move the declaration here

    const std::string source;
    enviornment::Enviornment enviornment;
    llvm::Function* current_function;
    Compiler();
    Compiler(const std::string& source);

    void compile(std::shared_ptr<AST::Node> node);

  private:
    void _initializeBuiltins();

    void _visitProgram(std::shared_ptr<AST::Program> program);

    void _visitExpressionStatement(std::shared_ptr<AST::ExpressionStatement> expression_statement);

    std::tuple<std::vector<llvm::Value*>, std::shared_ptr<enviornment::RecordStructInstance>> _visitInfixExpression(
        std::shared_ptr<AST::InfixExpression> infixed_expression);
    std::tuple<std::vector<llvm::Value*>, std::shared_ptr<enviornment::RecordStructInstance>> _visitIndexExpression(
        std::shared_ptr<AST::IndexExpression> index_expression);

    void _visitVariableDeclarationStatement(std::shared_ptr<AST::VariableDeclarationStatement> variable_declaration_statement);
    void _visitVariableAssignmentStatement(std::shared_ptr<AST::VariableAssignmentStatement> variable_assignment_statement);

    void _visitIfElseStatement(std::shared_ptr<AST::IfElseStatement> if_statement);

    void _visitFunctionDeclarationStatement(std::shared_ptr<AST::FunctionStatement> function_declaration_statement);
    // void visitFunctionDeclarationStatement(std::shared_ptr<AST::FunctionStatement> function_declaration_statement);
    std::tuple<std::vector<llvm::Value*>, std::shared_ptr<enviornment::RecordStructInstance>> _visitCallExpression(std::shared_ptr<AST::CallExpression>);
    std::tuple<std::vector<llvm::Value*>, std::shared_ptr<enviornment::RecordStructInstance>> _visitArrayLitetal(std::shared_ptr<AST::ArrayLiteral> array_literal);
    void _visitReturnStatement(std::shared_ptr<AST::ReturnStatement> return_statement);
    void _visitBlockStatement(std::shared_ptr<AST::BlockStatement> block_statement);
    void _visitWhileStatement(std::shared_ptr<AST::WhileStatement> while_statement);
    void _visitStructStatement(std::shared_ptr<AST::StructStatement> struct_statement);

    std::tuple<std::vector<llvm::Value*>, std::shared_ptr<enviornment::RecordStructInstance>> _resolveValue(std::shared_ptr<AST::Node> node);

    std::shared_ptr<enviornment::RecordStructInstance> _parseType(std::shared_ptr<AST::GenericType> type);
};
} // namespace compiler
