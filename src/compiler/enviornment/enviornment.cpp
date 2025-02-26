#include "enviornment.hpp"
#include "../compiler.hpp"
#include <cstdint>
#include <iostream>
#include <llvm/ADT/STLExtras.h>

// Use the enviornment namespace to simplify code references
using namespace enviornment;

#include <set>
#include <utility> // For std::pair

bool enviornment::_checkType(RecordStructType* type1, RecordStructType* type2) {
    std::set<std::pair<RecordStructType*, RecordStructType*>> checked;
    return _checkType(type1, type2, checked);
}

// Modify the _checkType function to handle checked pairs
bool enviornment::_checkType(RecordStructType* type1, RecordStructType* type2, std::set<std::pair<RecordStructType*, RecordStructType*>>& checked) {
    if (type1 == type2) return true; // Same memory address implies identical types
    if ((type1->struct_type && type2->name == "nullptr") || type2->struct_type && type1->name == "nullptr") return true;

    // Create a pair of the current types being compared
    std::pair<RecordStructType*, RecordStructType*> currentPair = {type1, type2};

    // Check if this pair has already been compared
    if (checked.find(currentPair) != checked.end()) {
        // If already checked, assume equality to prevent infinite recursion
        return true;
    }

    // Mark this pair as checked
    checked.insert(currentPair);

    // Iterate over fields of both types simultaneously
    const auto& fields1 = type1->getFields();
    const auto& fields2 = type2->getFields();

    if (fields1.size() != fields2.size()) return false;

    for (size_t i = 0; i < fields1.size(); ++i) {
        const std::string& field_name1 = fields1[i];
        const std::string& field_name2 = fields2[i];

        // If field names differ, types do not match
        if (field_name1 != field_name2) return false;

        // Retrieve subtypes
        RecordStructType* subtype1 = type1->sub_types.at(field_name1);
        RecordStructType* subtype2 = type2->sub_types.at(field_name2);

        // Recursively check subtypes
        if (!_checkType(subtype1, subtype2, checked)) { return false; }
    }

    if (type1->KW_int_map != type2->KW_int_map) return false;
    for (auto [field1, field2] : llvm::zip(type1->KW_int_map, type2->KW_int_map)) {
        if (field1 != field2) return false;
    }

    if (type1->stand_alone_type == type2->stand_alone_type) return true;
    return type1->name == type2->name;
}

// Helper function to verify if function parameters match
bool _checkFunctionParameterType(RecordFunction* func_record, std::vector<RecordStructType*> params, bool exact = false) {
    if (!exact && func_record->is_var_arg) return true;
    // Compare each argument type with the corresponding parameter type
    for (const auto& [arg, pass_instance] : llvm::zip(func_record->arguments, params)) {
        const auto& [arg_name, accept_instance, _, _] = arg;
        bool types_match = _checkType(accept_instance, pass_instance);
        // Allow type conversion if exact matching is not required
        bool can_convert = !exact && compiler::Compiler::canConvertType(accept_instance, pass_instance);
        if (!(types_match || can_convert)) { return false; }
    }

    // Ensure the number of arguments is correct
    bool correct_arg_count = func_record->is_var_arg || func_record->arguments.size() == params.size();
    return correct_arg_count;
}

// Checks if a struct type has a specific method matching the given criteria
bool RecordStructType::is_method(const std::string& name, const std::vector<RecordStructType*>& params_types, RecordStructType* return_type, bool exact, bool is_autocast) {
    // Iterate through all methods of the struct
    for (const auto& [method_name, method] : this->methods) {
        bool match = (is_autocast ? method->is_auto_cast : true);

        // Check if return type matches (if specified)
        bool return_correct = !return_type || _checkType(return_type, method->return_type);
        // Check if method name matches (if specified)
        bool name_matches = name.empty() || method->name == name;
        // Ensure function parameters match
        bool params_match = _checkFunctionParameterType(method, params_types, exact);

        // If all conditions are met, the method exists
        if (return_correct && match && name_matches && params_match) { return true; }
    }
    return false;
}

