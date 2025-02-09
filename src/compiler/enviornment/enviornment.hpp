#ifndef ENVIORNMENT_HPP
#define ENVIORNMENT_HPP

/**
 * @file enviornment.hpp
 * @brief Defines the environment structes for managing compilation records and
 * scopes.
 *
 * This header file declares several structes within the `enviornment` namespace
 * that are responsible for managing records of variables, functions, structs,
 * modules, and their respective environments during the compilation process.
 */

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

using Str = std::string;                                    ///< Alias for std::string.
using StrRecordMap = std::vector<std::tuple<Str, Record*>>; ///< Vector of tuples mapping
                                                            ///< strings to records.

/**
 * @enum RecordType
 * @brief Enumeration of different types of records.
 */
enum struct RecordType : char {
    GStructType,    ///< Represents a generic struct type record.
    StructInst,     ///< Represents a struct instance record.
    Variable,       ///< Represents a variable record.
    Function,       ///< Represents a function record.
    Module,         ///< Represents a module record.
    GenericFunction ///< Represents a generic function record.
};

/**
 * @struct Record
 * @brief Base struct representing a compilation record.
 *
 * The Record struct serves as the base struct for various types of records
 * used during the compilation process. It contains common attributes such
 * as the record type, name, metadata, and any additional information.
 */
struct Record {
    RecordType type;          ///< The type of the record.
    Str name;                 ///< The name of the record.

    /**
     * @brief Constructs a Record with the specified type and name.
     * @param type The type of the record.
     * @param name The name of the record.
     * @param extraInfo Optional extra information for the record.
     */
    Record(const RecordType& type, const Str& name) : type(type), name(name) {}

    /**
     * @brief Copy constructor for Record.
     * @param other The Record object to copy from.
     */
    Record(const Record& other) : type(other.type), name(other.name) {}
}; // struct Record

/**
 * @struct RecordFunction
 * @brief Represents a function record within the environment.
 *
 * This struct holds information about a function, including its LLVM function,
 * function type, arguments, return type, and other related data.
 */
struct RecordFunction : public Record {
  public:
    std::string ll_name;                         ///< The LLVM name of the function.
    llvm::Function* function = nullptr;          ///< Pointer to the LLVM Function.
    llvm::FunctionType* function_type = nullptr; ///< Pointer to the LLVM FunctionType.
    /**
     * @brief Vector of arguments where each argument is a tuple containing:
     * - Name of the argument.
     * - Pointer to the struct type of the argument.
     * - Boolean indicating if the argument is passed by reference.
     * - Boolean indicating if the argument is const.
     */
    std::vector<std::tuple<Str, RecordStructType*, bool, bool>> arguments = {};
    RecordStructType* return_type; ///< Pointer to the struct type of the return value.
    bool is_var_arg = false;       ///< Indicates if the function accepts a variable
    bool is_const_return = false;       ///< Indicates if the function accepts a variable
    bool is_auto_cast = false;       ///< Indicates if the function accepts a variable

    /**
     * @brief Constructs a RecordFunction with the given name.
     * @param name The name of the function.
     */
    RecordFunction(const Str& name) : Record(RecordType::Function, name) {}

    /**
     * @brief Constructs a RecordFunction with variable arguments support.
     * @param name The name of the function.
     * @param function Pointer to the LLVM Function.
     * @param functionType Pointer to the LLVM FunctionType.
     * @param arguments Vector of arguments.
     * @param returnInst Pointer to the struct type of the return value.
     * @param isVarArg Indicates if the function accepts variable arguments.
     */
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

    /**
     * @brief Copy constructor for RecordFunction.
     * @param other The RecordFunction object to copy from.
     */
    RecordFunction(const RecordFunction& other)
        : Record(other), ll_name(other.ll_name), function(other.function), function_type(other.function_type), arguments(other.arguments), return_type(other.return_type),
          is_var_arg(other.is_var_arg) {}

