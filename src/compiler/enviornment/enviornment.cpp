#include "enviornment.hpp"
#include <memory>


bool enviornment::_checkType(std::shared_ptr<enviornment::RecordStructInstance> type1, std::shared_ptr<enviornment::RecordStructInstance> type2) {
    if(type1->unknown || type2->unknown)
        return true;
    for(auto [gen_type1, gen_type2] : llvm::zip(type1->generic, type2->generic)) {
        if(!enviornment::_checkType(gen_type1, gen_type2)) {
            return false;
        }
    }
    for(auto [field_name1, field_name2] : llvm::zip(type1->struct_type->fields, type2->struct_type->fields)) {
        if(field_name1 != field_name2) {
            return false;
        }
        if(!enviornment::_checkType(type1->struct_type->sub_types[field_name1], type2->struct_type->sub_types[field_name2])) {
            return false;
        }
    }
    return type1->struct_type->stand_alone_type == type2->struct_type->stand_alone_type;
};

bool enviornment::_checkType(std::shared_ptr<enviornment::RecordStructInstance> type1, std::shared_ptr<enviornment::RecordStructType> type2) {
    if(type1->unknown)
        return true;
    for(auto [field_name1, field_name2] : llvm::zip(type1->struct_type->fields, type2->fields)) {
        if(field_name1 != field_name2) {
            return false;
        }
        if(!enviornment::_checkType(type1->struct_type->sub_types[field_name1], type2->sub_types[field_name2])) {
            return false;
        }
    }
    return type1->struct_type->stand_alone_type == type2->stand_alone_type;
};

bool enviornment::_checkType(std::shared_ptr<enviornment::RecordStructType> type1, std::shared_ptr<enviornment::RecordStructType> type2) {
    for(auto [field_name1, field_name2] : llvm::zip(type1->fields, type2->fields)) {
        if(field_name1 != field_name2) {
            return false;
        }
        if(!enviornment::_checkType(type1->sub_types[field_name1], type2->sub_types[field_name2])) {
            return false;
        }
    }
    return type1->stand_alone_type == type2->stand_alone_type;
};

bool _checkFunctionParameterType(std::shared_ptr<enviornment::RecordFunction> func_record, std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params) {
    for(auto [arg, pass_instanc] : llvm::zip(func_record->arguments, params)) {
        auto [arg_name, accept_instanc] = arg;
        if(!_checkType(accept_instanc, pass_instanc)) {
            return false;
        }
    }

    bool result = func_record->varArg || func_record->arguments.size() == params.size();
    return result;
};

bool enviornment::RecordStructType::is_method(const std::string& name, const std::vector<std::shared_ptr<enviornment::RecordStructInstance>>& params_types) {
    for(auto [method_name, method] : this->methods) {
        if(method->name == name && _checkFunctionParameterType(method, params_types)) {
            return true;
        }
    }
    return false;
};

std::shared_ptr<enviornment::RecordFunction> enviornment::RecordStructType::get_method(const std::string& name, const std::vector<std::shared_ptr<enviornment::RecordStructInstance>>& params_types) {
    for(auto [method_name, method] : this->methods) {
        std::cout << "Name: " << name + " : " + method->name << std::endl;
        if(method->name == name && _checkFunctionParameterType(method, params_types)) {
            return method;
        }
    }
    return nullptr;
};

bool enviornment::RecordModule::is_function(const std::string& name, const std::vector<std::shared_ptr<enviornment::RecordStructInstance>>& params_types) {
    for(auto [func_name, func_record] : record_map) {
        if(func_record->type == RecordType::RecordFunction) {
            auto func = std::static_pointer_cast<enviornment::RecordFunction>(func_record);
            if(func->name == name && _checkFunctionParameterType(func, params_types)) {
                return true;
            }
        }
    }
    return false;
}

bool enviornment::RecordModule::is_struct(const std::string& name) {
    for(auto [struct_name, struct_record] : record_map) {
        if(struct_record->type == RecordType::RecordStructInst) {
            auto struct_type = std::static_pointer_cast<enviornment::RecordStructType>(struct_record);
            if(struct_type->name == name) {
                return true;
            }
        }
    }
    return false;
}

bool enviornment::RecordModule::is_module(const std::string& name) {
    for(auto [module_name, module_record] : record_map) {
        if(module_record->type == RecordType::RecordModule) {
            auto module = std::static_pointer_cast<enviornment::RecordModule>(module_record);
            if(module->name == name) {
                return true;
            }
        }
    }
    return false;
}

bool enviornment::RecordModule::is_Gfunc(const std::string& name) {
    for(auto [module_name, module_record] : record_map) {
        if(module_record->type == RecordType::RecordGenericFunction) {
            auto module = std::static_pointer_cast<enviornment::RecordGenericFunction>(module_record);
            if(module->name == name) {
                return true;
            }
        }
    }
    return false;
}

std::shared_ptr<enviornment::RecordFunction> enviornment::RecordModule::get_function(const std::string& name, const std::vector<std::shared_ptr<enviornment::RecordStructInstance>>& params_types) {
    for(auto [func_name, func_record] : record_map) {
        if(func_record->type == RecordType::RecordFunction) {
            auto func = std::static_pointer_cast<enviornment::RecordFunction>(func_record);
            if(func->name == name && _checkFunctionParameterType(func, params_types)) {
                return func;
            }
        }
    }
    return nullptr;
};