// Retrieves a method from a struct type that matches the given criteria
RecordFunction* RecordStructType::get_method(const std::string& name, const std::vector<RecordStructType*>& params_types, RecordStructType* return_type, bool exact) {
    // Iterate through all methods of the struct
    for (const auto& [method_name, method] : this->methods) {
        // Check if return type matches (if specified)
        bool return_correct = !return_type || _checkType(return_type, method->return_type);
        // Check if method name matches (if specified)
        bool name_matches = name.empty() || method->name == name;
        // Ensure function parameters match
        bool params_match = _checkFunctionParameterType(method, params_types);

        // If all conditions are met, return the matching method
        if (return_correct && name_matches && params_match) { return method; }
    }
    // Return nullptr if no matching method is found
    return nullptr;
}

// Checks if a module contains a specific function matching the given criteria
bool RecordModule::isFunction(const std::string& name, const std::vector<RecordStructType*>& params_types, bool exact) {
    // Iterate through all records in the module
    for (const auto& [func_name, func_record] : record_map) {
        // Only consider function records
        if (func_record->type == RecordType::Function) {
            auto func = (RecordFunction*)func_record;
            bool name_matches = (func->name == name);
            bool params_match = _checkFunctionParameterType(func, params_types, exact);

            // Return true if both name and parameters match
            if (name_matches && params_match) { return true; }
        }
    }
    return false;
}

// Checks if a module contains a specific struct with matching generic
// parameters
bool RecordModule::is_struct(const std::string& name, std::vector<RecordStructType*> gens) {
    // Iterate through all records in the module
    for (const auto& [struct_name, struct_record] : record_map) {
        if (struct_record->type == RecordType::StructInst) {
            auto struct_type = (RecordStructType*)struct_record;
            if (struct_type->name == name) {
                bool all_types_match = true;
                // Check each generic type parameter
                for (const auto& [gen, expected_gen] : llvm::zip(gens, struct_type->generic_sub_types)) {
                    if (!_checkType(gen, expected_gen)) {
                        all_types_match = false;
                        break;
                    }
                }
                if (all_types_match) { return true; }
            }
        }
    }
    return false;
}

// Checks if a module contains a specific module by name
bool RecordModule::is_module(const std::string& name) {
    // Iterate through all records in the module
    for (const auto& [module_name, module_record] : record_map) {
        if (module_record->type == RecordType::Module) {
            auto module = (RecordModule*)module_record;
            if (module->name == name) { return true; }
        }
    }
    return false;
}

// Checks if a module contains a specific generic function by name
bool RecordModule::isGenericFunc(const std::string& name) {
    for (const auto& [Gfunc_name, Gfunc_record] : record_map) {
        if (Gfunc_record->type == RecordType::GenericFunction) {
            auto Gfunc = (RecordGenericFunction*)Gfunc_record;
            if (Gfunc->name == name) { return true; }
        }
    }
    return false;
}

// Checks if a module contains a specific generic struct by name
bool RecordModule::isGenericStruct(const std::string& name) {
    for (const auto& [Gstruct_name, Gstruct_record] : record_map) {
        if (Gstruct_record->type == RecordType::GStructType) {
            auto Gstruct = (RecordGenericStructType*)Gstruct_record;
            if (Gstruct->name == name) { return true; }
        }
    }
    return false;
}

// Retrieves a function from the module that matches the given criteria
RecordFunction* RecordModule::getFunction(const std::string& name, const std::vector<RecordStructType*>& params_types, bool exact) {
    for (const auto& [func_name, func_record] : record_map) {
        if (func_record->type == RecordType::Function) {
            auto func = (RecordFunction*)func_record;
            if (func->name == name && _checkFunctionParameterType(func, params_types, exact)) { return func; }
        }
    }
    return nullptr;
}

// Retrieves a struct from the module that matches the given name and generic
// parameters
RecordStructType* RecordModule::get_struct(const std::string& name, std::vector<RecordStructType*> gens) {
    for (const auto& [struct_name, struct_record] : record_map) {
        if (struct_record->type == RecordType::StructInst) {
            auto struct_type = (RecordStructType*)struct_record;
            if (struct_type->name == name) {
                bool types_match = true;
                // Check each generic type parameter
                for (const auto& [gen, expected_gen] : llvm::zip(gens, struct_type->generic_sub_types)) {
                    if (!_checkType(gen, expected_gen)) {
                        types_match = false;
                        break;
                    }
                }
                if (types_match) { return struct_type; }
            }
        }
    }
    return nullptr;
}

