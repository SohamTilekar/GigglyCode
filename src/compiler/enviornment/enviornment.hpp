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
enum class RecordType { RecordGStructType, RecordStructInst, RecordVariable, RecordFunction, RecordModule, RecordGenericFunction };

class Record {
  public:
    RecordType type;
    std::string name;
    AST::MetaData meta_data;
    virtual inline void set_meta_data(int st_line_no, int st_col_no, int end_line_no, int end_col_no) {
        this->meta_data.st_line_no = st_line_no;
        this->meta_data.st_col_no = st_col_no;
        this->meta_data.end_line_no = end_line_no;
        this->meta_data.end_col_no = end_col_no;
    };
    std::unordered_map<std::string, std::any> extra_info = {};
    Record(RecordType type, std::string name, const std::unordered_map<std::string, std::any>& extra_info = {}) : type(type), name(name), extra_info(extra_info) {};
}; // class Record

class RecordVariable;
class RecordStructType;
class Enviornment;

class RecordFunction : public Record {
  public:
    llvm::Function* function = nullptr;
    llvm::FunctionType* function_type = nullptr;
    std::vector<std::tuple<std::string, std::shared_ptr<RecordStructType>>> arguments;
    std::shared_ptr<RecordStructType> return_inst;
    bool varArg = false;
    RecordFunction(const std::string& name) : Record(RecordType::RecordFunction, name) {};
    RecordFunction(const std::string& name, llvm::Function* function, llvm::FunctionType* function_type, std::vector<std::tuple<std::string, std::shared_ptr<RecordStructType>>> arguments,
                   std::shared_ptr<RecordStructType> return_inst, const std::unordered_map<std::string, std::any>& extra_info = {})
        : Record(RecordType::RecordFunction, name, extra_info), function(function), function_type(function_type), arguments(arguments), return_inst(return_inst) {};
    RecordFunction(const std::string& name, llvm::Function* function, llvm::FunctionType* function_type, std::vector<std::tuple<std::string, std::shared_ptr<RecordStructType>>> arguments,
                   std::shared_ptr<RecordStructType> return_inst, bool isVarArg)
        : Record(RecordType::RecordFunction, name), function(function), function_type(function_type), arguments(arguments), return_inst(return_inst), varArg(isVarArg) {};
};

class RecordGenericFunction : public Record {
  public:
    std::shared_ptr<AST::FunctionStatement> func = nullptr;
    std::shared_ptr<Enviornment> env = nullptr;
    RecordGenericFunction(const std::string& name, std::shared_ptr<AST::FunctionStatement> func, std::shared_ptr<Enviornment> env)
        : Record(RecordType::RecordGenericFunction, name), func(func), env(env) {};
};

class RecordGStructType : public Record {
  public:
    std::shared_ptr<AST::StructStatement> structt = nullptr;
    std::shared_ptr<Enviornment> env = nullptr;
    RecordGStructType(const std::string& name, std::shared_ptr<AST::StructStatement> structt, std::shared_ptr<Enviornment> env)
        : Record(RecordType::RecordGStructType, name), structt(structt), env(env) {};
};

class RecordStructType : public Record {
  public:
    llvm::Type* stand_alone_type = nullptr;
    llvm::StructType* struct_type = nullptr;
    std::vector<std::string> fields = {};
    std::unordered_map<std::string, std::shared_ptr<RecordStructType>> sub_types = {};
    std::vector<std::shared_ptr<RecordStructType>> generic_sub_types = {};
    std::vector<std::tuple<std::string, std::shared_ptr<RecordFunction>>> methods = {};
    RecordStructType(const std::string& name) : Record(RecordType::RecordStructInst, name) {};
    RecordStructType(const std::string& name, llvm::Type* stand_alone_type) : Record(RecordType::RecordStructInst, name), stand_alone_type(stand_alone_type) {};
    bool is_method(const std::string& name, const std::vector<std::shared_ptr<enviornment::RecordStructType>>& params_types, const std::unordered_map<std::string, std::any>& ex_info = {},
                   std::shared_ptr<enviornment::RecordStructType> return_type = nullptr, bool exact = false);
    std::shared_ptr<RecordFunction> get_method(const std::string& name, const std::vector<std::shared_ptr<enviornment::RecordStructType>>& params_types,
                                               const std::unordered_map<std::string, std::any>& ex_info = {}, std::shared_ptr<enviornment::RecordStructType> return_type = nullptr, bool exact = false);
};

