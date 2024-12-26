#include "./compiler.hpp"
#include <llvm/IR/Metadata.h>

using namespace compiler;

void Compiler::addBuiltinFunction(const Str& name, llvm::FunctionType* funcType, const vector<std::tuple<Str, StructTypePtr, bool>>& params, StructTypePtr returnType) {
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, this->llvm_module.get());
    this->env->parent->addRecord(std::make_shared<RecordFunction>(name, func, funcType, params, returnType, true));
}

void Compiler::_initilizeCSTDLib() {
    addBuiltinFunction("puts", llvm::FunctionType::get(this->ll_void, this->ll_str, false), {{"Str", this->gc_str, false}}, this->gc_void);
    addBuiltinFunction("printf", llvm::FunctionType::get(this->ll_int, {this->ll_str}, true), {{"format", this->gc_str, false}}, this->gc_int);
    addBuiltinFunction("scanf", llvm::FunctionType::get(this->ll_int, {this->ll_str}, true), {{"format", this->gc_str, false}}, this->gc_int);
    addBuiltinFunction("malloc", llvm::FunctionType::get(this->ll_pointer, {this->ll_int}, true), {{"bits", this->gc_int, false}}, this->gc_void);
    addBuiltinFunction("free", llvm::FunctionType::get(this->ll_void, {this->ll_pointer}, true), {}, this->gc_void);
}
