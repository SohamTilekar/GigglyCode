#ifndef ENVIORNMENT_HPP
#define ENVIORNMENT_HPP

/**
 * @file enviornment.hpp
 * @brief Defines the environment classes for managing compilation records and scopes.
 *
 * This header file declares several classes within the `enviornment` namespace
 * that are responsible for managing records of variables, functions, structs,
 * modules, and their respective environments during the compilation process.
 */

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

/**
 * @class Record
 * @brief Base class representing a compilation record.
 *
 * The Record class serves as the base class for various types of records
 * such as functions, variables, structs, and modules. It holds common
 * information like the record type, name, metadata, and any extra information.
 */
class Record;

/**
 * @class RecordVariable
 * @brief Represents a variable record within the environment.
 *
 * This class holds information about a variable, including its LLVM value,
 * allocation instruction, and type.
 */
class RecordVariable;

/**
 * @class RecordFunction
 * @brief Represents a function record within the environment.
 *
 * This class holds information about a function, including its LLVM function,
 * function type, arguments, return type, and other related data.
 */
class RecordFunction;

/**
 * @class RecordStructType
 * @brief Represents a struct type record within the environment.
 *
 * This class manages information about a struct type, including its LLVM
 * struct type, fields, subtypes, methods, and related functionalities.
 */
class RecordStructType;

/**
 * @class Enviornment
 * @brief Manages the scope and records within the compilation environment.
 *
 * The Enviornment class maintains a collection of records, handles scope
 * management, and provides functionalities to query and manipulate records
 * such as variables, functions, structs, and modules.
 */
class Enviornment;

/**
 * @class RecordGenericFunction
 * @brief Represents a generic function record within the environment.
 *
 * This class holds information about a generic function, including its
 * associated AST node and environment.
 */
class RecordGenericFunction;

/**
 * @class RecordGenericStructType
 * @brief Represents a generic struct type record within the environment.
 *
 * This class manages information about a generic struct type, including its
 * associated AST node and environment.
 */
class RecordGenericStructType;

/**
 * @class RecordModule
 * @brief Represents a module record within the environment.
 *
 * This class manages a collection of records within a module, allowing
 * for querying and retrieval of functions, structs, and generic entities
 * defined within the module.
 */
class RecordModule;

// Type Aliases for Improved Readability

using Str = std::string; ///< Alias for std::string.
using Any = std::any;    ///< Alias for std::any.
using std::shared_ptr;
using StrAnyMap = std::unordered_map<Str, Any>;                        ///< Map from string to any type.
using StrRecordMap = std::vector<std::tuple<Str, shared_ptr<Record>>>; ///< Vector of tuples mapping strings to records.
using StructTypePtr = shared_ptr<RecordStructType>;                    ///< Shared pointer to RecordStructType.
using FunctionPtr = shared_ptr<RecordFunction>;                        ///< Shared pointer to RecordFunction.
using GenericFunctionPtr = shared_ptr<RecordGenericFunction>;          ///< Shared pointer to RecordGenericFunction.
using GenericStructTypePtr = shared_ptr<RecordGenericStructType>;      ///< Shared pointer to RecordGenericStructType.
using ModulePtr = shared_ptr<RecordModule>;                            ///< Shared pointer to RecordModule.
using VariablePtr = shared_ptr<RecordVariable>;                        ///< Shared pointer to RecordVariable.
using EnviornmentPtr = shared_ptr<Enviornment>;                        ///< Shared pointer to Enviornment.
using ASTFunctionStatementPtr = shared_ptr<AST::FunctionStatement>;    ///< Shared pointer to AST FunctionStatement.
using ASTStructStatementPtr = shared_ptr<AST::StructStatement>;        ///< Shared pointer to AST StructStatement.

/**
 * @enum RecordType
 * @brief Enumeration of different types of records.
 */
enum class RecordType {
    RecordGStructType,    ///< Represents a generic struct type record.
    RecordStructInst,     ///< Represents a struct instance record.
    RecordVariable,       ///< Represents a variable record.
    RecordFunction,       ///< Represents a function record.
    RecordModule,         ///< Represents a module record.
    RecordGenericFunction ///< Represents a generic function record.
};