    /**
     * @brief Sets the LLVM Function pointer.
     * @param func Pointer to the LLVM Function.
     * @return Pointer to the current RecordFunction instance.
     */
    RecordFunction* setFunction(llvm::Function* func) {
        function = func;
        return this;
    }

    /**
     * @brief Sets the LLVM FunctionType pointer.
     * @param func_type Pointer to the LLVM FunctionType.
     * @return Pointer to the current RecordFunction instance.
     */
    RecordFunction* setFunction(llvm::FunctionType* func_type) {
        function_type = func_type;
        return this;
    }

    /**
     * @brief Sets the arguments for the function.
     * @param arguments Vector of arguments.
     * @return Pointer to the current RecordFunction instance.
     */
    RecordFunction* setArguments(const std::vector<std::tuple<Str, RecordStructType*, bool, bool>>& arguments) {
        this->arguments = arguments;
        return this;
    }

    /**
     * @brief Adds a single argument to the function.
     * @param argument Tuple containing argument details.
     * @return Pointer to the current RecordFunction instance.
     */
    RecordFunction* addArgument(const std::tuple<Str, RecordStructType*, bool, bool>& argument) {
        arguments.push_back(argument);
        return this;
    }

    /**
     * @brief Sets the return type of the function.
     * @param return_type Pointer to the struct type of the return value.
     * @return Pointer to the current RecordFunction instance.
     */
    RecordFunction* setRetiType(RecordStructType* return_type) {
        this->return_type = return_type;
        return this;
    }

    /**
     * @brief Marks the function as accepting variable arguments.
     * @return Pointer to the current RecordFunction instance.
     */
    RecordFunction* VarArg() {
        is_var_arg = true;
        return this;
    }
};

/**
 * @struct RecordGenericFunction
 * @brief Represents a generic function record within the environment.
 *
 * This struct holds information about a generic function, including its
 * associated AST node and environment.
 */
struct RecordGenericFunction : public Record {
  public:
    AST::FunctionStatement* func = nullptr; ///< Pointer to the AST FunctionStatement.
    Enviornment* env;                       ///< Pointer to the environment.

    /**
     * @brief Constructs a RecordGenericFunction with the specified name,
     * function, and environment.
     * @param name The name of the generic function.
     * @param func Pointer to the AST FunctionStatement.
     * @param env Pointer to the environment.
     */
    RecordGenericFunction(const Str& name, AST::FunctionStatement* func, Enviornment* env) : Record(RecordType::GenericFunction, name), func(func), env(env) {}

    /**
     * @brief Copy constructor for RecordGenericFunction.
     * @param other The RecordGenericFunction object to copy from.
     */
    RecordGenericFunction(const RecordGenericFunction& other) : Record(other), func(other.func), env(other.env) {}

    /**
     * @brief Sets the AST FunctionStatement pointer.
     * @param funcAST Pointer to the AST FunctionStatement.
     * @return Pointer to the current RecordGenericFunction instance.
     */
    RecordGenericFunction* setFuncAST(AST::FunctionStatement* funcAST) {
        func = funcAST;
        return this;
    }

    /**
     * @brief Sets the environment for the generic function.
     * @param env Pointer to the environment.
     * @return Pointer to the current RecordGenericFunction instance.
     */
    RecordGenericFunction* setEnv(Enviornment* env) {
        this->env = env;
        return this;
    }
};

/**
 * @struct RecordGenericStructType
 * @brief Represents a generic struct type record within the environment.
 *
 * This struct manages information about a generic struct type, including its
 * associated AST node and environment.
 */
struct RecordGenericStructType : public Record {
  public:
    AST::StructStatement* structAST = nullptr; ///< Pointer to the AST StructStatement.
    Enviornment* env;                          ///< Pointer to the environment.

    /**
     * @brief Constructs a RecordGenericStructType with the specified name, struct
     * AST, and environment.
     * @param name The name of the generic struct type.
     * @param structAST Pointer to the AST StructStatement.
     * @param env Pointer to the environment.
     */
    RecordGenericStructType(const Str& name, AST::StructStatement* structAST, Enviornment* env) : Record(RecordType::GStructType, name), structAST(structAST), env(env) {};

