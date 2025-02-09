#ifndef ENVIORNMENT_HPP
#define ENVIORNMENT_HPP

#include <cstdio>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <set>
#include <unordered_map>
#include <vector>

#include "../../config.hpp"
#include "../../parser/AST/ast.hpp"

namespace enviornment {

struct Record;

struct RecordVariable;

struct RecordFunction;

struct RecordStructType;

struct Enviornment;

struct RecordGenericFunction;

struct RecordGenericStructType;

struct RecordModule;

using Str = std::string;
using StrRecordMap = std::vector<std::tuple<Str, Record*>>;

enum struct RecordType : char {
    GStructType,
    StructInst,
    Variable,
    Function,
    Module,
    GenericFunction
};

struct Record {
    RecordType type;
    Str name;

    Record(const RecordType& type, const Str& name) : type(type), name(name) {}

    Record(const Record& other) : type(other.type), name(other.name) {}
}; // struct Record

struct RecordFunction : public Record {
  public:
    std::string ll_name;
    llvm::Function* function = nullptr;
    llvm::FunctionType* function_type = nullptr;
    std::vector<std::tuple<Str, RecordStructType*, bool, bool>> arguments = {};
    RecordStructType* return_type;
    bool is_var_arg = false;
    bool is_const_return = false;
    bool is_auto_cast = false;

    RecordFunction(const Str& name) : Record(RecordType::Function, name) {}

    RecordFunction(
        const Str& name,
        llvm::Function* function,
        llvm::FunctionType* functionType,
        std::vector<std::tuple<Str, RecordStructType*, bool, bool>> arguments,
        RecordStructType* returnInst,
        bool isVarArg,
        bool is_const_return,
        bool is_auto_cast)
        : Record(RecordType::Function, name), function(function), function_type(functionType), arguments(arguments), return_type(returnInst), is_var_arg(isVarArg), is_const_return(is_const_return), is_auto_cast(is_auto_cast) {}

    RecordFunction(const RecordFunction& other)
        : Record(other), ll_name(other.ll_name), function(other.function), function_type(other.function_type), arguments(other.arguments), return_type(other.return_type),
          is_var_arg(other.is_var_arg) {}

    RecordFunction* setFunction(llvm::Function* func) {
        function = func;
        return this;
    }

    RecordFunction* setFunction(llvm::FunctionType* func_type) {
        function_type = func_type;
        return this;
    }

    RecordFunction* setArguments(const std::vector<std::tuple<Str, RecordStructType*, bool, bool>>& arguments) {
        this->arguments = arguments;
        return this;
    }

    RecordFunction* addArgument(const std::tuple<Str, RecordStructType*, bool, bool>& argument) {
        arguments.push_back(argument);
        return this;
    }

    RecordFunction* setRetiType(RecordStructType* return_type) {
        this->return_type = return_type;
        return this;
    }

    RecordFunction* VarArg() {
        is_var_arg = true;
        return this;
    }
};

struct RecordGenericFunction : public Record {
  public:
    AST::FunctionStatement* func = nullptr;
    Enviornment* env;

    RecordGenericFunction(const Str& name, AST::FunctionStatement* func, Enviornment* env) : Record(RecordType::GenericFunction, name), func(func), env(env) {}

    RecordGenericFunction(const RecordGenericFunction& other) : Record(other), func(other.func), env(other.env) {}

    RecordGenericFunction* setFuncAST(AST::FunctionStatement* funcAST) {
        func = funcAST;
        return this;
    }

    RecordGenericFunction* setEnv(Enviornment* env) {
        this->env = env;
        return this;
    }
};

struct RecordGenericStructType : public Record {
  public:
    AST::StructStatement* structAST = nullptr;
    Enviornment* env;

    RecordGenericStructType(const Str& name, AST::StructStatement* structAST, Enviornment* env) : Record(RecordType::GStructType, name), structAST(structAST), env(env) {};

    RecordGenericStructType(const RecordGenericStructType& other) : Record(other), structAST(other.structAST), env(other.env) {}

    RecordGenericStructType* setFuncAST(AST::StructStatement* structAST) {
        this->structAST = structAST;
        return this;
    }

    RecordGenericStructType* setEnv(Enviornment* env) {
        this->env = env;
        return this;
    }
};

struct RecordStructType : public Record {
  private:
    std::vector<Str> fields = {};

  public:
    llvm::Type* stand_alone_type = nullptr;
    llvm::StructType* struct_type = nullptr;
    std::unordered_map<Str, RecordStructType*> sub_types = {};
    std::vector<RecordStructType*> generic_sub_types = {};
    std::unordered_map<std::string, uint32_t> KW_int_map;
    std::vector<std::tuple<Str, RecordFunction*>> methods = {};
    bool is_enum_kind = false;

