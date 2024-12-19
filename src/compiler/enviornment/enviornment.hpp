#ifndef ENVIORNMENT_HPP
#define ENVIORNMENT_HPP

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <memory>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "../../parser/AST/ast.hpp"

namespace enviornment {

class Record;
class RecordVariable;
class RecordFunction;
class RecordStructType;
class Enviornment;
class RecordGenericFunction;
class RecordGenericStructType;
class RecordModule;

using Str = std::string;
using Any = std::any;
using StrAnyMap = std::unordered_map<Str, Any>;
using StrRecordMap = std::vector<std::tuple<Str, std::shared_ptr<Record>>>;
using StructTypePtr = std::shared_ptr<RecordStructType>;
using FunctionPtr = std::shared_ptr<RecordFunction>;
using GenericFunctionPtr = std::shared_ptr<RecordGenericFunction>;
using GenericStructTypePtr = std::shared_ptr<RecordGenericStructType>;
using ModulePtr = std::shared_ptr<RecordModule>;
using VariablePtr = std::shared_ptr<RecordVariable>;
using EnviornmentPtr = std::shared_ptr<Enviornment>;
using ASTFunctionStatementPtr = std::shared_ptr<AST::FunctionStatement>;
using ASTStructStatementPtr = std::shared_ptr<AST::StructStatement>;

enum class RecordType { RecordGStructType, RecordStructInst, RecordVariable, RecordFunction, RecordModule, RecordGenericFunction };

class Record {
  public:
    RecordType type;
    Str name;
    AST::MetaData meta_data;
    StrAnyMap extra_info = {};

    virtual inline void set_meta_data(int stLineNo, int stColNo, int endLineNo, int endColNo) {
        this->meta_data.st_line_no = stLineNo;
        this->meta_data.st_col_no = stColNo;
        this->meta_data.end_line_no = endLineNo;
        this->meta_data.end_col_no = endColNo;
    };
    Record(const RecordType& type, const Str& name, const StrAnyMap& extraInfo = {}) : type(type), name(name), extra_info(extraInfo) {};
}; // class Record


class RecordFunction : public Record {
  public:
    llvm::Function* function = nullptr;
    llvm::FunctionType* function_type = nullptr;
    std::vector<std::tuple</*name=*/Str, StructTypePtr, /*IsPassByRefrence=*/bool>> arguments;
    StructTypePtr return_type;
    bool is_var_arg = false; // def xyz(x: int, `args*`) in py
    EnviornmentPtr env;

    RecordFunction(const Str& name) : Record(RecordType::RecordFunction, name) {};
    RecordFunction(const Str& name, llvm::Function* function, llvm::FunctionType* functionType, std::vector<std::tuple<Str, StructTypePtr, bool>> arguments, StructTypePtr returnInst,
                   const StrAnyMap& extraInfo = {})
        : Record(RecordType::RecordFunction, name, extraInfo), function(function), function_type(functionType), arguments(arguments), return_type(returnInst) {};
    RecordFunction(const Str& name, llvm::Function* function, llvm::FunctionType* functionType, std::vector<std::tuple<Str, StructTypePtr, bool>> arguments, StructTypePtr returnInst, bool isVarArg)
        : Record(RecordType::RecordFunction, name), function(function), function_type(functionType), arguments(arguments), return_type(returnInst), is_var_arg(isVarArg) {};

    RecordFunction* setFunction(llvm::Function* func) {
        function = func;
        return this;
    };
    RecordFunction* setFunction(llvm::FunctionType* func_type) {
        function_type = func_type;
        return this;
    };

    RecordFunction* setArguments(const std::vector<std::tuple</*name=*/Str, StructTypePtr, /*IsPassByRefrence=*/bool>>& arguments) {
        this->arguments = arguments;
        return this;
    };
    RecordFunction* addArgument(const std::tuple</*name=*/Str, StructTypePtr, /*IsPassByRefrence=*/bool>& argument) {
        arguments.push_back(argument);
        return this;
    };