    /**
     * @brief Copy constructor for RecordGenericStructType.
     * @param other The RecordGenericStructType object to copy from.
     */
    RecordGenericStructType(const RecordGenericStructType& other) : Record(other), structAST(other.structAST), env(other.env) {}

    /**
     * @brief Sets the AST StructStatement pointer.
     * @param structAST Pointer to the AST StructStatement.
     * @return Pointer to the current RecordGenericStructType instance.
     */
    RecordGenericStructType* setFuncAST(AST::StructStatement* structAST) {
        this->structAST = structAST;
        return this;
    }

    /**
     * @brief Sets the environment for the generic struct type.
     * @param env Pointer to the environment.
     * @return Pointer to the current RecordGenericStructType instance.
     */
    RecordGenericStructType* setEnv(Enviornment* env) {
        this->env = env;
        return this;
    }
};

/**
 * @struct RecordStructType
 * @brief Represents a struct type record within the environment.
 *
 * This struct manages information about a struct type, including its LLVM
 * struct type, fields, subtypes, methods, and related functionalities.
 */
struct RecordStructType : public Record {
  private:
    std::vector<Str> fields = {}; ///< List of field names in the struct.

  public:
    llvm::Type* stand_alone_type = nullptr;                    ///< Pointer to the standalone LLVM Type.
    llvm::StructType* struct_type = nullptr;                   ///< Pointer to the LLVM StructType.
    std::unordered_map<Str, RecordStructType*> sub_types = {}; ///< Map of subtypes by field name.
    std::vector<RecordStructType*> generic_sub_types = {};     ///< Vector of generic subtypes.
    std::unordered_map<std::string, uint32_t> KW_int_map;
    std::vector<std::tuple<Str, RecordFunction*>> methods = {};
    bool is_enum_kind = false;

    /**
     * @brief Constructs a RecordStructType with the specified name.
     * @param name The name of the struct type.
     */
    RecordStructType(const Str& name) : Record(RecordType::StructInst, name) {}

    /**
     * @brief Constructs a RecordStructType with the specified name and standalone
     * type.
     * @param name The name of the struct type.
     * @param stand_alone_type Pointer to the standalone LLVM Type.
     */
    RecordStructType(const Str& name, llvm::Type* stand_alone_type) : Record(RecordType::StructInst, name), stand_alone_type(stand_alone_type) {}

    /**
     * @brief Copy constructor for RecordStructType.
     * @param other The RecordStructType object to copy from.
     * @param is_on_stack If true, methods are not copied.
     */
    RecordStructType(const RecordStructType& other, bool is_on_stack = false)
        : Record(other), fields(other.fields), stand_alone_type(other.stand_alone_type), struct_type(other.struct_type), sub_types(other.sub_types), generic_sub_types(other.generic_sub_types) {
        if (!is_on_stack) {
            for (const auto& method : other.methods) { methods.push_back({std::get<0>(method), new RecordFunction(*std::get<1>(method))}); }
        }
    }

    RecordStructType(std::string& name, llvm::IntegerType* ll_enum_underthe_hood_type, std::unordered_map<std::string, uint32_t> KW_int_map)
        : Record(RecordType::StructInst, name), stand_alone_type(ll_enum_underthe_hood_type), KW_int_map(KW_int_map), is_enum_kind(true) {};

    /**
     * @brief Destructor for RecordStructType.
     */
    ~RecordStructType() {
        for (auto& [_, method] : methods) { delete method; }
        // for (auto& [_, field] : KW_int_map) { delete field; }
    }

    /**
     * @brief Checks if a method with the given name and parameters exists in the
     * struct.
     * @param name The name of the method.
     * @param params_types Vector of parameter struct types.
     * @param ex_info Extra information map.
     * @param return_type Pointer to the return struct type.
     * @param exact If true, performs an exact match.
     * @return True if the method exists, false otherwise.
     */
    bool is_method(const Str& name, const std::vector<RecordStructType*>& params_types, RecordStructType* return_type = nullptr, bool exact = false, bool is_auto_cast = false);

