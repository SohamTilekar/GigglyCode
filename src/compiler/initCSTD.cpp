#include "compiler.hpp"
#include "enviornment/enviornment.hpp"
#include <memory>

using namespace compiler;

void Compiler::addFunc(const Str& name, const Str& llvm_name, llvm::FunctionType* funcType, vector<std::tuple<Str, RecordStructType*, bool, bool>>& params, RecordStructType* returnType, bool strictTypeCheck) {
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, llvm_name, this->llvm_module.get());
    this->env->parent->addRecord(std::make_unique<RecordFunction>(name, func, funcType, params, returnType, strictTypeCheck));
}

void Compiler::addFunc2Mod(RecordModule* module, const Str& name, const Str& llvm_name, llvm::FunctionType* funcType, vector<std::tuple<Str, RecordStructType*, bool, bool>>& params, RecordStructType* returnType, bool strictTypeCheck) {
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, llvm_name, this->llvm_module.get());
    module->addRecord(std::make_unique<RecordFunction>(name, func, funcType, params, returnType, strictTypeCheck));
}

void Compiler::_initilizeCSTDLib() {
    vector<std::tuple<Str, RecordStructType*, bool, bool>> params;

    params = {std::make_tuple("size", this->gc_int, false, false)};
    addFunc("malloc", "malloc", llvm::FunctionType::get(this->ll_pointer, {this->ll_int}, false), params, this->gc_void, true);

    params = {std::make_tuple("ptr", this->gc_void, false, false)};
    addFunc("free", "free", llvm::FunctionType::get(this->ll_void, {this->ll_pointer}, false), params, this->gc_void, true);

    params = {std::make_tuple("status", this->gc_int, false, false)};
    addFunc("exit", "exit", llvm::FunctionType::get(this->ll_void, {this->ll_int}, false), params, this->gc_void, false);

    params = {std::make_tuple("format", this->gc_str, false, false)};
    addFunc("printf", "printf", llvm::FunctionType::get(this->ll_int, {this->ll_str}, true), params, this->gc_int, true);

    params = {std::make_tuple("str", this->gc_str, false, false)};
    addFunc("puts", "puts", llvm::FunctionType::get(this->ll_int, {this->ll_str}, false), params, this->gc_int, false);

    params = {std::make_tuple("useconds", this->gc_uint, false, false)};
    addFunc("usleep", "usleep", llvm::FunctionType::get(this->ll_int, {this->ll_uint}, false), params, this->gc_int, false);

    params = {std::make_tuple("dest", this->gc_raw_array, false, false), std::make_tuple("c", this->gc_int, false, false), std::make_tuple("n", this->gc_uint, false, false)};
    addFunc("memset", "memset", llvm::FunctionType::get(this->ll_pointer, {this->ll_pointer, this->ll_int, this->ll_uint}, false), params, this->gc_void, false);

    auto funcType = llvm::FunctionType::get(this->ll_int32, {this->ll_int}, false);
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "putchar", this->llvm_module.get());

    params = {std::make_tuple("char", this->gc_int32, false, false)};
    this->env->parent->addRecord(std::make_unique<RecordFunction>("putchar", func, funcType, params, this->gc_int32, false));

    params = {std::make_tuple("char", this->gc_char, false, false)};
    this->env->parent->addRecord(std::make_unique<RecordFunction>("putchar", func, funcType, params, this->gc_int32, false));

    // math.h
    auto math_module = std::make_unique<enviornment::RecordModule>("math");

    params = {std::make_tuple("x", this->gc_float, false, false)};
    addFunc2Mod(math_module.get(), "sin", "sin", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "cos", "cos", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "tan", "tan", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "asin", "asin", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "acos", "acos", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "atan", "atan", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("y", this->gc_float, false, false), std::make_tuple("x", this->gc_float, false, false)};
    addFunc2Mod(math_module.get(), "atan2", "atan2", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false)};
    addFunc2Mod(math_module.get(), "sinh", "sinh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "cosh", "cosh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "tanh", "tanh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "asinh", "asinh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "acosh", "acosh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "atanh", "atanh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "exp", "exp", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "exp2", "exp2", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "expm1", "expm1", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "log", "log", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "log10", "log10", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "log2", "log2", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "log1p", "log1p", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "sqrt", "sqrt", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "cbrt", "cbrt", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false), std::make_tuple("y", this->gc_float, false, false)};
    addFunc2Mod(math_module.get(), "hypot", "hypot", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false)};
    addFunc2Mod(math_module.get(), "ceil", "ceil", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "floor", "floor", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "round", "round", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "trunc", "trunc", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("numer", this->gc_float, false, false), std::make_tuple("denom", this->gc_float, false, false)};
    addFunc2Mod(math_module.get(), "fmod", "fmod", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "remainder", "remainder", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "remquo", "remquo", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false), std::make_tuple("y", this->gc_float, false, false), std::make_tuple("z", this->gc_float, false, false)};
    addFunc2Mod(math_module.get(), "fma", "fma", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false), std::make_tuple("y", this->gc_float, false, false)};
    addFunc2Mod(math_module.get(), "fdim", "fdim", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false)};
    addFunc2Mod(math_module.get(), "fabs", "fabs", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false), std::make_tuple("y", this->gc_float, false, false)};
    addFunc2Mod(math_module.get(), "fmax", "fmax", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "fmin", "fmin", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "copysign", "copysign", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("tagp", this->gc_str, false, false)};
    addFunc2Mod(math_module.get(), "nan", "nan", llvm::FunctionType::get(this->ll_float, {this->ll_str}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false), std::make_tuple("y", this->gc_float, false, false)};
    addFunc2Mod(math_module.get(), "nextafter", "nextafter", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "nexttoward", "nexttoward", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false)};
    addFunc2Mod(math_module.get(), "erf", "erf", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "erfc", "erfc", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "tgamma", "tgamma", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module.get(), "lgamma", "lgamma", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);

    this->env->parent->addRecord(std::move(math_module));
}