    RecordStructType(const Str& name) : Record(RecordType::StructInst, name) {}

    RecordStructType(const Str& name, llvm::Type* stand_alone_type) : Record(RecordType::StructInst, name), stand_alone_type(stand_alone_type) {}

    RecordStructType(const RecordStructType& other, bool is_on_stack = false)
        : Record(other), fields(other.fields), stand_alone_type(other.stand_alone_type), struct_type(other.struct_type), sub_types(other.sub_types), generic_sub_types(other.generic_sub_types) {
        if (!is_on_stack) {
            for (const auto& method : other.methods) { methods.push_back({std::get<0>(method), new RecordFunction(*std::get<1>(method))}); }
        }
    }

    RecordStructType(std::string& name, llvm::IntegerType* ll_enum_underthe_hood_type, std::unordered_map<std::string, uint32_t> KW_int_map)
        : Record(RecordType::StructInst, name), stand_alone_type(ll_enum_underthe_hood_type), KW_int_map(KW_int_map), is_enum_kind(true) {};

    ~RecordStructType() {
        for (auto& [_, method] : methods) { delete method; }
        // for (auto& [_, field] : KW_int_map) { delete field; }
    }

    bool is_method(const Str& name, const std::vector<RecordStructType*>& params_types, RecordStructType* return_type = nullptr, bool exact = false, bool is_auto_cast = false);

    bool isVal(std::string name);

    RecordFunction* get_method(const Str& name, const std::vector<RecordStructType*>& params_types, RecordStructType* return_type = nullptr, bool exact = false);

    uint32_t getVal(std::string name);

    void setStandAloneType(llvm::Type* stand_alone_type) { this->stand_alone_type = stand_alone_type; }

    void setStructType(llvm::StructType* struct_type) { this->struct_type = struct_type; }

    void addSubType(Str name, RecordStructType* type) {
        this->fields.push_back(name);
        this->sub_types[name] = type;
    }

    void addGenericSubType(RecordStructType* type) { this->generic_sub_types.push_back(type); }

    void addMethod(Str name, RecordFunction* type) { this->methods.push_back({name, type}); }

    std::vector<Str>& getFields() { return this->fields; }
};

struct RecordVariable : public Record {
  public:
    llvm::Value* value = nullptr;
    llvm::Value* allocainst = nullptr;
    RecordStructType* variable_type = nullptr;
    bool is_const;

    RecordVariable(const Str& name) : Record(RecordType::Variable, name) {}

    RecordVariable(const Str& name, llvm::Value* value, llvm::Value* allocainst, RecordStructType* generic, bool is_const = false)
        : Record(RecordType::Variable, name), value(value), allocainst(allocainst), variable_type(generic), is_const(is_const) {}

    RecordVariable(const RecordVariable& other) : Record(other), value(other.value), allocainst(other.allocainst), variable_type(other.variable_type) {}
};

bool _checkType(RecordStructType* type1, RecordStructType* type2);

bool _checkType(RecordStructType* type1, RecordStructType* type2, std::set<std::pair<RecordStructType*, RecordStructType*>>& checked);

bool _checkType(RecordGenericStructType* type1, RecordGenericStructType* type2);

struct RecordModule : public Record {
  public:
    StrRecordMap record_map = {};

    RecordModule(const Str& name, const StrRecordMap& record_map) : Record(RecordType::Module, name), record_map(record_map) {}

    RecordModule(const RecordModule& other) : Record(other), record_map(other.record_map) {}

    ~RecordModule() {
        for (auto& [_, record] : record_map) {
            switch (record->type) {
                case RecordType::Variable: {
                    delete (RecordVariable*)(record);
                    break;
                }
                case RecordType::Function: {
                    delete (RecordFunction*)(record);
                    break;
                }
                case RecordType::StructInst: {
                    delete (RecordStructType*)(record);
                    break;
                }
                case RecordType::Module: {
                    delete (RecordModule*)(record);
                    break;
                }
                case RecordType::GenericFunction: {
                    delete (RecordGenericFunction*)(record);
                    break;
                }
                case RecordType::GStructType: {
                    delete (RecordGenericStructType*)(record);
                    break;
                }
                default: {
                    delete record;
                    break;
                }
            }
        }
    }

    RecordModule(const Str& name) : Record(RecordType::Module, name) {}

    void addRecord(Record* record) { record_map.push_back({record->name, record}); }

    bool isFunction(const Str& name, const std::vector<RecordStructType*>& params_types, bool exact = false);

    RecordFunction* getFunction(const Str& name, const std::vector<RecordStructType*>& params_types, bool exact = false);

    bool isGenericFunc(const Str& name);