class RecordVariable : public Record {
  public:
    llvm::Value* value = nullptr;
    llvm::Value* allocainst = nullptr;
    std::shared_ptr<RecordStructType> variableType = nullptr;
    RecordVariable(const std::string& name) : Record(RecordType::RecordVariable, name) {};
    RecordVariable(const std::string& name, llvm::Value* value, llvm::Value* allocainst, std::shared_ptr<RecordStructType> generic)
        : Record(RecordType::RecordVariable, name), value(value), allocainst(allocainst), variableType(generic) {};
};

bool _checkType(std::shared_ptr<enviornment::RecordStructType> type1, std::shared_ptr<enviornment::RecordStructType> type2);
bool _checkType(std::shared_ptr<enviornment::RecordGStructType> type1, std::shared_ptr<enviornment::RecordGStructType> type2);

class RecordModule : public Record {
  public:
    std::vector<std::tuple<std::string, std::shared_ptr<Record>>> record_map;
    RecordModule(const std::string& name, const std::vector<std::tuple<std::string, std::shared_ptr<Record>>>& record_map) : Record(RecordType::RecordModule, name), record_map(record_map) {};
    RecordModule(const std::string& name) : Record(RecordType::RecordModule, name) {};
    bool is_function(const std::string& name, const std::vector<std::shared_ptr<enviornment::RecordStructType>>& params_types, bool exact = false);
    std::shared_ptr<RecordFunction> get_function(const std::string& name, const std::vector<std::shared_ptr<enviornment::RecordStructType>>& params_types, bool exact = false);
    bool is_struct(const std::string& name, std::vector<std::shared_ptr<RecordStructType>> gen = {});
    std::shared_ptr<RecordStructType> get_struct(const std::string& name, std::vector<std::shared_ptr<RecordStructType>> gen = {});
    bool is_module(const std::string& name);
    std::shared_ptr<RecordModule> get_module(const std::string& name);
    bool is_Gfunc(const std::string& name);
    std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> get_Gfunc(const std::string& name);
    bool is_Gstruct(const std::string& name);
    std::vector<std::shared_ptr<enviornment::RecordGStructType>> get_Gstruct(const std::string& name);
};

class Enviornment {
  public:
    std::shared_ptr<Enviornment> parent;
    std::string name;
    std::vector<std::tuple<std::string, std::shared_ptr<Record>>> record_map;

    std::shared_ptr<RecordFunction> current_function = nullptr;

    std::vector<llvm::BasicBlock*> loop_body_block = {};
    std::vector<llvm::BasicBlock*> loop_end_block = {};
    std::vector<llvm::BasicBlock*> loop_condition_block = {};

    Enviornment(std::shared_ptr<Enviornment> parent = nullptr, const std::vector<std::tuple<std::string, std::shared_ptr<Record>>>& records = {}, std::string name = "unnamed")
        : parent(parent), name(name), record_map(records) {};
    void add(std::shared_ptr<Record> record);
    bool is_variable(const std::string& name, bool limit2current_scope = false);
    std::shared_ptr<RecordVariable> get_variable(const std::string& name, bool limit2current_scope = false);
    bool is_function(const std::string& name, std::vector<std::shared_ptr<enviornment::RecordStructType>> params_types, bool limit2current_scope = false, bool exact = false);
    std::shared_ptr<RecordFunction> get_function(const std::string& name, std::vector<std::shared_ptr<enviornment::RecordStructType>> params_types, bool limit2current_scope = false,
                                                 bool exact = false);
    bool is_struct(const std::string& name, bool limit2current_scope = false, std::vector<std::shared_ptr<RecordStructType>> gen = {});
    std::shared_ptr<RecordStructType> get_struct(const std::string& name, bool limit2current_scope = false, std::vector<std::shared_ptr<RecordStructType>> gen = {});
    bool is_module(const std::string& name, bool limit2current_scope = false);
    std::shared_ptr<RecordModule> get_module(const std::string& name, bool limit2current_scope = false);
    bool is_Gfunc(const std::string& name);
    std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> get_Gfunc(const std::string& name);
    bool is_Gstruct(const std::string& name);
    std::vector<std::shared_ptr<enviornment::RecordGStructType>> get_Gstruct(const std::string& name);
}; // class Environment
} // namespace enviornment
#endif