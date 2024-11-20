#include "../../parser/AST/ast.hpp"
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

namespace enviornment {
enum class RecordType { RecordStructInst, RecordVariable, RecordFunction, RecordModule };

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
    Record(RecordType type, std::string name) : type(type), name(name) {};
}; // class Record

class RecordVariable;
class RecordStructType;
class RecordStructInstance;

class RecordFunction : public Record {
  public:
    llvm::Function* function = nullptr;
    llvm::FunctionType* function_type = nullptr;
    std::vector<std::tuple<std::string, std::shared_ptr<RecordStructInstance>>> arguments;
    std::shared_ptr<RecordStructInstance> return_inst;
    RecordFunction(std::string name) : Record(RecordType::RecordFunction, name) {};
    RecordFunction(std::string name, llvm::Function* function, llvm::FunctionType* function_type,
                   std::vector<std::tuple<std::string, std::shared_ptr<RecordStructInstance>>> arguments, std::shared_ptr<RecordStructInstance> return_inst)
        : Record(RecordType::RecordFunction, name), function(function), function_type(function_type), arguments(arguments), return_inst(return_inst) {};
};

class RecordStructType : public Record {
  public:
    llvm::Type* stand_alone_type = nullptr;
    llvm::StructType* struct_type = nullptr;
    std::vector<std::string> fields = {};
    std::unordered_map<std::string, std::shared_ptr<RecordStructInstance>> sub_types = {};
    std::vector<std::tuple<std::string, std::shared_ptr<RecordFunction>>> methods = {};
    RecordStructType(std::string name) : Record(RecordType::RecordStructInst, name) {};
    RecordStructType(std::string name, llvm::Type* stand_alone_type)
        : Record(RecordType::RecordStructInst, name), stand_alone_type(stand_alone_type) {};
    bool is_method(std::string name, std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_types);
    std::shared_ptr<RecordFunction> get_method(std::string name, std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_types);
};

class RecordStructInstance {
  public:
    std::shared_ptr<RecordStructType> struct_type;
    std::vector<std::shared_ptr<RecordStructInstance>> generic = {};
    RecordStructInstance(std::shared_ptr<RecordStructType> struct_type) : struct_type(struct_type) {};
    RecordStructInstance(std::shared_ptr<RecordStructType> struct_type, std::vector<std::shared_ptr<RecordStructInstance>> generic)
        : struct_type(struct_type), generic(generic) {};
};

class RecordVariable : public Record {
  public:
    llvm::Value* value;
    llvm::AllocaInst* allocainst;
    std::shared_ptr<RecordStructInstance> variableType = nullptr;
    RecordVariable(std::string name) : Record(RecordType::RecordVariable, name) {};
    RecordVariable(std::string name, llvm::Value* value, llvm::AllocaInst* allocainst, std::shared_ptr<RecordStructInstance> generic)
    : Record(RecordType::RecordVariable, name), value(value), allocainst(allocainst), variableType(generic) {};
};

bool _checkType(std::shared_ptr<enviornment::RecordStructInstance> type1, std::shared_ptr<enviornment::RecordStructInstance> type2);
bool _checkType(std::shared_ptr<enviornment::RecordStructInstance> type1, std::shared_ptr<enviornment::RecordStructType> type2);
bool _checkType(std::shared_ptr<enviornment::RecordStructType> type1, std::shared_ptr<enviornment::RecordStructType> type2);

class RecordModule : public Record {
  public:
    std::vector<std::tuple<std::string, std::shared_ptr<Record>>> record_map;
    std::string name;
    RecordModule(std::string name, std::vector<std::tuple<std::string, std::shared_ptr<Record>>> record_map) : Record(RecordType::RecordModule, name), name(name), record_map(record_map) {};
    RecordModule(std::string name) : Record(RecordType::RecordModule, name), name(name) {};
    bool is_function(std::string name, std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_types);
    std::shared_ptr<RecordFunction> get_function(std::string name, std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_types);
    bool is_struct(std::string name);
    std::shared_ptr<RecordStructType> get_struct(std::string name);
    bool is_module(std::string name);
    std::shared_ptr<RecordModule> get_module(std::string name);
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

    Enviornment(std::shared_ptr<Enviornment> parent = nullptr, std::vector<std::tuple<std::string, std::shared_ptr<Record>>> records = {},
                std::string name = "unnamed")
        : parent(parent), name(name), record_map(records) {};
    void add(std::shared_ptr<Record> record);
    bool is_variable(std::string name, bool limit2current_scope = false);
    std::shared_ptr<RecordVariable> get_variable(std::string name, bool limit2current_scope = false);
    bool is_function(std::string name, std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_types, bool limit2current_scope = false);
    std::shared_ptr<RecordFunction> get_function(std::string name, std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_types, bool limit2current_scope = false);
    bool is_struct(std::string name, bool limit2current_scope = false);
    std::shared_ptr<RecordStructType> get_struct(std::string
        name, bool limit2current_scope = false);
    bool is_module(std::string name, bool limit2current_scope = false);
    std::shared_ptr<RecordModule> get_module(std::string
        name, bool limit2current_scope = false);
}; // class Environment
} // namespace enviornment
