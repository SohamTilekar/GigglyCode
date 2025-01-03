#include "compiler.hpp"
#include "enviornment/enviornment.hpp"
#include <memory>

using namespace compiler;

void Compiler::addBuiltinFunction(const Str& name, const Str& llvm_name, llvm::FunctionType* funcType, const vector<std::tuple<Str, StructTypePtr, bool>>& params, StructTypePtr returnType, bool isVarArg) {
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, llvm_name, this->llvm_module.get());
    this->env->parent->addRecord(std::make_shared<RecordFunction>(name, func, funcType, params, returnType, isVarArg));
}

void Compiler::addBuiltinFunctionToModule(std::shared_ptr<RecordModule> module, const Str& name, const Str& llvm_name, llvm::FunctionType* funcType, const vector<std::tuple<Str, StructTypePtr, bool>>& params, StructTypePtr returnType, bool isVarArg) {
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, llvm_name, this->llvm_module.get());
    module->addRecord(std::make_shared<RecordFunction>(name, func, funcType, params, returnType, isVarArg));
}

void Compiler::_initilizeCSTDLib() {
    addBuiltinFunction("malloc", "malloc", llvm::FunctionType::get(this->ll_pointer, {this->ll_int}, false), {{"size", this->gc_int, false}}, this->gc_void, true);
    addBuiltinFunction("free", "free", llvm::FunctionType::get(this->ll_void, {this->ll_pointer}, false), {{"ptr", this->gc_void, false}}, this->gc_void, true);
    addBuiltinFunction("exit", "exit", llvm::FunctionType::get(this->ll_void, {this->ll_int}, false), {{"status", this->gc_int, false}}, this->gc_void, false);
    addBuiltinFunction("printf", "printf", llvm::FunctionType::get(this->ll_int, {this->ll_str}, true), {{"format", this->gc_str, false}}, this->gc_int, true);

    // stdio.h
    // auto io_module = std::make_shared<enviornment::RecordModule>("io");
    // addBuiltinFunctionToModule(io_module, "puts", "puts", llvm::FunctionType::get(this->ll_int, {this->ll_str}, false), {{"Str", this->gc_str, false}}, this->gc_int, false);
    // addBuiltinFunctionToModule(io_module, "scanf", "scanf", llvm::FunctionType::get(this->ll_int, {this->ll_str}, true), {{"format", this->gc_str, false}}, this->gc_int, true);
    // addBuiltinFunctionToModule(io_module, "putchar", "putchar", llvm::FunctionType::get(this->ll_int, {this->ll_int}, false), {{"char", this->gc_int, false}}, this->gc_int, false);
    // addBuiltinFunctionToModule(io_module, "getchar", "getchar", llvm::FunctionType::get(this->ll_int, false), {}, this->gc_int, false);
    // addBuiltinFunctionToModule(io_module, "fopen", "fopen", llvm::FunctionType::get(this->ll_pointer, {this->ll_str, this->ll_str}, false), {{"filename", this->gc_str, false}, {"mode", this->gc_str, false}}, this->gc_void, true);
    // addBuiltinFunctionToModule(io_module, "fclose", "fclose", llvm::FunctionType::get(this->ll_int, {this->ll_pointer}, false), {{"file", this->gc_void, false}}, this->gc_int, true);
    // addBuiltinFunctionToModule(io_module, "fread", "fread", llvm::FunctionType::get(this->ll_int, {this->ll_pointer, this->ll_int, this->ll_int, this->ll_pointer}, false), {{"ptr", this->gc_void, false}, {"size", this->gc_int, false}, {"count", this->gc_int, false}, {"stream", this->gc_void, false}}, this->gc_int, true);
    // addBuiltinFunctionToModule(io_module, "fwrite", "fwrite", llvm::FunctionType::get(this->ll_int, {this->ll_pointer, this->ll_int, this->ll_int, this->ll_pointer}, false), {{"ptr", this->gc_void, false}, {"size", this->gc_int, false}, {"count", this->gc_int, false}, {"stream", this->gc_void, false}}, this->gc_int, true);
    // this->env->parent->addRecord(io_module);

    // stdlib.h
    // auto stdlib_module = std::make_shared<enviornment::RecordModule>("stdlib");
    // addBuiltinFunctionToModule(stdlib_module, "atoi", "atoi", llvm::FunctionType::get(this->ll_int, {this->ll_str}, false), {{"str", this->gc_str, false}}, this->gc_int, false);
    // addBuiltinFunctionToModule(stdlib_module, "atof", "atof", llvm::FunctionType::get(this->ll_float, {this->ll_str}, false), {{"str", this->gc_str, false}}, this->gc_float, false);
    // addBuiltinFunctionToModule(stdlib_module, "srand", "srand", llvm::FunctionType::get(this->ll_void, {this->ll_int}, false), {{"seed", this->gc_int, false}}, this->gc_void, false);
    // addBuiltinFunctionToModule(stdlib_module, "rand", "rand", llvm::FunctionType::get(this->ll_int, false), {}, this->gc_int, false);
    // this->env->parent->addRecord(stdlib_module);

    // string.h
    // auto string_module = std::make_shared<enviornment::RecordModule>("string");
    // addBuiltinFunctionToModule(string_module, "strlen", "strlen", llvm::FunctionType::get(this->ll_int, {this->ll_str}, false), {{"str", this->gc_str, false}}, this->gc_int, false);
    // this->env->parent->addRecord(string_module);

    // math.h
    auto math_module = std::make_shared<enviornment::RecordModule>("math");
    addBuiltinFunctionToModule(math_module, "sin", "sin", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "cos", "cos", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "tan", "tan", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "asin", "asin", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "acos", "acos", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "atan", "atan", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "atan2", "atan2", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), {{"y", this->gc_float, false}, {"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "sinh", "sinh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "cosh", "cosh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "tanh", "tanh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "asinh", "asinh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "acosh", "acosh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "atanh", "atanh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "exp", "exp", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "exp2", "exp2", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "expm1", "expm1", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "log", "log", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "log10", "log10", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "log2", "log2", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "log1p", "log1p", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "sqrt", "sqrt", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "cbrt", "cbrt", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "hypot", "hypot", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), {{"x", this->gc_float, false}, {"y", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "ceil", "ceil", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "floor", "floor", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "round", "round", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "trunc", "trunc", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "fmod", "fmod", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), {{"numer", this->gc_float, false}, {"denom", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "remainder", "remainder", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), {{"x", this->gc_float, false}, {"y", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "remquo", "remquo", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), {{"x", this->gc_float, false}, {"y", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "fma", "fma", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float, this->ll_float}, false), {{"x", this->gc_float, false}, {"y", this->gc_float, false}, {"z", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "fdim", "fdim", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), {{"x", this->gc_float, false}, {"y", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "fabs", "fabs", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "fmax", "fmax", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), {{"x", this->gc_float, false}, {"y", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "fmin", "fmin", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), {{"x", this->gc_float, false}, {"y", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "copysign", "copysign", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), {{"x", this->gc_float, false}, {"y", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "nan", "nan", llvm::FunctionType::get(this->ll_float, {this->ll_str}, false), {{"tagp", this->gc_str, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "nextafter", "nextafter", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), {{"x", this->gc_float, false}, {"y", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "nexttoward", "nexttoward", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), {{"x", this->gc_float, false}, {"y", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "erf", "erf", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "erfc", "erfc", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "tgamma", "tgamma", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    addBuiltinFunctionToModule(math_module, "lgamma", "lgamma", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), {{"x", this->gc_float, false}}, this->gc_float, false);
    this->env->parent->addRecord(math_module);
}
