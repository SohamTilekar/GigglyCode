#include "enviornment.hpp"
#include "../compiler.hpp"
#include <iostream>
#include <memory>

using namespace enviornment;

bool enviornment::_checkType(StructTypePtr type1, StructTypePtr type2) {
    for (const auto& [field_name1, field_name2] : llvm::zip(type1->getFields(), type2->getFields())) {
        if (field_name1 != field_name2) {
            return false;
        }
        if (!_checkType(type1->sub_types.at(field_name1), type2->sub_types.at(field_name2))) {
            return false;
        }
    }
    return type1->stand_alone_type == type2->stand_alone_type;
}

bool _checkFunctionParameterType(FunctionPtr func_record, std::vector<StructTypePtr> params, bool exact = false) {
    for (const auto& [arg, pass_instance] : llvm::zip(func_record->arguments, params)) {
        const auto& [arg_name, accept_instance, _] = arg;
        bool types_match = _checkType(accept_instance, pass_instance);
        bool can_convert = !exact && compiler::Compiler::canConvertType(accept_instance, pass_instance);

        if (!(types_match || can_convert)) {
            return false;
        }
    }

    bool correct_arg_count = func_record->is_var_arg || func_record->arguments.size() == params.size();
    return correct_arg_count;
}

bool RecordStructType::is_method(const std::string& name, const std::vector<StructTypePtr>& params_types, const std::unordered_map<std::string, std::any>& ex_info, StructTypePtr return_type,
                                 bool exact) {
    for (const auto& [method_name, method] : this->methods) {
        // Check if all keys in ex_info are present in this->extra_info and their values match
        bool match = true;
        for (const auto& [key, value] : ex_info) {
            if (key == "autocast") {
                auto it = method->extra_info.find(key);
                if (it == method->extra_info.end() || std::any_cast<bool>(it->second) != std::any_cast<bool>(value)) {
                    match = false;
                    break;
                }
            } else {
                std::cerr << "Unsupported key found in ex_info: " << key << std::endl;
                throw std::runtime_error("Unsupported key found in ex_info: " + key);
            }
        }

        bool return_correct = !return_type || _checkType(return_type, method->return_type);
        bool name_matches = name.empty() || method->name == name;
        bool params_match = _checkFunctionParameterType(method, params_types, exact);
        if (return_correct && match && name_matches && params_match) {
            return true;
        }
    }
    return false;
}


FunctionPtr RecordStructType::get_method(const std::string& name, const std::vector<StructTypePtr>& params_types, const std::unordered_map<std::string, std::any>& ex_info, StructTypePtr return_type,
                                         bool exact) {
    for (const auto& [method_name, method] : this->methods) {
        // Check if all keys in ex_info are present in this->extra_info and their values match
        bool match = true;
        for (const auto& [key, value] : ex_info) {
            if (key == "autocast") {
                auto it = method->extra_info.find(key);
                if (it == method->extra_info.end() || std::any_cast<bool>(it->second) != std::any_cast<bool>(value)) {
                    match = false;
                    break;
                }
            } else {
                throw std::runtime_error("Unsupported key found in ex_info: " + key);
            }
        }

        bool return_correct = !return_type || _checkType(return_type, method->return_type);
        bool name_matches = name.empty() || method->name == name;
        bool params_match = _checkFunctionParameterType(method, params_types);

        if (return_correct && match && name_matches && params_match) {
            return method;
        }
    }
    return nullptr;
}

bool RecordModule::isFunction(const std::string& name, const std::vector<StructTypePtr>& params_types, bool exact) {
    for (const auto& [func_name, func_record] : record_map) {
        if (func_record->type == RecordType::RecordFunction) {
            auto func = std::static_pointer_cast<RecordFunction>(func_record);
            bool name_matches = (func->name == name);
            bool params_match = _checkFunctionParameterType(func, params_types, exact);

            if (name_matches && params_match) {
                return true;
            }
        }
    }
    return false;
}