    RecordFunction* setRetiType(StructTypePtr return_type) {
        this->return_type = return_type;
        return this;
    };
    RecordFunction* setEnv(EnviornmentPtr env) {
        this->env = env;
        return this;
    };

    RecordFunction* VarArg() {
        is_var_arg = true;
        return this;
    };
};

class RecordGenericFunction : public Record {
  public:
    ASTFunctionStatementPtr func = nullptr;
    EnviornmentPtr env = nullptr;

    RecordGenericFunction(const Str& name, ASTFunctionStatementPtr func, EnviornmentPtr env) : Record(RecordType::RecordGenericFunction, name), func(func), env(env) {};

    RecordGenericFunction* setFuncAST(ASTFunctionStatementPtr funcAST) {
        func = funcAST;
        return this;
    };
    RecordGenericFunction* setEnv(EnviornmentPtr env) {
        this->env = env;
        return this;
    };
};

class RecordGenericStructType : public Record {
  public:
    ASTStructStatementPtr structAST = nullptr;
    EnviornmentPtr env = nullptr;

    RecordGenericStructType(const Str& name, ASTStructStatementPtr structAST, EnviornmentPtr env) : Record(RecordType::RecordGStructType, name), structAST(structAST), env(env) {};

    RecordGenericStructType* setFuncAST(ASTStructStatementPtr structAST) {
        this->structAST = structAST;
        return this;
    };
    RecordGenericStructType* setEnv(EnviornmentPtr env) {
        this->env = env;
        return this;
    };
};

class RecordStructType : public Record {
  private:
    std::vector<Str> fields = {};

  public:
    llvm::Type* stand_alone_type = nullptr;
    llvm::StructType* struct_type = nullptr;
    std::unordered_map<Str, StructTypePtr> sub_types = {};
    std::vector<StructTypePtr> generic_sub_types = {};
    std::vector<std::tuple<Str, FunctionPtr>> methods = {};
    FunctionPtr gc_struct_clear = nullptr;

    RecordStructType(const Str& name) : Record(RecordType::RecordStructInst, name) {};
    RecordStructType(const Str& name, llvm::Type* stand_alone_type) : Record(RecordType::RecordStructInst, name), stand_alone_type(stand_alone_type) {};

    bool is_method(const Str& name, const std::vector<StructTypePtr>& params_types, const StrAnyMap& ex_info = {}, StructTypePtr return_type = nullptr, bool exact = false);
    FunctionPtr get_method(const Str& name, const std::vector<StructTypePtr>& params_types, const StrAnyMap& ex_info = {}, StructTypePtr return_type = nullptr, bool exact = false);

    void setStandAloneType(llvm::Type* stand_alone_type) { this->stand_alone_type = stand_alone_type; };
    void setStructType(llvm::StructType* struct_type) { this->struct_type = struct_type; };

    void addSubType(Str name, StructTypePtr type) {
        this->fields.push_back(name);
        this->sub_types[name] = type;
    };
    void addGenericSubType(StructTypePtr type) { this->generic_sub_types.push_back(type); };
    void addMethod(Str name, FunctionPtr type) { this->methods.push_back({name, type}); };

    std::vector<Str>& getFields() { return this->fields; };
};

class RecordVariable : public Record {
  public:
    llvm::Value* value = nullptr;
    llvm::Value* allocainst = nullptr;
    StructTypePtr variable_type = nullptr;
    RecordVariable(const Str& name) : Record(RecordType::RecordVariable, name) {};
    RecordVariable(const Str& name, llvm::Value* value, llvm::Value* allocainst, StructTypePtr generic)
        : Record(RecordType::RecordVariable, name), value(value), allocainst(allocainst), variable_type(generic) {};
};

bool _checkType(StructTypePtr type1, StructTypePtr type2);
bool _checkType(GenericStructTypePtr type1, GenericStructTypePtr type2);

class RecordModule : public Record {
  public:
    StrRecordMap record_map = {}; // Holds Records in modules

    RecordModule(const Str& name, const StrRecordMap& record_map) : Record(RecordType::RecordModule, name), record_map(record_map) {};
    RecordModule(const Str& name) : Record(RecordType::RecordModule, name) {};