    bool isVal(std::string name);

    /**
     * @brief Retrieves the method with the given name and parameters.
     * @param name The name of the method.
     * @param params_types Vector of parameter struct types.
     * @param ex_info Extra information map.
     * @param return_type Pointer to the return struct type.
     * @param exact If true, performs an exact match.
     * @return Pointer to the FunctionRecord if found, nullptr otherwise.
     */
    RecordFunction* get_method(const Str& name, const std::vector<RecordStructType*>& params_types, RecordStructType* return_type = nullptr, bool exact = false);

    uint32_t getVal(std::string name);

    /**
     * @brief Sets the standalone LLVM Type.
     * @param stand_alone_type Pointer to the standalone LLVM Type.
     */
    void setStandAloneType(llvm::Type* stand_alone_type) { this->stand_alone_type = stand_alone_type; }

    /**
     * @brief Sets the LLVM StructType pointer.
     * @param struct_type Pointer to the LLVM StructType.
     */
    void setStructType(llvm::StructType* struct_type) { this->struct_type = struct_type; }

    /**
     * @brief Adds a subtype to the struct.
     * @param name The name of the subtype.
     * @param type Pointer to the struct type of the subtype.
     */
    void addSubType(Str name, RecordStructType* type) {
        this->fields.push_back(name);
        this->sub_types[name] = type;
    }

    /**
     * @brief Adds a generic subtype to the struct.
     * @param type Pointer to the generic struct type.
     */
    void addGenericSubType(RecordStructType* type) { this->generic_sub_types.push_back(type); }

    /**
     * @brief Adds a method to the struct.
     * @param name The name of the method.
     * @param type Pointer to the function record of the method.
     */
    void addMethod(Str name, RecordFunction* type) { this->methods.push_back({name, type}); }

    /**
     * @brief Retrieves the list of field names in the struct.
     * @return Reference to the vector of field names.
     */
    std::vector<Str>& getFields() { return this->fields; }
};

/**
 * @struct RecordVariable
 * @brief Represents a variable record within the environment.
 *
 * This struct holds information about a variable, including its LLVM value,
 * allocation instruction, and type.
 */
struct RecordVariable : public Record {
  public:
    llvm::Value* value = nullptr;              ///< Pointer to the LLVM Value representing the variable.
    llvm::Value* allocainst = nullptr;         ///< Pointer to the LLVM Allocation Instruction.
    RecordStructType* variable_type = nullptr; ///< Pointer to the struct type of the variable.
    bool is_const;

    /**
     * @brief Constructs a RecordVariable with the specified name.
     * @param name The name of the variable.
     */
    RecordVariable(const Str& name) : Record(RecordType::Variable, name) {}

    /**
     * @brief Constructs a RecordVariable with detailed parameters.
     * @param name The name of the variable.
     * @param value Pointer to the LLVM Value representing the variable.
     * @param allocainst Pointer to the LLVM Allocation Instruction.
     * @param generic Pointer to the struct type of the variable.
     */
    RecordVariable(const Str& name, llvm::Value* value, llvm::Value* allocainst, RecordStructType* generic, bool is_const = false)
        : Record(RecordType::Variable, name), value(value), allocainst(allocainst), variable_type(generic), is_const(is_const) {}

    /**
     * @brief Copy constructor for RecordVariable.
     * @param other The RecordVariable object to copy from.
     */
    RecordVariable(const RecordVariable& other) : Record(other), value(other.value), allocainst(other.allocainst), variable_type(other.variable_type) {}
};

/**
 * @brief Checks if two struct types are equivalent.
 * @param type1 Pointer to the first struct type.
 * @param type2 Pointer to the second struct type.
 * @return True if both struct types are equivalent, false otherwise.
 */