bool RecordModule::is_struct(const std::string& name, std::vector<StructTypePtr> gens) {
    for (const auto& [struct_name, struct_record] : record_map) {
        if (struct_record->type == RecordType::RecordStructInst) {
            auto struct_type = std::static_pointer_cast<RecordStructType>(struct_record);
            if (struct_type->name == name) {
                bool all_types_match = true;
                for (const auto& [gen, expected_gen] : llvm::zip(gens, struct_type->generic_sub_types)) {
                    if (!_checkType(gen, expected_gen)) {
                        all_types_match = false;
                        break;
                    }
                }
                if (all_types_match) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool RecordModule::is_module(const std::string& name) {
    for (const auto& [module_name, module_record] : record_map) {
        if (module_record->type == RecordType::RecordModule) {
            auto module = std::static_pointer_cast<RecordModule>(module_record);
            if (module->name == name) {
                return true;
            }
        }
    }
    return false;
}

bool RecordModule::isGenericFunc(const std::string& name) {
    for (const auto& [Gfunc_name, Gfunc_record] : record_map) {
        if (Gfunc_record->type == RecordType::RecordGenericFunction) {
            auto Gfunc = std::static_pointer_cast<RecordGenericFunction>(Gfunc_record);
            if (Gfunc->name == name) {
                return true;
            }
        }
    }
    return false;
}

bool RecordModule::isGenericStruct(const std::string& name) {
    for (const auto& [Gstruct_name, Gstruct_record] : record_map) {
        if (Gstruct_record->type == RecordType::RecordGStructType) {
            auto Gstruct = std::static_pointer_cast<RecordGenericStructType>(Gstruct_record);
            if (Gstruct->name == name) {
                return true;
            }
        }
    }
    return false;
}

FunctionPtr RecordModule::getFunction(const std::string& name, const std::vector<StructTypePtr>& params_types, bool exact) {
    for (const auto& [func_name, func_record] : record_map) {
        if (func_record->type == RecordType::RecordFunction) {
            auto func = std::static_pointer_cast<RecordFunction>(func_record);
            if (func->name == name && _checkFunctionParameterType(func, params_types, exact)) {
                return func;
            }
        }
    }
    return nullptr;
}

StructTypePtr RecordModule::get_struct(const std::string& name, std::vector<StructTypePtr> gens) {
    for (const auto& [struct_name, struct_record] : record_map) {
        if (struct_record->type == RecordType::RecordStructInst) {
            auto struct_type = std::static_pointer_cast<RecordStructType>(struct_record);
            if (struct_type->name == name) {
                for (const auto& [gen, expected_gen] : llvm::zip(gens, struct_type->generic_sub_types)) {
                    if (!_checkType(gen, expected_gen)) {
                        continue;
                    }
                }
                return struct_type;
            }
        }
    }
    return nullptr;
}

ModulePtr RecordModule::get_module(const std::string& name) {
    for (const auto& [module_name, module_record] : record_map) {
        if (module_record->type == RecordType::RecordModule) {
            auto module = std::static_pointer_cast<RecordModule>(module_record);
            if (module->name == name) {
                return module;
            }
        }
    }
    return nullptr;
}

std::vector<GenericFunctionPtr> RecordModule::get_GenericFunc(const std::string& name) {
    std::vector<GenericFunctionPtr> matching_gfuncs;
    for (const auto& [Gf_name, Gf_record] : record_map) {
        if (Gf_record->type == RecordType::RecordGenericFunction) {
            auto Gf = std::static_pointer_cast<RecordGenericFunction>(Gf_record);
            if (Gf->name == name) {
                matching_gfuncs.push_back(Gf);
            }
        }
    }
    return matching_gfuncs;
}

std::vector<GenericStructTypePtr> RecordModule::getGenericStruct(const std::string& name) {
    std::vector<GenericStructTypePtr> matching_gstructs;
    for (const auto& [Gs_name, Gs_record] : record_map) {
        if (Gs_record->type == RecordType::RecordGStructType) {
            auto Gs = std::static_pointer_cast<RecordGenericStructType>(Gs_record);
            if (Gs->name == name) {
                matching_gstructs.push_back(Gs);
            }
        }
    }
    return matching_gstructs;
}

void Enviornment::addRecord(std::shared_ptr<Record> record) { record_map.push_back({record->name, record}); }

bool Enviornment::isVariable(const std::string& name, bool limit2current_scope) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordVariable && record->name == name) {
            return true;
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->isVariable(name) : false;
}

bool Enviornment::isFunction(const std::string& name, std::vector<StructTypePtr> params_types, bool limit2current_scope, bool exact) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordFunction && record->name == name) {
            auto func = std::static_pointer_cast<RecordFunction>(record);
            if (_checkFunctionParameterType(func, params_types, exact)) {
                return true;
            }
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->isFunction(name, params_types, exact) : false;
}

bool Enviornment::isStruct(const std::string& name, bool limit2current_scope, std::vector<StructTypePtr> gens) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordStructInst && record->name == name) {
            for (const auto& [gen, expected_gen] : llvm::zip(gens, std::static_pointer_cast<RecordStructType>(record)->generic_sub_types)) {
                if (!_checkType(gen, expected_gen)) {
                    continue;
                }
            }
            return true;
        }
    }
    return this->parent ? this->parent->isStruct(name) : false;
}

