#include "enviornment.hpp"
#include "../compiler.hpp"
#include <memory>

bool enviornment::_checkType(std::shared_ptr<enviornment::RecordStructType> type1, std::shared_ptr<enviornment::RecordStructType> type2) {
    for (const auto& [field_name1, field_name2] : llvm::zip(type1->fields, type2->fields)) {
        if (field_name1 != field_name2) {
            return false;
        }
        if (!enviornment::_checkType(type1->sub_types.at(field_name1), type2->sub_types.at(field_name2))) {
            return false;
        }
    }
    return type1->stand_alone_type == type2->stand_alone_type;
}

bool _checkFunctionParameterType(std::shared_ptr<enviornment::RecordFunction> func_record, std::vector<std::shared_ptr<enviornment::RecordStructType>> params, bool exact = false) {
    for (const auto& [arg, pass_instance] : llvm::zip(func_record->arguments, params)) {
        const auto& [arg_name, accept_instance] = arg;
        bool types_match = _checkType(accept_instance, pass_instance);
        bool can_convert = !exact && compiler::Compiler::canConvertType(accept_instance, pass_instance);

        if (!(types_match || can_convert)) {
            return false;
        }
    }

    bool correct_arg_count = func_record->varArg || func_record->arguments.size() == params.size();
    return correct_arg_count;
}

bool enviornment::RecordStructType::is_method(const std::string& name, const std::vector<std::shared_ptr<enviornment::RecordStructType>>& params_types,
                                              const std::unordered_map<std::string, std::any>& ex_info, std::shared_ptr<enviornment::RecordStructType> return_type) {
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

        bool return_correct = !return_type || enviornment::_checkType(return_type, method->return_inst);
        bool name_matches = name.empty() || method->name == name;
        bool params_match = _checkFunctionParameterType(method, params_types);

        if (return_correct && match && name_matches && params_match) {
            return true;
        }
    }
    return false;
}


std::shared_ptr<enviornment::RecordFunction> enviornment::RecordStructType::get_method(const std::string& name, const std::vector<std::shared_ptr<enviornment::RecordStructType>>& params_types,
                                                                                       const std::unordered_map<std::string, std::any>& ex_info,
                                                                                       std::shared_ptr<enviornment::RecordStructType> return_type) {
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

        bool return_correct = !return_type || enviornment::_checkType(return_type, method->return_inst);
        bool name_matches = name.empty() || method->name == name;
        bool params_match = _checkFunctionParameterType(method, params_types);

        if (return_correct && match && name_matches && params_match) {
            return method;
        }
    }
    return nullptr;
}

bool enviornment::RecordModule::is_function(const std::string& name, const std::vector<std::shared_ptr<enviornment::RecordStructType>>& params_types, bool exact) {
    for (const auto& [func_name, func_record] : record_map) {
        if (func_record->type == RecordType::RecordFunction) {
            auto func = std::static_pointer_cast<enviornment::RecordFunction>(func_record);
            bool name_matches = (func->name == name);
            bool params_match = _checkFunctionParameterType(func, params_types, exact);

            if (name_matches && params_match) {
                return true;
            }
        }
    }
    return false;
}