    void addRecord(std::shared_ptr<Record> record) { record_map.push_back({record->name, record}); };

    bool isFunction(const Str& name, const std::vector<StructTypePtr>& params_types, bool exact = false);
    FunctionPtr getFunction(const Str& name, const std::vector<StructTypePtr>& params_types, bool exact = false);

    bool isGenericFunc(const Str& name);
    std::vector<GenericFunctionPtr> get_GenericFunc(const Str& name);

    bool isGenericStruct(const Str& name);
    std::vector<GenericStructTypePtr> getGenericStruct(const Str& name);

    bool is_struct(const Str& name, std::vector<StructTypePtr> gen = {});
    StructTypePtr get_struct(const Str& name, std::vector<StructTypePtr> gen = {});

    bool is_module(const Str& name);
    ModulePtr get_module(const Str& name);
};

class Enviornment {
  public:
    EnviornmentPtr parent;
    Str name;
    StrRecordMap record_map;

    RecordFunction* current_function = nullptr;

    std::vector<llvm::BasicBlock*> loop_body_block = {};
    std::vector<llvm::BasicBlock*> loop_conti_block = {};
    std::vector<llvm::BasicBlock*> loop_condition_block = {};
    std::vector<llvm::BasicBlock*> loop_ifbreak_block = {};
    std::vector<llvm::BasicBlock*> loop_notbreak_block = {};

    Enviornment(EnviornmentPtr parent = nullptr, const StrRecordMap& records = {}, Str name = "unnamed") : parent(parent), name(name), record_map(records) {
        if (parent) {
            this->loop_conti_block = parent->loop_conti_block;
            this->loop_body_block = parent->loop_body_block;
            this->loop_condition_block = parent->loop_condition_block;
            this->loop_ifbreak_block = parent->loop_ifbreak_block;
            this->loop_notbreak_block = parent->loop_notbreak_block;
            this->current_function = parent->current_function;
        }
    };
    void addRecord(std::shared_ptr<Record> record);

    bool isVariable(const Str& name, bool limit2current_scope = false);
    VariablePtr getVariable(const Str& name, bool limit2current_scope = false);

    bool isFunction(const Str& name, std::vector<StructTypePtr> params_types, bool limit2current_scope = false, bool exact = false);
    FunctionPtr getFunction(const Str& name, std::vector<StructTypePtr> params_types, bool limit2current_scope = false, bool exact = false);

    bool isStruct(const Str& name, bool limit2current_scope = false, std::vector<StructTypePtr> gen = {});
    StructTypePtr getStruct(const Str& name, bool limit2current_scope = false, std::vector<StructTypePtr> gen = {});

    bool isModule(const Str& name, bool limit2current_scope = false);
    ModulePtr getModule(const Str& name, bool limit2current_scope = false);

    bool isGenericFunc(const Str& name);
    std::vector<GenericFunctionPtr> getGenericFunc(const Str& name);

    bool isGenericStruct(const Str& name);
    std::vector<GenericStructTypePtr> getGenericStruct(const Str& name);

    std::vector<VariablePtr> getCurrentVars();

    void enterLoop(llvm::BasicBlock* contiBlock, llvm::BasicBlock* bodyBlock, llvm::BasicBlock* condBlock, llvm::BasicBlock* ifBreakBlock, llvm::BasicBlock* ifNotBreakBlock) {
        this->loop_conti_block.push_back(contiBlock);
        this->loop_body_block.push_back(bodyBlock);
        this->loop_condition_block.push_back(condBlock);
        this->loop_ifbreak_block.push_back(ifBreakBlock);
        this->loop_notbreak_block.push_back(ifNotBreakBlock);
    };
    void exitLoop() {
        this->loop_conti_block.pop_back();
        this->loop_body_block.pop_back();
        this->loop_condition_block.pop_back();
        this->loop_ifbreak_block.pop_back();
        this->loop_notbreak_block.pop_back();
    };
}; // class Environment
} // namespace enviornment
#endif