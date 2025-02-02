#include "compiler.hpp"
#include "enviornment/enviornment.hpp"
#include <memory>

using namespace compiler;

void Compiler::addFunc(const Str& name, const Str& llvm_name, llvm::FunctionType* funcType, vector<std::tuple<Str, RecordStructType*, bool, bool>>& params, RecordStructType* returnType, bool strictTypeCheck) {
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, llvm_name, this->llvm_module.get());
    this->env->parent->addRecord(new RecordFunction(name, func, funcType, params, returnType, strictTypeCheck));
}

void Compiler::addFunc2Mod(RecordModule* module, const Str& name, const Str& llvm_name, llvm::FunctionType* funcType, vector<std::tuple<Str, RecordStructType*, bool, bool>>& params, RecordStructType* returnType, bool strictTypeCheck) {
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, llvm_name, this->llvm_module.get());
    module->addRecord(new RecordFunction(name, func, funcType, params, returnType, strictTypeCheck));
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
    this->env->parent->addRecord(new RecordFunction("putchar", func, funcType, params, this->gc_int32, false));

    params = {std::make_tuple("char", this->gc_char, false, false)};
    this->env->parent->addRecord(new RecordFunction("putchar", func, funcType, params, this->gc_int32, false));

    // math.h
    auto math_module = new enviornment::RecordModule("math");

    params = {std::make_tuple("x", this->gc_float, false, false)};
    addFunc2Mod(math_module, "sin", "sin", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "cos", "cos", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "tan", "tan", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "asin", "asin", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "acos", "acos", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "atan", "atan", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("y", this->gc_float, false, false), std::make_tuple("x", this->gc_float, false, false)};
    addFunc2Mod(math_module, "atan2", "atan2", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false)};
    addFunc2Mod(math_module, "sinh", "sinh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "cosh", "cosh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "tanh", "tanh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "asinh", "asinh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "acosh", "acosh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "atanh", "atanh", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "exp", "exp", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "exp2", "exp2", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "expm1", "expm1", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "log", "log", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "log10", "log10", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "log2", "log2", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "log1p", "log1p", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "sqrt", "sqrt", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "cbrt", "cbrt", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false), std::make_tuple("y", this->gc_float, false, false)};
    addFunc2Mod(math_module, "hypot", "hypot", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false)};
    addFunc2Mod(math_module, "ceil", "ceil", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "floor", "floor", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "round", "round", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "trunc", "trunc", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("numer", this->gc_float, false, false), std::make_tuple("denom", this->gc_float, false, false)};
    addFunc2Mod(math_module, "fmod", "fmod", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "remainder", "remainder", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "remquo", "remquo", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false), std::make_tuple("y", this->gc_float, false, false), std::make_tuple("z", this->gc_float, false, false)};
    addFunc2Mod(math_module, "fma", "fma", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false), std::make_tuple("y", this->gc_float, false, false)};
    addFunc2Mod(math_module, "fdim", "fdim", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false)};
    addFunc2Mod(math_module, "fabs", "fabs", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false), std::make_tuple("y", this->gc_float, false, false)};
    addFunc2Mod(math_module, "fmax", "fmax", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "fmin", "fmin", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "copysign", "copysign", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("tagp", this->gc_str, false, false)};
    addFunc2Mod(math_module, "nan", "nan", llvm::FunctionType::get(this->ll_float, {this->ll_str}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false), std::make_tuple("y", this->gc_float, false, false)};
    addFunc2Mod(math_module, "nextafter", "nextafter", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "nexttoward", "nexttoward", llvm::FunctionType::get(this->ll_float, {this->ll_float, this->ll_float}, false), params, this->gc_float, false);

    params = {std::make_tuple("x", this->gc_float, false, false)};
    addFunc2Mod(math_module, "erf", "erf", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "erfc", "erfc", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "tgamma", "tgamma", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);
    addFunc2Mod(math_module, "lgamma", "lgamma", llvm::FunctionType::get(this->ll_float, {this->ll_float}, false), params, this->gc_float, false);

    this->env->parent->addRecord(math_module);
}