bool enviornment::RecordModule::is_struct(const std::string& name, std::vector<std::shared_ptr<RecordStructType>> gens) {
    for (const auto& [struct_name, struct_record] : record_map) {
        if (struct_record->type == RecordType::RecordStructInst) {
            auto struct_type = std::static_pointer_cast<enviornment::RecordStructType>(struct_record);
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

bool enviornment::RecordModule::is_module(const std::string& name) {
    for (const auto& [module_name, module_record] : record_map) {
        if (module_record->type == RecordType::RecordModule) {
            auto module = std::static_pointer_cast<enviornment::RecordModule>(module_record);
            if (module->name == name) {
                return true;
            }
        }
    }
    return false;
}

bool enviornment::RecordModule::is_Gfunc(const std::string& name) {
    for (const auto& [Gfunc_name, Gfunc_record] : record_map) {
        if (Gfunc_record->type == RecordType::RecordGenericFunction) {
            auto Gfunc = std::static_pointer_cast<enviornment::RecordGenericFunction>(Gfunc_record);
            if (Gfunc->name == name) {
                return true;
            }
        }
    }
    return false;
}

bool enviornment::RecordModule::is_Gstruct(const std::string& name) {
    for (const auto& [Gstruct_name, Gstruct_record] : record_map) {
        if (Gstruct_record->type == RecordType::RecordGStructType) {
            auto Gstruct = std::static_pointer_cast<enviornment::RecordGStructType>(Gstruct_record);
            if (Gstruct->name == name) {
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<enviornment::RecordFunction> enviornment::RecordModule::get_function(const std::string& name, const std::vector<std::shared_ptr<enviornment::RecordStructType>>& params_types,
                                                                                     bool exact) {
    for (const auto& [func_name, func_record] : record_map) {
        if (func_record->type == RecordType::RecordFunction) {
            auto func = std::static_pointer_cast<enviornment::RecordFunction>(func_record);
            if (func->name == name && _checkFunctionParameterType(func, params_types, exact)) {
                return func;
            }
        }
    }
    return nullptr;
}

std::shared_ptr<enviornment::RecordStructType> enviornment::RecordModule::get_struct(const std::string& name, std::vector<std::shared_ptr<RecordStructType>> gens) {
    for (const auto& [struct_name, struct_record] : record_map) {
        if (struct_record->type == RecordType::RecordStructInst) {
            auto struct_type = std::static_pointer_cast<enviornment::RecordStructType>(struct_record);
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

std::shared_ptr<enviornment::RecordModule> enviornment::RecordModule::get_module(const std::string& name) {
    for (const auto& [module_name, module_record] : record_map) {
        if (module_record->type == RecordType::RecordModule) {
            auto module = std::static_pointer_cast<enviornment::RecordModule>(module_record);
            if (module->name == name) {
                return module;
            }
        }
    }
    return nullptr;
}

std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> enviornment::RecordModule::get_Gfunc(const std::string& name) {
    std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> matching_gfuncs;
    for (const auto& [Gf_name, Gf_record] : record_map) {
        if (Gf_record->type == RecordType::RecordGenericFunction) {
            auto Gf = std::static_pointer_cast<enviornment::RecordGenericFunction>(Gf_record);
            if (Gf->name == name) {
                matching_gfuncs.push_back(Gf);
            }
        }
    }
    return matching_gfuncs;
}

std::vector<std::shared_ptr<enviornment::RecordGStructType>> enviornment::RecordModule::get_Gstruct(const std::string& name) {
    std::vector<std::shared_ptr<enviornment::RecordGStructType>> matching_gstructs;
    for (const auto& [Gs_name, Gs_record] : record_map) {
        if (Gs_record->type == RecordType::RecordGStructType) {
            auto Gs = std::static_pointer_cast<enviornment::RecordGStructType>(Gs_record);
            if (Gs->name == name) {
                matching_gstructs.push_back(Gs);
            }
        }
    }
    return matching_gstructs;
}

void enviornment::Enviornment::add(std::shared_ptr<Record> record) { record_map.push_back({record->name, record}); }

bool enviornment::Enviornment::is_variable(const std::string& name, bool limit2current_scope) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordVariable && record->name == name) {
            return true;
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->is_variable(name) : false;
}

bool enviornment::Enviornment::is_function(const std::string& name, std::vector<std::shared_ptr<enviornment::RecordStructType>> params_types, bool limit2current_scope, bool exact) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordFunction && record->name == name) {
            auto func = std::static_pointer_cast<enviornment::RecordFunction>(record);
            if (_checkFunctionParameterType(func, params_types, exact)) {
                return true;
            }
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->is_function(name, params_types, exact) : false;
}

bool enviornment::Enviornment::is_struct(const std::string& name, bool limit2current_scope, std::vector<std::shared_ptr<RecordStructType>> gens) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordStructInst && record->name == name) {
            for (const auto& [gen, expected_gen] : llvm::zip(gens, std::static_pointer_cast<enviornment::RecordStructType>(record)->generic_sub_types)) {
                if (!_checkType(gen, expected_gen)) {
                    continue;
                }
            }
            return true;
        }
    }
    return this->parent ? this->parent->is_struct(name) : false;
}

bool enviornment::Enviornment::is_module(const std::string& name, bool limit2current_scope) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordModule && record->name == name) {
            return true;
        }
    }
    return this->parent ? this->parent->is_module(name) : false;
}

bool enviornment::Enviornment::is_Gfunc(const std::string& name) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordGenericFunction && record->name == name) {
            return true;
        }
    }
    return this->parent ? this->parent->is_Gfunc(name) : false;
}