bool _checkType(RecordStructType* type1, RecordStructType* type2);

/**
 * @brief Checks if two struct types are equivalent.
 * @param type1 Pointer to the first struct type.
 * @param type2 Pointer to the second struct type.
 * @param checked Set of checked type pairs to avoid infinite recursion.
 * @return True if both struct types are equivalent, false otherwise.
 */
bool _checkType(RecordStructType* type1, RecordStructType* type2, std::set<std::pair<RecordStructType*, RecordStructType*>>& checked);

/**
 * @brief Checks if two generic struct types are equivalent.
 * @param type1 Pointer to the first generic struct type.
 * @param type2 Pointer to the second generic struct type.
 * @return True if both generic struct types are equivalent, false otherwise.
 */
bool _checkType(RecordGenericStructType* type1, RecordGenericStructType* type2);

/**
 * @struct RecordModule
 * @brief Represents a module record within the environment.
 *
 * This struct manages a collection of records within a module, allowing
 * for querying and retrieval of functions, structs, and generic entities
 * defined within the module.
 */
struct RecordModule : public Record {
  public:
    StrRecordMap record_map = {}; ///< Holds Records in the module.

    /**
     * @brief Constructs a RecordModule with the specified name and record map.
     * @param name The name of the module.
     * @param record_map The map of records within the module.
     */
    RecordModule(const Str& name, const StrRecordMap& record_map) : Record(RecordType::Module, name), record_map(record_map) {}

    /**
     * @brief Copy constructor for RecordModule.
     * @param other The RecordModule object to copy from.
     */
    RecordModule(const RecordModule& other) : Record(other), record_map(other.record_map) {}

    /**
     * @brief Destructor for RecordModule.
     */
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

    /**
     * @brief Constructs a RecordModule with the specified name.
     * @param name The name of the module.
     */
    RecordModule(const Str& name) : Record(RecordType::Module, name) {}

    /**
     * @brief Adds a record to the module.
     * @param record Pointer to the record to be added.
     */
    void addRecord(Record* record) { record_map.push_back({record->name, record}); }

    /**
     * @brief Checks if a function with the given name and parameters exists in
     * the module.
     * @param name The name of the function.
     * @param params_types Vector of parameter struct types.
     * @param exact If true, performs an exact match.
     * @return True if the function exists, false otherwise.
     */
    bool isFunction(const Str& name, const std::vector<RecordStructType*>& params_types, bool exact = false);

    /**
     * @brief Retrieves a function with the given name and parameters from the
     * module.
     * @param name The name of the function.
     * @param params_types Vector of parameter struct types.
     * @param exact If true, performs an exact match.
     * @return Pointer to the FunctionRecord if found, nullptr otherwise.
     */
    RecordFunction* getFunction(const Str& name, const std::vector<RecordStructType*>& params_types, bool exact = false);

    /**
     * @brief Checks if a generic function with the given name exists in the
     * module.
     * @param name The name of the generic function.
     * @return True if the generic function exists, false otherwise.
     */
    bool isGenericFunc(const Str& name);

    /**
     * @brief Retrieves all generic functions with the given name from the module.
     * @param name The name of the generic function.
     * @return Vector of pointers to generic function records.
     */
    std::vector<RecordGenericFunction*> get_GenericFunc(const Str& name);

    /**
     * @brief Checks if a generic struct with the given name exists in the module.
     * @param name The name of the generic struct.
     * @return True if the generic struct exists, false otherwise.
     */
    bool isGenericStruct(const Str& name);

    /**
     * @brief Retrieves all generic structs with the given name from the module.
     * @param name The name of the generic struct.
     * @return Vector of pointers to generic struct type records.
     */
    std::vector<RecordGenericStructType*> getGenericStruct(const Str& name);

    /**
     * @brief Checks if a struct with the given name exists in the module.
     * @param name The name of the struct.
     * @param gen Vector of generic struct types for parameterized structs.
     * @return True if the struct exists, false otherwise.
     */
    bool is_struct(const Str& name, std::vector<RecordStructType*> gen = {});