bool Enviornment::isModule(const std::string& name, bool limit2current_scope) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordModule && record->name == name) {
            return true;
        }
    }
    return this->parent ? this->parent->isModule(name) : false;
}

bool Enviornment::isGenericFunc(const std::string& name) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordGenericFunction && record->name == name) {
            return true;
        }
    }
    return this->parent ? this->parent->isGenericFunc(name) : false;
}

bool Enviornment::isGenericStruct(const std::string& name) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordGStructType && record->name == name) {
            return true;
        }
    }
    return this->parent ? this->parent->isGenericStruct(name) : false;
}

VariablePtr Enviornment::getVariable(const std::string& name, bool limit2current_scope) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordVariable && record->name == name) {
            return std::static_pointer_cast<RecordVariable>(record);
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->getVariable(name) : nullptr;
}

FunctionPtr Enviornment::getFunction(const std::string& name, std::vector<StructTypePtr> params_types, bool limit2current_scope, bool exact) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordFunction && record->name == name) {
            auto func = std::static_pointer_cast<RecordFunction>(record);
            if (_checkFunctionParameterType(func, params_types, exact)) {
                return func;
            }
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->getFunction(name, params_types) : nullptr;
}

StructTypePtr Enviornment::getStruct(const std::string& name, bool limit2current_scope, std::vector<StructTypePtr> gens) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordStructInst && record->name == name) {
            for (const auto& [gen, expected_gen] : llvm::zip(gens, std::static_pointer_cast<RecordStructType>(record)->generic_sub_types)) {
                if (!_checkType(gen, expected_gen)) {
                    continue;
                }
            }
            return std::static_pointer_cast<RecordStructType>(record);
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->getStruct(name) : nullptr;
}

ModulePtr Enviornment::getModule(const std::string& name, bool limit2current_scope) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordModule && record->name == name) {
            return std::static_pointer_cast<RecordModule>(record);
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->getModule(name) : nullptr;
}

std::vector<GenericFunctionPtr> Enviornment::getGenericFunc(const std::string& name) {
    std::vector<GenericFunctionPtr> matching_gfuncs;
    for (const auto& [Gf_name, Gf_record] : record_map) {
        if (Gf_record->type == RecordType::RecordGenericFunction) {
            auto Gf = std::static_pointer_cast<RecordGenericFunction>(Gf_record);
            if (Gf->name == name) {
                matching_gfuncs.push_back(Gf);
            }
        }
    }
    return this->parent != nullptr && matching_gfuncs.empty() ? this->parent->getGenericFunc(name) : matching_gfuncs;
}

std::vector<GenericStructTypePtr> Enviornment::getGenericStruct(const std::string& name) {
    std::vector<GenericStructTypePtr> matching_gstructs;
    for (const auto& [Gs_name, Gs_record] : record_map) {
        if (Gs_record->type == RecordType::RecordGStructType) {
            auto Gs = std::static_pointer_cast<RecordGenericStructType>(Gs_record);
            if (Gs->name == name) {
                matching_gstructs.push_back(Gs);
            }
        }
    }
    return this->parent != nullptr && matching_gstructs.empty() ? this->parent->getGenericStruct(name) : matching_gstructs;
}

std::vector<VariablePtr> Enviornment::getCurrentVars() {
    std::vector<VariablePtr> vars = {};
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordVariable) {
            vars.push_back(std::static_pointer_cast<RecordVariable>(record));
        }
    }
    return vars;
};
