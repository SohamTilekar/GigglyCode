#include "../../parser/AST/ast.hpp"
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <memory>
#include <stack>
#include <string>
#include <tuple>
#include <unordered_map>


namespace enviornment {
enum class RecordType { RecordStructType, RecordVariable, RecordFunction };

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

class RecordFunction : public Record {
  public:
    llvm::Function* function;
    llvm::FunctionType* function_type;
    std::vector<std::tuple<std::string, std::shared_ptr<RecordVariable>>> arguments;
    std::shared_ptr<RecordStructType> return_type;
    RecordFunction(std::string name) : Record(RecordType::RecordFunction, name) {};
    RecordFunction(std::string name, llvm::Function* function, llvm::FunctionType* function_type,
                   std::vector<std::tuple<std::string, std::shared_ptr<RecordVariable>>> arguments, std::shared_ptr<RecordStructType> return_type)
        : Record(RecordType::RecordFunction, name), function(function), function_type(function_type), arguments(arguments),
          return_type(return_type) {};
};

class RecordStructType : public Record {
  public:
    llvm::Type* stand_alone_type = nullptr;
    llvm::StructType* struct_type = nullptr;
    std::vector<std::string> fields = {};

    std::unordered_map<std::string, std::shared_ptr<RecordFunction>> methods;
    RecordStructType(std::string name) : Record(RecordType::RecordStructType, name) {};
    RecordStructType(std::string name, llvm::StructType* struct_type, std::vector<std::string> variable_names,
                    std::unordered_map<std::string, std::shared_ptr<RecordFunction>> functions = {})
        : Record(RecordType::RecordStructType, name), struct_type(struct_type), fields(variable_names) {};
    RecordStructType(std::string name, llvm::Type* stand_alone_type)
        : Record(RecordType::RecordStructType, name), stand_alone_type(stand_alone_type) {};
};

class RecordVariable : public Record {
  public:
    llvm::Value* value;
    llvm::Type* type;
    llvm::AllocaInst* allocainst;
    std::shared_ptr<RecordStructType> class_type;
    RecordVariable(std::string name) : Record(RecordType::RecordVariable, name) {};
    RecordVariable(std::string name, llvm::Value* value, llvm::Type* type, llvm::AllocaInst* allocainst, std::shared_ptr<RecordStructType> class_type)
        : Record(RecordType::RecordVariable, name), value(value), type(type), allocainst(allocainst), class_type(class_type) {};
};

class Enviornment {
  public:
    std::shared_ptr<Enviornment> parent;
    std::string name;
    std::unordered_map<std::string, std::shared_ptr<Record>> record_map;

    std::stack<llvm::BasicBlock*> loop_body_block = {};
    std::stack<llvm::BasicBlock*> loop_end_block = {};
    std::stack<llvm::BasicBlock*> loop_condition_block = {};

    Enviornment(std::shared_ptr<Enviornment> parent = nullptr, std::unordered_map<std::string, std::shared_ptr<Record>> records = {},
                std::string name = "unnamed")
        : parent(parent), name(name), record_map(records) {};
    void add(std::shared_ptr<Record> record);
    std::shared_ptr<Record> get(std::string name, bool limit2current_scope = false);
    bool contains(std::string name, bool limit2current_scope = false);
    bool is_variable(std::string name, bool limit2current_scope = false);
    std::shared_ptr<RecordVariable> get_variable(std::string name, bool limit2current_scope = false);
    bool is_function(std::string name, bool limit2current_scope = false);
    std::shared_ptr<RecordFunction> get_function(std::string name, bool limit2current_scope = false);
    bool is_struct(std::string name, bool limit2current_scope = false);
    std::shared_ptr<RecordStructType> get_struct(std::string
        name, bool limit2current_scope = false);
}; // class Environment
} // namespace enviornment