// Retrieves a module by name from the current module
RecordModule* RecordModule::get_module(const std::string& name) {
    for (const auto& [module_name, module_record] : record_map) {
        if (module_record->type == RecordType::Module) {
            auto module = (RecordModule*)module_record;
            if (module->name == name) { return module; }
        }
    }
    return nullptr;
}

// Retrieves all generic functions matching the given name
std::vector<RecordGenericFunction*> RecordModule::get_GenericFunc(const std::string& name) {
    std::vector<RecordGenericFunction*> matching_gfuncs;
    for (const auto& [Gf_name, Gf_record] : record_map) {
        if (Gf_record->type == RecordType::GenericFunction) {
            auto Gf = (RecordGenericFunction*)Gf_record;
            if (Gf->name == name) { matching_gfuncs.push_back(Gf); }
        }
    }
    return matching_gfuncs;
}

// Retrieves all generic structs matching the given name
std::vector<RecordGenericStructType*> RecordModule::getGenericStruct(const std::string& name) {
    std::vector<RecordGenericStructType*> matching_gstructs;
    for (const auto& [Gs_name, Gs_record] : record_map) {
        if (Gs_record->type == RecordType::GStructType) {
            auto Gs = (RecordGenericStructType*)Gs_record;
            if (Gs->name == name) { matching_gstructs.push_back(Gs); }
        }
    }
    return matching_gstructs;
}

// Adds a new record to the environment
void Enviornment::addRecord(Record* record) {
    record_map.push_back({record->name, record});
}

// Checks if a variable exists in the environment
bool Enviornment::isVariable(const std::string& name, bool limit2current_scope) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::Variable && record->name == name) { return true; }
    }
    // If not found and not limited to current scope, check parent environments
    return (parent != nullptr && !limit2current_scope) ? parent->isVariable(name) : false;
}

// Checks if a function exists in the environment with matching parameters
bool Enviornment::isFunction(const std::string& name, std::vector<RecordStructType*> params_types, bool limit2current_scope, bool exact) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::Function && record->name == name) {
            auto func = (RecordFunction*)record;
            if (_checkFunctionParameterType(func, params_types, exact)) { return true; }
        }
    }
    // If not found and not limited to current scope, check parent environments
    return (parent != nullptr && !limit2current_scope) ? parent->isFunction(name, params_types, exact) : false;
}

// Checks if a struct exists in the environment with matching generic parameters
bool Enviornment::isStruct(const std::string& name, bool limit2current_scope, std::vector<RecordStructType*> gens) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::StructInst && record->name == name) {
            bool all_types_match = true;
            // Verify each generic type parameter
            for (const auto& [gen, expected_gen] : llvm::zip(gens, ((RecordStructType*)record)->generic_sub_types)) {
                if (!_checkType(gen, expected_gen)) {
                    all_types_match = false;
                    break;
                }
            }
            if (all_types_match) { return true; }
        }
    }
    // If not found and not limited to current scope, check parent environments
    return this->parent ? this->parent->isStruct(name, limit2current_scope, gens) : false;
}

// Checks if a module with the specified name exists in the environment
bool Enviornment::isModule(const std::string& name, bool limit2current_scope) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::Module && record->name == name) { return true; }
    }
    // If not found and not limited to current scope, check parent environments
    return this->parent ? this->parent->isModule(name) : false;
}

// Checks if a generic function exists in the environment
bool Enviornment::isGenericFunc(const std::string& name) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::GenericFunction && record->name == name) { return true; }
    }
    // If not found, check parent environments
    return this->parent ? this->parent->isGenericFunc(name) : false;
}

// Checks if a generic struct exists in the environment
bool Enviornment::isGenericStruct(const std::string& name) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::GStructType && record->name == name) { return true; }
    }
    // If not found, check parent environments
    return this->parent ? this->parent->isGenericStruct(name) : false;
}

// Retrieves a variable from the environment
RecordVariable* Enviornment::getVariable(const std::string& name, bool limit2current_scope) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::Variable && record->name == name) { return (RecordVariable*)record; }
    }
    // If not found and not limited to current scope, check parent environments
    return (parent != nullptr && !limit2current_scope) ? parent->getVariable(name) : nullptr;
}