bool enviornment::Enviornment::is_Gstruct(const std::string& name) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordGStructType && record->name == name) {
            return true;
        }
    }
    return this->parent ? this->parent->is_Gstruct(name) : false;
}

std::shared_ptr<enviornment::RecordVariable> enviornment::Enviornment::get_variable(const std::string& name, bool limit2current_scope) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordVariable && record->name == name) {
            return std::static_pointer_cast<enviornment::RecordVariable>(record);
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->get_variable(name) : nullptr;
}

std::shared_ptr<enviornment::RecordFunction> enviornment::Enviornment::get_function(const std::string& name, std::vector<std::shared_ptr<enviornment::RecordStructType>> params_types,
                                                                                    bool limit2current_scope, bool exact) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordFunction && record->name == name) {
            auto func = std::static_pointer_cast<enviornment::RecordFunction>(record);
            if (_checkFunctionParameterType(func, params_types, exact)) {
                return func;
            }
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->get_function(name, params_types) : nullptr;
}

std::shared_ptr<enviornment::RecordStructType> enviornment::Enviornment::get_struct(const std::string& name, bool limit2current_scope, std::vector<std::shared_ptr<RecordStructType>> gens) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordStructInst && record->name == name) {
            for (const auto& [gen, expected_gen] : llvm::zip(gens, std::static_pointer_cast<enviornment::RecordStructType>(record)->generic_sub_types)) {
                if (!_checkType(gen, expected_gen)) {
                    continue;
                }
            }
            return std::static_pointer_cast<enviornment::RecordStructType>(record);
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->get_struct(name) : nullptr;
}

std::shared_ptr<enviornment::RecordModule> enviornment::Enviornment::get_module(const std::string& name, bool limit2current_scope) {
    for (const auto& [record_name, record] : record_map) {
        if (record->type == RecordType::RecordModule && record->name == name) {
            return std::static_pointer_cast<enviornment::RecordModule>(record);
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->get_module(name) : nullptr;
}

std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> enviornment::Enviornment::get_Gfunc(const std::string& name) {
    std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> matching_gfuncs;
    for (const auto& [Gf_name, Gf_record] : record_map) {
        if (Gf_record->type == RecordType::RecordGenericFunction) {
            auto Gf = std::static_pointer_cast<enviornment::RecordGenericFunction>(Gf_record);
            if (Gf->name == name) {
                matching_gfuncs.push_back(Gf);
            }
        }
    }
    return this->parent != nullptr && matching_gfuncs.empty() ? this->parent->get_Gfunc(name) : matching_gfuncs;
}

std::vector<std::shared_ptr<enviornment::RecordGStructType>> enviornment::Enviornment::get_Gstruct(const std::string& name) {
    std::vector<std::shared_ptr<enviornment::RecordGStructType>> matching_gstructs;
    for (const auto& [Gs_name, Gs_record] : record_map) {
        if (Gs_record->type == RecordType::RecordGStructType) {
            auto Gs = std::static_pointer_cast<enviornment::RecordGStructType>(Gs_record);
            if (Gs->name == name) {
                matching_gstructs.push_back(Gs);
            }
        }
    }
    return this->parent != nullptr && matching_gstructs.empty() ? this->parent->get_Gstruct(name) : matching_gstructs;
}