    /**
     * @brief Retrieves a struct with the given name from the module.
     * @param name The name of the struct.
     * @param gen Vector of generic struct types for parameterized structs.
     * @return Pointer to the StructTypeRecord if found, nullptr otherwise.
     */
    RecordStructType* get_struct(const Str& name, std::vector<RecordStructType*> gen = {});

    /**
     * @brief Checks if a module with the given name exists within the module.
     * @param name The name of the module.
     * @return True if the module exists, false otherwise.
     */
    bool is_module(const Str& name);

    /**
     * @brief Retrieves a module with the given name from the module.
     * @param name The name of the module.
     * @return Pointer to the RecordModule if found, nullptr otherwise.
     */
    RecordModule* get_module(const Str& name);
};

/**
 * @struct Enviornment
 * @brief Manages the scope and records within the compilation environment.
 *
 * The Enviornment struct maintains a collection of records, handles scope
 * management, and provides functionalities to query and manipulate records
 * such as variables, functions, structs, and modules.
 */
struct Enviornment {
  public:
    Enviornment* parent;     ///< Pointer to the parent environment.
    Str name;                ///< Name of the current environment.
    StrRecordMap record_map; ///< Map of records within the environment.

    RecordFunction* current_function = nullptr; ///< Pointer to the current function record.

    /**
     * @brief Vectors managing loop-related basic blocks for control flow.
     */
    std::vector<llvm::BasicBlock*> loop_body_block = {};      ///< Stack of loop body blocks.
    std::vector<llvm::BasicBlock*> loop_conti_block = {};     ///< Stack of loop continue blocks.
    std::vector<llvm::BasicBlock*> loop_condition_block = {}; ///< Stack of loop condition blocks.
    std::vector<llvm::BasicBlock*> loop_ifbreak_block = {};   ///< Stack of loop if-break blocks.
    std::vector<llvm::BasicBlock*> loop_notbreak_block = {};  ///< Stack of loop not-break blocks.

    std::vector<Enviornment*> childes = {}; ///< Vector of child environments.

    /**
     * @brief Constructs an Enviornment with an optional parent, records, and
     * name.
     * @param parent Pointer to the parent environment (default: nullptr).
     * @param records Vector of records to initialize the environment.
     * @param name Name of the environment (default: "unnamed").
     *
     * If a parent environment is provided, the loop-related basic blocks
     * and the current function pointer are inherited from the parent.
     */
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

    /**
     * @brief Destructor for Enviornment.
     */
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

    /**
     * @brief Adds a record to the environment.
     * @param record Pointer to the record to be added.
     */
    void addRecord(Record* record);

    /**
     * @brief Checks if a variable with the given name exists in the environment.
     * @param name The name of the variable.
     * @param limit2current_scope If true, limits the search to the current scope.
     * @return True if the variable exists, false otherwise.
     */
    bool isVariable(const Str& name, bool limit2current_scope = false);

    /**
     * @brief Retrieves a variable with the given name from the environment.
     * @param name The name of the variable.
     * @param limit2current_scope If true, limits the search to the current scope.
     * @return Pointer to the VariableRecord if found, nullptr otherwise.
     */
    RecordVariable* getVariable(const Str& name, bool limit2current_scope = false);

    /**
     * @brief Checks if a function with the given name and parameters exists in
     * the environment.
     * @param name The name of the function.
     * @param params_types Vector of parameter struct types.
     * @param limit2current_scope If true, limits the search to the current scope.
     * @param exact If true, performs an exact match.
     * @return True if the function exists, false otherwise.
     */
    bool isFunction(const Str& name, std::vector<RecordStructType*> params_types, bool limit2current_scope = false, bool exact = false);