/**
 * @class Record
 * @brief Base class representing a compilation record.
 *
 * The Record class serves as the base class for various types of records
 * used during the compilation process. It contains common attributes such
 * as the record type, name, metadata, and any additional information.
 */
class Record {
  public:
    RecordType type;           ///< The type of the record.
    Str name;                  ///< The name of the record.
    AST::MetaData meta_data;   ///< Metadata associated with the record.
    StrAnyMap extra_info = {}; ///< Additional information stored in the record.

    /**
     * @brief Sets the metadata for the record.
     * @param stLineNo Starting line number.
     * @param stColNo Starting column number.
     * @param endLineNo Ending line number.
     * @param endColNo Ending column number.
     */
    virtual inline void set_meta_data(int stLineNo, int stColNo, int endLineNo, int endColNo) {
        this->meta_data.st_line_no = stLineNo;
        this->meta_data.st_col_no = stColNo;
        this->meta_data.end_line_no = endLineNo;
        this->meta_data.end_col_no = endColNo;
    }

    /**
     * @brief Constructs a Record with the specified type and name.
     * @param type The type of the record.
     * @param name The name of the record.
     * @param extraInfo Optional extra information for the record.
     */
    Record(const RecordType& type, const Str& name, const StrAnyMap& extraInfo = {}) : type(type), name(name), extra_info(extraInfo) {}
}; // class Record

/**
 * @class RecordFunction
 * @brief Represents a function record within the environment.
 *
 * This class holds information about a function, including its LLVM function,
 * function type, arguments, return type, and other related data.
 */
class RecordFunction : public Record {
  public:
    llvm::Function* function = nullptr;          ///< Pointer to the LLVM Function.
    llvm::FunctionType* function_type = nullptr; ///< Pointer to the LLVM FunctionType.
    /**
     * @brief Vector of arguments where each argument is a tuple containing:
     * - Name of the argument.
     * - Pointer to the struct type of the argument.
     * - Boolean indicating if the argument is passed by reference.
     */
    std::vector<std::tuple<Str, StructTypePtr, bool>> arguments;
    StructTypePtr return_type; ///< Pointer to the struct type of the return value.
    bool is_var_arg = false;   ///< Indicates if the function accepts variable number of arguments.
    EnviornmentPtr env;        ///< Pointer to the environment in which the function is defined.

    /**
     * @brief Constructs a RecordFunction with the given name.
     * @param name The name of the function.
     */
    RecordFunction(const Str& name) : Record(RecordType::RecordFunction, name) {}

    /**
     * @brief Constructs a RecordFunction with detailed parameters.
     * @param name The name of the function.
     * @param function Pointer to the LLVM Function.
     * @param functionType Pointer to the LLVM FunctionType.
     * @param arguments Vector of arguments.
     * @param returnInst Pointer to the struct type of the return value.
     * @param extraInfo Optional extra information.
     */
    RecordFunction(const Str& name,
                   llvm::Function* function,
                   llvm::FunctionType* functionType,
                   std::vector<std::tuple<Str, StructTypePtr, bool>> arguments,
                   StructTypePtr returnInst,
                   const StrAnyMap& extraInfo = {})
        : Record(RecordType::RecordFunction, name, extraInfo), function(function), function_type(functionType), arguments(arguments), return_type(returnInst) {}

    /**
     * @brief Constructs a RecordFunction with variable arguments support.
     * @param name The name of the function.
     * @param function Pointer to the LLVM Function.
     * @param functionType Pointer to the LLVM FunctionType.
     * @param arguments Vector of arguments.
     * @param returnInst Pointer to the struct type of the return value.
     * @param isVarArg Indicates if the function accepts variable arguments.
     */
    RecordFunction(const Str& name, llvm::Function* function, llvm::FunctionType* functionType, std::vector<std::tuple<Str, StructTypePtr, bool>> arguments, StructTypePtr returnInst, bool isVarArg)
        : Record(RecordType::RecordFunction, name), function(function), function_type(functionType), arguments(arguments), return_type(returnInst), is_var_arg(isVarArg) {}

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
    RecordFunction* setArguments(const std::vector<std::tuple<Str, StructTypePtr, bool>>& arguments) {
        this->arguments = arguments;
        return this;
    }