std::shared_ptr<enviornment::RecordStructType> enviornment::RecordModule::get_struct(const std::string& name) {
    for(auto [struct_name, struct_record] : record_map) {
        if(struct_record->type == RecordType::RecordStructInst) {
            auto struct_type = std::static_pointer_cast<enviornment::RecordStructType>(struct_record);
            if(struct_type->name == name) {
                return struct_type;
            }
        }
    }
    return nullptr;
};

std::shared_ptr<enviornment::RecordModule> enviornment::RecordModule::get_module(const std::string& name) {
    for(auto [module_name, module_record] : record_map) {
        if(module_record->type == RecordType::RecordModule) {
            auto module = std::static_pointer_cast<enviornment::RecordModule>(module_record);
            if(module->name == name) {
                return module;
            }
        }
    }
    return nullptr;
};

std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> enviornment::RecordModule::get_Gfunc(const std::string& name) {
    std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> matching_gfuncs;
    for(auto [Gf_name, Gf_record] : record_map) {
        if(Gf_record->type == RecordType::RecordGenericFunction) {
            auto Gf = std::static_pointer_cast<enviornment::RecordGenericFunction>(Gf_record);
            if(Gf->name == name) {
                matching_gfuncs.push_back(Gf);
            }
        }
    }
    return matching_gfuncs;
};

void enviornment::Enviornment::add(std::shared_ptr<Record> record) { record_map.push_back({record->name, record}); }

bool enviornment::Enviornment::is_variable(const std::string& name, bool limit2current_scope) {
    for(auto [record_name, record] : record_map) {
        if(record->type == RecordType::RecordVariable && record->name == name) {
            return true;
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->is_variable(name) : false;
};

bool enviornment::Enviornment::is_function(const std::string& name, std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_types, bool limit2current_scope) {
    for(auto [record_name, record] : record_map) {
        if(record->type == RecordType::RecordFunction && record->name == name) {
            auto func = std::static_pointer_cast<enviornment::RecordFunction>(record);
            if(_checkFunctionParameterType(func, params_types)) {
                return true;
            }
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->is_function(name, params_types) : false;
};

bool enviornment::Enviornment::is_struct(const std::string& name, bool limit2current_scope) {
    for(auto [record_name, record] : this->record_map) {
        if(record->type == RecordType::RecordStructInst && record->name == name) {
            return true;
        }
    }
    return this->parent ? this->parent->is_struct(name) : false;
};

bool enviornment::Enviornment::is_module(const std::string& name, bool limit2current_scope) {
    for(auto [record_name, record] : record_map) {
        if(record->type == RecordType::RecordModule && record->name == name) {
            return true;
        }
    }
    return this->parent ? this->parent->is_module(name) : false;
};

bool enviornment::Enviornment::is_Gfunc(const std::string& name) {
    for(auto [record_name, record] : record_map) {
        if(record->type == RecordType::RecordGenericFunction && record->name == name) {
            return true;
        }
    }
    return this->parent ? this->parent->is_Gfunc(name) : false;
};

std::shared_ptr<enviornment::RecordVariable> enviornment::Enviornment::get_variable(const std::string& name, bool limit2current_scope) {
    for(auto [record_name, record] : record_map) {
        if(record->type == RecordType::RecordVariable && record->name == name) {
            return std::static_pointer_cast<enviornment::RecordVariable>(record);
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->get_variable(name) : nullptr;
};

std::shared_ptr<enviornment::RecordFunction> enviornment::Enviornment::get_function(const std::string& name, std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_types,
                                                                                    bool limit2current_scope) {
    for(auto [record_name, record] : record_map) {
        if(record->type == RecordType::RecordFunction && record->name == name) {
            auto func = std::static_pointer_cast<enviornment::RecordFunction>(record);
            if(_checkFunctionParameterType(func, params_types)) {
                return func;
            }
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->get_function(name, params_types) : nullptr;
};

std::shared_ptr<enviornment::RecordStructType> enviornment::Enviornment::get_struct(const std::string& name, bool limit2current_scope) {
    for(auto [record_name, record] : record_map) {
        if(record->type == RecordType::RecordStructInst && record->name == name) {
            return std::static_pointer_cast<enviornment::RecordStructType>(record);
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->get_struct(name) : nullptr;
};

std::shared_ptr<enviornment::RecordModule> enviornment::Enviornment::get_module(const std::string& name, bool limit2current_scope) {
    for(auto [record_name, record] : record_map) {
        if(record->type == RecordType::RecordModule && record->name == name) {
            return std::static_pointer_cast<enviornment::RecordModule>(record);
        }
    }
    return (parent != nullptr && !limit2current_scope) ? parent->get_module(name) : nullptr;
};

std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> enviornment::Enviornment::get_Gfunc(const std::string& name) {
    std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> matching_gfuncs;
    for(auto [Gf_name, Gf_record] : record_map) {
        if(Gf_record->type == RecordType::RecordGenericFunction) {
            auto Gf = std::static_pointer_cast<enviornment::RecordGenericFunction>(Gf_record);
            if(Gf->name == name) {
                matching_gfuncs.push_back(Gf);
            }
        }
    }
    return this->parent != nullptr && matching_gfuncs.size() == 0 ? this->parent->get_Gfunc(name) : matching_gfuncs;
};