    /**
     * @brief Retrieves a function with the given name and parameters from the
     * environment.
     * @param name The name of the function.
     * @param params_types Vector of parameter struct types.
     * @param limit2current_scope If true, limits the search to the current scope.
     * @param exact If true, performs an exact match.
     * @return Pointer to the FunctionRecord if found, nullptr otherwise.
     */
    RecordFunction* getFunction(const Str& name, std::vector<RecordStructType*> params_types, bool limit2current_scope = false, bool exact = false);

    /**
     * @brief Checks if a struct with the given name exists in the environment.
     * @param name The name of the struct.
     * @param limit2current_scope If true, limits the search to the current scope.
     * @param gen Vector of generic struct types for parameterized structs.
     * @return True if the struct exists, false otherwise.
     */
    bool isStruct(const Str& name, bool limit2current_scope = false, std::vector<RecordStructType*> gen = {});

    /**
     * @brief Retrieves a struct with the given name from the environment.
     * @param name The name of the struct.
     * @param limit2current_scope If true, limits the search to the current scope.
     * @param gen Vector of generic struct types for parameterized structs.
     * @return Pointer to the StructTypeRecord if found, nullptr otherwise.
     */
    RecordStructType* getStruct(const Str& name, bool limit2current_scope = false, std::vector<RecordStructType*> gen = {});

    /**
     * @brief Checks if a module with the given name exists in the environment.
     * @param name The name of the module.
     * @param limit2current_scope If true, limits the search to the current scope.
     * @return True if the module exists, false otherwise.
     */
    bool isModule(const Str& name, bool limit2current_scope = false);

    /**
     * @brief Retrieves a module with the given name from the environment.
     * @param name The name of the module.
     * @param limit2current_scope If true, limits the search to the current scope.
     * @return Pointer to the RecordModule if found, nullptr otherwise.
     */
    RecordModule* getModule(const Str& name, bool limit2current_scope = false);

    /**
     * @brief Checks if a generic function with the given name exists in the
     * environment.
     * @param name The name of the generic function.
     * @return True if the generic function exists, false otherwise.
     */
    bool isGenericFunc(const Str& name);

    std::vector<RecordFunction*> getFunc(const Str& name);

    /**
     * @brief Retrieves all generic functions with the given name from the
     * environment.
     * @param name The name of the generic function.
     * @return Vector of pointers to generic function records.
     */
    std::vector<RecordGenericFunction*> getGenericFunc(const Str& name);

    /**
     * @brief Checks if a generic struct with the given name exists in the
     * environment.
     * @param name The name of the generic struct.
     * @return True if the generic struct exists, false otherwise.
     */
    bool isGenericStruct(const Str& name);

    /**
     * @brief Retrieves all generic structs with the given name from the
     * environment.
     * @param name The name of the generic struct.
     * @return Vector of pointers to generic struct type records.
     */
    std::vector<RecordGenericStructType*> getGenericStruct(const Str& name);

    /**
     * @brief Retrieves all variables in the current environment.
     * @return Vector of pointers to VariableRecords.
     */
    std::vector<RecordVariable*> getCurrentFuncVars();

    /**
     * @brief Enters a new loop scope by adding loop-related basic blocks.
     * @param contiBlock Pointer to the continue basic block.
     * @param bodyBlock Pointer to the body basic block.
     * @param condBlock Pointer to the condition basic block.
     * @param ifBreakBlock Pointer to the if-break basic block.
     * @param ifNotBreakBlock Pointer to the if-not-break basic block.
     */
    void enterLoop(llvm::BasicBlock* contiBlock, llvm::BasicBlock* bodyBlock, llvm::BasicBlock* condBlock, llvm::BasicBlock* ifBreakBlock, llvm::BasicBlock* ifNotBreakBlock) {
        this->loop_conti_block.push_back(contiBlock);
        this->loop_body_block.push_back(bodyBlock);
        this->loop_condition_block.push_back(condBlock);
        this->loop_ifbreak_block.push_back(ifBreakBlock);
        this->loop_notbreak_block.push_back(ifNotBreakBlock);
    }

    /**
     * @brief Exits the current loop scope by removing loop-related basic blocks.
     */
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