    /**
     * @brief Adds a single argument to the function.
     * @param argument Tuple containing argument details.
     * @return Pointer to the current RecordFunction instance.
     */
    RecordFunction* addArgument(const std::tuple<Str, StructTypePtr, bool>& argument) {
        arguments.push_back(argument);
        return this;
    }

    /**
     * @brief Sets the return type of the function.
     * @param return_type Pointer to the struct type of the return value.
     * @return Pointer to the current RecordFunction instance.
     */
    RecordFunction* setRetiType(StructTypePtr return_type) {
        this->return_type = return_type;
        return this;
    }

    /**
     * @brief Sets the environment in which the function is defined.
     * @param env Pointer to the environment.
     * @return Pointer to the current RecordFunction instance.
     */
    RecordFunction* setEnv(EnviornmentPtr env) {
        this->env = env;
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
 * @class RecordGenericFunction
 * @brief Represents a generic function record within the environment.
 *
 * This class holds information about a generic function, including its
 * associated AST node and environment.
 */
class RecordGenericFunction : public Record {
  public:
    ASTFunctionStatementPtr func = nullptr; ///< Pointer to the AST FunctionStatement.
    EnviornmentPtr env = nullptr;           ///< Pointer to the environment.

    /**
     * @brief Constructs a RecordGenericFunction with the specified name, function, and environment.
     * @param name The name of the generic function.
     * @param func Pointer to the AST FunctionStatement.
     * @param env Pointer to the environment.
     */
    RecordGenericFunction(const Str& name, ASTFunctionStatementPtr func, EnviornmentPtr env) : Record(RecordType::RecordGenericFunction, name), func(func), env(env) {}

    /**
     * @brief Sets the AST FunctionStatement pointer.
     * @param funcAST Pointer to the AST FunctionStatement.
     * @return Pointer to the current RecordGenericFunction instance.
     */
    RecordGenericFunction* setFuncAST(ASTFunctionStatementPtr funcAST) {
        func = funcAST;
        return this;
    }

    /**
     * @brief Sets the environment for the generic function.
     * @param env Pointer to the environment.
     * @return Pointer to the current RecordGenericFunction instance.
     */
    RecordGenericFunction* setEnv(EnviornmentPtr env) {
        this->env = env;
        return this;
    }
};

/**
 * @class RecordGenericStructType
 * @brief Represents a generic struct type record within the environment.
 *
 * This class manages information about a generic struct type, including its
 * associated AST node and environment.
 */
class RecordGenericStructType : public Record {
  public:
    ASTStructStatementPtr structAST = nullptr; ///< Pointer to the AST StructStatement.
    EnviornmentPtr env = nullptr;              ///< Pointer to the environment.

    /**
     * @brief Constructs a RecordGenericStructType with the specified name, struct AST, and environment.
     * @param name The name of the generic struct type.
     * @param structAST Pointer to the AST StructStatement.
     * @param env Pointer to the environment.
     */
    RecordGenericStructType(const Str& name, ASTStructStatementPtr structAST, EnviornmentPtr env) : Record(RecordType::RecordGStructType, name), structAST(structAST), env(env) {}

    /**
     * @brief Sets the AST StructStatement pointer.
     * @param structAST Pointer to the AST StructStatement.
     * @return Pointer to the current RecordGenericStructType instance.
     */
    RecordGenericStructType* setFuncAST(ASTStructStatementPtr structAST) {
        this->structAST = structAST;
        return this;
    }

    /**
     * @brief Sets the environment for the generic struct type.
     * @param env Pointer to the environment.
     * @return Pointer to the current RecordGenericStructType instance.
     */
    RecordGenericStructType* setEnv(EnviornmentPtr env) {
        this->env = env;
        return this;
    }
};

/**
 * @class RecordStructType
 * @brief Represents a struct type record within the environment.
 *
 * This class manages information about a struct type, including its LLVM
 * struct type, fields, subtypes, methods, and related functionalities.
 */
class RecordStructType : public Record {
  private:
    std::vector<Str> fields = {}; ///< List of field names in the struct.

  public:
    llvm::Type* stand_alone_type = nullptr;                ///< Pointer to the standalone LLVM Type.
    llvm::StructType* struct_type = nullptr;               ///< Pointer to the LLVM StructType.
    std::unordered_map<Str, StructTypePtr> sub_types = {}; ///< Map of subtypes by field name.
    std::vector<StructTypePtr> generic_sub_types = {};     ///< Vector of generic subtypes.
    /**
     * @brief Vector of methods where each method is a tuple containing:
     * - Method name.
     * - Pointer to the function record.
     */
    std::vector<std::tuple<Str, FunctionPtr>> methods = {};
    FunctionPtr gc_struct_clear = nullptr; ///< Pointer to the garbage collection clear function.

    /**
     * @brief Constructs a RecordStructType with the specified name.
     * @param name The name of the struct type.
     */
    RecordStructType(const Str& name) : Record(RecordType::RecordStructInst, name) {}

    /**
     * @brief Constructs a RecordStructType with the specified name and standalone type.
     * @param name The name of the struct type.
     * @param stand_alone_type Pointer to the standalone LLVM Type.
     */
    RecordStructType(const Str& name, llvm::Type* stand_alone_type) : Record(RecordType::RecordStructInst, name), stand_alone_type(stand_alone_type) {}

    /**
     * @brief Checks if a method with the given name and parameters exists in the struct.
     * @param name The name of the method.
     * @param params_types Vector of parameter struct types.
     * @param ex_info Extra information map.
     * @param return_type Pointer to the return struct type.
     * @param exact If true, performs an exact match.
     * @return True if the method exists, false otherwise.
     */
    bool is_method(const Str& name, const std::vector<StructTypePtr>& params_types, const StrAnyMap& ex_info = {}, StructTypePtr return_type = nullptr, bool exact = false);

    /**
     * @brief Retrieves the method with the given name and parameters.
     * @param name The name of the method.
     * @param params_types Vector of parameter struct types.
     * @param ex_info Extra information map.
     * @param return_type Pointer to the return struct type.
     * @param exact If true, performs an exact match.
     * @return Pointer to the FunctionRecord if found, nullptr otherwise.
     */
    FunctionPtr get_method(const Str& name, const std::vector<StructTypePtr>& params_types, const StrAnyMap& ex_info = {}, StructTypePtr return_type = nullptr, bool exact = false);

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
    void addSubType(Str name, StructTypePtr type) {
        this->fields.push_back(name);
        this->sub_types[name] = type;
    }

    /**
     * @brief Adds a generic subtype to the struct.
     * @param type Pointer to the generic struct type.
     */
    void addGenericSubType(StructTypePtr type) { this->generic_sub_types.push_back(type); }

    /**
     * @brief Adds a method to the struct.
     * @param name The name of the method.
     * @param type Pointer to the function record of the method.
     */
    void addMethod(Str name, FunctionPtr type) { this->methods.push_back({name, type}); }

    /**
     * @brief Retrieves the list of field names in the struct.
     * @return Reference to the vector of field names.
     */
    std::vector<Str>& getFields() { return this->fields; }
};

/**
 * @class RecordVariable
 * @brief Represents a variable record within the environment.
 *
 * This class holds information about a variable, including its LLVM value,
 * allocation instruction, and type.
 */
class RecordVariable : public Record {
  public:
    llvm::Value* value = nullptr;          ///< Pointer to the LLVM Value representing the variable.
    llvm::Value* allocainst = nullptr;     ///< Pointer to the LLVM Allocation Instruction.
    StructTypePtr variable_type = nullptr; ///< Pointer to the struct type of the variable.

    /**
     * @brief Constructs a RecordVariable with the specified name.
     * @param name The name of the variable.
     */
    RecordVariable(const Str& name) : Record(RecordType::RecordVariable, name) {}

    /**
     * @brief Constructs a RecordVariable with detailed parameters.
     * @param name The name of the variable.
     * @param value Pointer to the LLVM Value representing the variable.
     * @param allocainst Pointer to the LLVM Allocation Instruction.
     * @param generic Pointer to the struct type of the variable.
     */
    RecordVariable(const Str& name, llvm::Value* value, llvm::Value* allocainst, StructTypePtr generic)
        : Record(RecordType::RecordVariable, name), value(value), allocainst(allocainst), variable_type(generic) {}
};

/**
 * @brief Checks if two struct types are equivalent.
 * @param type1 Pointer to the first struct type.
 * @param type2 Pointer to the second struct type.
 * @return True if both struct types are equivalent, false otherwise.
 */
bool _checkType(StructTypePtr type1, StructTypePtr type2);

/**
 * @brief Checks if two generic struct types are equivalent.
 * @param type1 Pointer to the first generic struct type.
 * @param type2 Pointer to the second generic struct type.
 * @return True if both generic struct types are equivalent, false otherwise.
 */
bool _checkType(GenericStructTypePtr type1, GenericStructTypePtr type2);

/**
 * @class RecordModule
 * @brief Represents a module record within the environment.
 *
 * This class manages a collection of records within a module, allowing
 * for querying and retrieval of functions, structs, and generic entities
 * defined within the module.
 */
class RecordModule : public Record {
  public:
    StrRecordMap record_map = {}; ///< Holds Records in the module.

    /**
     * @brief Constructs a RecordModule with the specified name and record map.
     * @param name The name of the module.
     * @param record_map The map of records within the module.
     */
    RecordModule(const Str& name, const StrRecordMap& record_map) : Record(RecordType::RecordModule, name), record_map(record_map) {}

    /**
     * @brief Constructs a RecordModule with the specified name.
     * @param name The name of the module.
     */
    RecordModule(const Str& name) : Record(RecordType::RecordModule, name) {}

    /**
     * @brief Adds a record to the module.
     * @param record Shared pointer to the record to be added.
     */
    void addRecord(shared_ptr<Record> record) { record_map.push_back({record->name, record}); }

    /**
     * @brief Checks if a function with the given name and parameters exists in the module.
     * @param name The name of the function.
     * @param params_types Vector of parameter struct types.
     * @param exact If true, performs an exact match.
     * @return True if the function exists, false otherwise.
     */
    bool isFunction(const Str& name, const std::vector<StructTypePtr>& params_types, bool exact = false);

    /**
     * @brief Retrieves a function with the given name and parameters from the module.
     * @param name The name of the function.
     * @param params_types Vector of parameter struct types.
     * @param exact If true, performs an exact match.
     * @return Pointer to the FunctionRecord if found, nullptr otherwise.
     */
    FunctionPtr getFunction(const Str& name, const std::vector<StructTypePtr>& params_types, bool exact = false);

    /**
     * @brief Checks if a generic function with the given name exists in the module.
     * @param name The name of the generic function.
     * @return True if the generic function exists, false otherwise.
     */
    bool isGenericFunc(const Str& name);

    /**
     * @brief Retrieves all generic functions with the given name from the module.
     * @param name The name of the generic function.
     * @return Vector of pointers to generic function records.
     */
    std::vector<GenericFunctionPtr> get_GenericFunc(const Str& name);

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
    std::vector<GenericStructTypePtr> getGenericStruct(const Str& name);

    /**
     * @brief Checks if a struct with the given name exists in the module.
     * @param name The name of the struct.
     * @param gen Vector of generic struct types for parameterized structs.
     * @return True if the struct exists, false otherwise.
     */
    bool is_struct(const Str& name, std::vector<StructTypePtr> gen = {});

    /**
     * @brief Retrieves a struct with the given name from the module.
     * @param name The name of the struct.
     * @param gen Vector of generic struct types for parameterized structs.
     * @return Pointer to the StructTypeRecord if found, nullptr otherwise.
     */
    StructTypePtr get_struct(const Str& name, std::vector<StructTypePtr> gen = {});

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
    ModulePtr get_module(const Str& name);
};

/**
 * @class Enviornment
 * @brief Manages the scope and records within the compilation environment.
 *
 * The Enviornment class maintains a collection of records, handles scope
 * management, and provides functionalities to query and manipulate records
 * such as variables, functions, structs, and modules.
 */
class Enviornment {
  public:
    EnviornmentPtr parent;   ///< Pointer to the parent environment.
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

    /**
     * @brief Constructs an Enviornment with an optional parent, records, and name.
     * @param parent Pointer to the parent environment (default: nullptr).
     * @param records Vector of records to initialize the environment.
     * @param name Name of the environment (default: "unnamed").
     *
     * If a parent environment is provided, the loop-related basic blocks
     * and the current function pointer are inherited from the parent.
     */
    Enviornment(EnviornmentPtr parent = nullptr, const StrRecordMap& records = {}, Str name = "unnamed") : parent(parent), name(name), record_map(records) {
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
     * @brief Adds a record to the environment.
     * @param record Shared pointer to the record to be added.
     */
    void addRecord(shared_ptr<Record> record);

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
    VariablePtr getVariable(const Str& name, bool limit2current_scope = false);

    /**
     * @brief Checks if a function with the given name and parameters exists in the environment.
     * @param name The name of the function.
     * @param params_types Vector of parameter struct types.
     * @param limit2current_scope If true, limits the search to the current scope.
     * @param exact If true, performs an exact match.
     * @return True if the function exists, false otherwise.
     */
    bool isFunction(const Str& name, std::vector<StructTypePtr> params_types, bool limit2current_scope = false, bool exact = false);

    /**
     * @brief Retrieves a function with the given name and parameters from the environment.
     * @param name The name of the function.
     * @param params_types Vector of parameter struct types.
     * @param limit2current_scope If true, limits the search to the current scope.
     * @param exact If true, performs an exact match.
     * @return Pointer to the FunctionRecord if found, nullptr otherwise.
     */
    FunctionPtr getFunction(const Str& name, std::vector<StructTypePtr> params_types, bool limit2current_scope = false, bool exact = false);

    /**
     * @brief Checks if a struct with the given name exists in the environment.
     * @param name The name of the struct.
     * @param limit2current_scope If true, limits the search to the current scope.
     * @param gen Vector of generic struct types for parameterized structs.
     * @return True if the struct exists, false otherwise.
     */
    bool isStruct(const Str& name, bool limit2current_scope = false, std::vector<StructTypePtr> gen = {});

    /**
     * @brief Retrieves a struct with the given name from the environment.
     * @param name The name of the struct.
     * @param limit2current_scope If true, limits the search to the current scope.
     * @param gen Vector of generic struct types for parameterized structs.
     * @return Pointer to the StructTypeRecord if found, nullptr otherwise.
     */
    StructTypePtr getStruct(const Str& name, bool limit2current_scope = false, std::vector<StructTypePtr> gen = {});

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
    ModulePtr getModule(const Str& name, bool limit2current_scope = false);

    /**
     * @brief Checks if a generic function with the given name exists in the environment.
     * @param name The name of the generic function.
     * @return True if the generic function exists, false otherwise.
     */
    bool isGenericFunc(const Str& name);

    /**
     * @brief Retrieves all generic functions with the given name from the environment.
     * @param name The name of the generic function.
     * @return Vector of pointers to generic function records.
     */
    std::vector<GenericFunctionPtr> getGenericFunc(const Str& name);

    /**
     * @brief Checks if a generic struct with the given name exists in the environment.
     * @param name The name of the generic struct.
     * @return True if the generic struct exists, false otherwise.
     */
    bool isGenericStruct(const Str& name);

    /**
     * @brief Retrieves all generic structs with the given name from the environment.
     * @param name The name of the generic struct.
     * @return Vector of pointers to generic struct type records.
     */
    std::vector<GenericStructTypePtr> getGenericStruct(const Str& name);

    /**
     * @brief Retrieves all variables in the current environment.
     * @return Vector of pointers to VariableRecords.
     */
    std::vector<VariablePtr> getCurrentVars();

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
}; // class Enviornment

} // namespace enviornment

#endif // ENVIORNMENT_HPP
