#include "enviornment.hpp"

void enviornment::Enviornment::add(std::shared_ptr<Record> record) { record_map[record->name] = record; }

std::shared_ptr<enviornment::Record> enviornment::Enviornment::get(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        return record_map[name];
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->get(name);
    } else {
        return nullptr;
    }
}

bool enviornment::Enviornment::contains(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        return true;
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->contains(name);
    } else {
        return false;
    }
};

bool enviornment::Enviornment::is_builtin_type(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        return record_map[name]->type == RecordType::BuiltinType;
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->is_builtin_type(name);
    } else {
        return false;
    }
};

llvm::Type* enviornment::Enviornment::get_builtin_type(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        if(record_map[name]->type == RecordType::BuiltinType) {
            return std::static_pointer_cast<enviornment::RecordBuiltinType>(record_map[name])->type;
        }
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->get_builtin_type(name);
    }
    return nullptr;
};

bool enviornment::Enviornment::is_variable(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        return record_map[name]->type == RecordType::RecordVariable;
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->is_variable(name);
    } else {
        return false;
    }
};
bool enviornment::Enviornment::is_function(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        return record_map[name]->type == RecordType::RecordFunction;
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->is_function(name);
    } else {
        return false;
    }
};
bool enviornment::Enviornment::is_class(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        return record_map[name]->type == RecordType::RecordClassType;
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->is_class(name);
    } else {
        return false;
    }
};
bool enviornment::Enviornment::is_enum(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        return record_map[name]->type == RecordType::RecordEnumType;
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->is_enum(name);
    } else {
        return false;
    }
};

std::shared_ptr<enviornment::RecordVariable> enviornment::Enviornment::get_variable(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        if(record_map[name]->type == RecordType::RecordVariable) {
            return std::static_pointer_cast<enviornment::RecordVariable>(record_map[name]);
        }
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->get_variable(name);
    }
    return nullptr;
};

std::shared_ptr<enviornment::RecordFunction> enviornment::Enviornment::get_function(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        if(record_map[name]->type == RecordType::RecordFunction) {
            return std::static_pointer_cast<enviornment::RecordFunction>(record_map[name]);
        }
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->get_function(name);
    }
    return nullptr;
};

std::shared_ptr<enviornment::RecordClassType> enviornment::Enviornment::get_class(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        if(record_map[name]->type == RecordType::RecordClassType) {
            return std::static_pointer_cast<enviornment::RecordClassType>(record_map[name]);
        }
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->get_class(name);
    }
    return nullptr;
};

std::shared_ptr<enviornment::RecordEnumType> enviornment::Enviornment::get_enum(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        if(record_map[name]->type == RecordType::RecordEnumType) {
            return std::static_pointer_cast<enviornment::RecordEnumType>(record_map[name]);
        }
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->get_enum(name);
    }
    return nullptr;
};