    std::vector<RecordGenericFunction*> get_GenericFunc(const Str& name);

    bool isGenericStruct(const Str& name);

    std::vector<RecordGenericStructType*> getGenericStruct(const Str& name);

    bool is_struct(const Str& name, std::vector<RecordStructType*> gen = {});

    RecordStructType* get_struct(const Str& name, std::vector<RecordStructType*> gen = {});

    bool is_module(const Str& name);

    RecordModule* get_module(const Str& name);
};

struct Enviornment {
  public:
    Enviornment* parent;
    Str name;
    StrRecordMap record_map;

    RecordFunction* current_function = nullptr;

    std::vector<llvm::BasicBlock*> loop_body_block = {};
    std::vector<llvm::BasicBlock*> loop_conti_block = {};
    std::vector<llvm::BasicBlock*> loop_condition_block = {};
    std::vector<llvm::BasicBlock*> loop_ifbreak_block = {};
    std::vector<llvm::BasicBlock*> loop_notbreak_block = {};

    std::vector<Enviornment*> childes = {};

    Enviornment(Enviornment* parent = nullptr, const StrRecordMap& records = {}, Str name = "unnamed") : parent(parent), name(name), record_map(records) {
        if (parent) {
            this->loop_conti_block = parent->loop_conti_block;
            this->loop_body_block = parent->loop_body_block;
            this->loop_condition_block = parent->loop_condition_block;
            this->loop_ifbreak_block = parent->loop_ifbreak_block;
            this->loop_notbreak_block = parent->loop_notbreak_block;
            this->current_function = parent->current_function;
        }
    }

    ~Enviornment() {
        for (auto _record : record_map) {
            switch (std::get<1>(_record)->type) {
                case RecordType::Variable: {
                    delete (RecordVariable*)(std::get<1>(_record));
                    break;
                }
                case RecordType::Function: {
                    delete (RecordFunction*)(std::get<1>(_record));
                    break;
                }
                case RecordType::StructInst: {
                    delete (RecordStructType*)(std::get<1>(_record));
                    break;
                }
                case RecordType::Module: {
                    delete (RecordModule*)(std::get<1>(_record));
                    break;
                }
                case RecordType::GenericFunction: {
                    delete (RecordGenericFunction*)(std::get<1>(_record));
                    break;
                }
                case RecordType::GStructType: {
                    delete (RecordGenericStructType*)(std::get<1>(_record));
                    break;
                }
                default: {
                    delete std::get<1>(_record);
                    break;
                }
            }
        }
        for (auto child : childes) { delete child; }
    }

    void addRecord(Record* record);

    bool isVariable(const Str& name, bool limit2current_scope = false);

    RecordVariable* getVariable(const Str& name, bool limit2current_scope = false);

    bool isFunction(const Str& name, std::vector<RecordStructType*> params_types, bool limit2current_scope = false, bool exact = false);

    RecordFunction* getFunction(const Str& name, std::vector<RecordStructType*> params_types, bool limit2current_scope = false, bool exact = false);

    bool isStruct(const Str& name, bool limit2current_scope = false, std::vector<RecordStructType*> gen = {});

    RecordStructType* getStruct(const Str& name, bool limit2current_scope = false, std::vector<RecordStructType*> gen = {});

    bool isModule(const Str& name, bool limit2current_scope = false);

    RecordModule* getModule(const Str& name, bool limit2current_scope = false);

    bool isGenericFunc(const Str& name);

    std::vector<RecordFunction*> getFunc(const Str& name);

    std::vector<RecordGenericFunction*> getGenericFunc(const Str& name);

    bool isGenericStruct(const Str& name);

    std::vector<RecordGenericStructType*> getGenericStruct(const Str& name);

    std::vector<RecordVariable*> getCurrentFuncVars();

    void enterLoop(llvm::BasicBlock* contiBlock, llvm::BasicBlock* bodyBlock, llvm::BasicBlock* condBlock, llvm::BasicBlock* ifBreakBlock, llvm::BasicBlock* ifNotBreakBlock) {
        this->loop_conti_block.push_back(contiBlock);
        this->loop_body_block.push_back(bodyBlock);
        this->loop_condition_block.push_back(condBlock);
        this->loop_ifbreak_block.push_back(ifBreakBlock);
        this->loop_notbreak_block.push_back(ifNotBreakBlock);
    }

    void exitLoop() {
        this->loop_conti_block.pop_back();
        this->loop_body_block.pop_back();
        this->loop_condition_block.pop_back();
        this->loop_ifbreak_block.pop_back();
        this->loop_notbreak_block.pop_back();
    }
}; // struct Enviornment

} // namespace enviornment

#endif // ENVIORNMENT_HPP