// Retrieves a function from the environment that matches the given criteria
RecordFunction* Enviornment::getFunction(const std::string& name, std::vector<RecordStructType*> params_types, bool limit2current_scope, bool exact) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::Function && record->name == name) {
            auto func = (RecordFunction*)record;
            if (_checkFunctionParameterType(func, params_types, exact)) { return func; }
        }
    }
    // If not found and not limited to current scope, check parent environments
    return (parent != nullptr && !limit2current_scope) ? parent->getFunction(name, params_types, exact) : nullptr;
}

// Retrieves a struct from the environment that matches the given name and
// generic parameters
RecordStructType* Enviornment::getStruct(const std::string& name, bool limit2current_scope, std::vector<RecordStructType*> gens) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::StructInst && record->name == name) {
            bool types_match = true;
            // Verify each generic type parameter
            for (const auto& [gen, expected_gen] : llvm::zip(gens, ((RecordStructType*)record)->generic_sub_types)) {
                if (!_checkType(gen, expected_gen)) {
                    types_match = false;
                    break;
                }
            }
            if (types_match) { return (RecordStructType*)record; }
        }
    }
    // If not found and not limited to current scope, check parent environments
    return (parent != nullptr && !limit2current_scope) ? parent->getStruct(name, limit2current_scope, gens) : nullptr;
}

// Retrieves a module from the environment by name
RecordModule* Enviornment::getModule(const std::string& name, bool limit2current_scope) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::Module && record->name == name) { return (RecordModule*)record; }
    }
    // If not found and not limited to current scope, check parent environments
    return (parent != nullptr && !limit2current_scope) ? parent->getModule(name) : nullptr;
}

// Retrieves all generic functions with the specified name from the environment
std::vector<RecordGenericFunction*> Enviornment::getGenericFunc(const std::string& name) {
    std::vector<RecordGenericFunction*> matching_gfuncs;
    for (const auto& [Gf_name, Gf_record] : record_map) {
        if (Gf_record->type == RecordType::GenericFunction) {
            auto Gf = (RecordGenericFunction*)Gf_record;
            if (Gf->name == name) { matching_gfuncs.push_back(Gf); }
        }
    }
    // If no matches found in current scope, check parent environments
    return this->parent != nullptr && matching_gfuncs.empty() ? this->parent->getGenericFunc(name) : matching_gfuncs;
}

// Retrieves all generic structs with the specified name from the environment
std::vector<RecordGenericStructType*> Enviornment::getGenericStruct(const std::string& name) {
    std::vector<RecordGenericStructType*> matching_gstructs;
    for (const auto& [Gs_name, Gs_record] : record_map) {
        if (Gs_record->type == RecordType::GStructType) {
            auto Gs = (RecordGenericStructType*)Gs_record;
            if (Gs->name == name) { matching_gstructs.push_back(Gs); }
        }
    }
    // If no matches found in current scope, check parent environments
    return this->parent != nullptr && matching_gstructs.empty() ? this->parent->getGenericStruct(name) : matching_gstructs;
}

// Retrieves all variables in the current environment scope
std::vector<RecordVariable*> Enviornment::getCurrentFuncVars() {
    std::vector<RecordVariable*> vars;
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::Variable) { vars.push_back(((RecordVariable*)record)); }
    }
    if (this->parent && this->parent->current_function) {
        auto parent_vars = this->parent->getCurrentFuncVars();
        vars.insert(vars.end(), parent_vars.begin(), parent_vars.end());
    }
    return vars;
}

std::vector<RecordFunction*> Enviornment::getFunc(const Str& name) {
    std::vector<RecordFunction*> jadu;
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::Function && record->name == name) { jadu.push_back((RecordFunction*)record); }
    }
    return this->parent && jadu.size() == 0 ? this->parent->getFunc(name) : jadu;
}

uint32_t RecordStructType::getVal(std::string name) {
    return KW_int_map.contains(name) ? KW_int_map[name] : INT32_MAX;
}

bool RecordStructType::isVal(std::string name) {
    return KW_int_map.contains(name);
};
