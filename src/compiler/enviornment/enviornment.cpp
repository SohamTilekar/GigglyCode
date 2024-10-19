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
bool enviornment::Enviornment::is_struct(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        return record_map[name]->type == RecordType::RecordStructType;
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->is_struct(name);
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

std::shared_ptr<enviornment::RecordStructType> enviornment::Enviornment::get_class(std::string name, bool limit2current_scope) {
    if(record_map.find(name) != record_map.end()) {
        if(record_map[name]->type == RecordType::RecordStructType) {
            return std::static_pointer_cast<enviornment::RecordStructType>(record_map[name]);
        }
    } else if(parent != nullptr & !limit2current_scope) {
        return parent->get_class(name);
    }
    return nullptr;
};