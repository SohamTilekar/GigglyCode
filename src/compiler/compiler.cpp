#include "compiler.hpp"
#include <algorithm>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/ADT/APInt.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <regex.h>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "../errors/errors.hpp"
#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"

compiler::Compiler::Compiler(const std::string& source, std::filesystem::path file_path, std::filesystem::path ir_gc_map, std::filesystem::path buildDir, std::filesystem::path relativePath)
    : llvm_context(llvm::LLVMContext()), llvm_ir_builder(llvm_context), source(source), file_path(file_path), ir_gc_map(ir_gc_map), buildDir(buildDir), relativePath(relativePath) {
    std::string path_str = file_path.string();
    size_t pos = path_str.rfind("src");
    if(pos != std::string::npos) {
        pos += 4; // Move past the "src"
    } else {
        pos = 0; // Default to the start if "src" is not found
    }
    this->fc_st_name_prefix = path_str.substr(pos);
    pos = 0;
    while((pos = this->fc_st_name_prefix.find('/', pos)) != std::string::npos) {
        this->fc_st_name_prefix.replace(pos, 1, "..");
        pos += 2;
    }
    pos = 0;
    while((pos = this->fc_st_name_prefix.find('\\', pos)) != std::string::npos) {
        this->fc_st_name_prefix.replace(pos, 1, "..");
        pos += 2;
    }
    this->llvm_module = std::make_unique<llvm::Module>(this->fc_st_name_prefix, llvm_context);
    this->fc_st_name_prefix += "..";
    this->llvm_module->setSourceFileName(file_path.string());
    this->enviornment = std::make_shared<enviornment::Enviornment>(nullptr, std::vector<std::tuple<std::string, std::shared_ptr<enviornment::Record>>>());
    this->enviornment->parent = std::make_shared<enviornment::Enviornment>(nullptr, std::vector<std::tuple<std::string, std::shared_ptr<enviornment::Record>>>(), "buildtins");
    this->_initializeBuiltins();
    // Open the ir_gc_map JSON file and attach it to this->ir_gc_map_json
    std::ifstream ir_gc_map_file(ir_gc_map.string());
    if(!ir_gc_map_file.is_open()) {
        throw std::runtime_error("Failed to open ir_gc_map file: " + ir_gc_map.string());
    }
    ir_gc_map_file >> this->ir_gc_map_json;
    this->ir_gc_map_json["functions"] = nlohmann::json::object();
    this->ir_gc_map_json["structs"] = nlohmann::json::object();
    ir_gc_map_file.close();
}
void compiler::Compiler::_initializeBuiltins() {
    auto _int = std::make_shared<enviornment::RecordStructType>("int", llvm::Type::getInt64Ty(llvm_context));
    this->enviornment->parent->add(_int);
    auto _int32 = std::make_shared<enviornment::RecordStructType>("int32", llvm::Type::getInt32Ty(llvm_context));
    this->enviornment->parent->add(_int32);
    auto _int128 = std::make_shared<enviornment::RecordStructType>("int128", llvm::Type::getInt64Ty(llvm_context));
    this->enviornment->parent->add(_int128);
    auto _uint = std::make_shared<enviornment::RecordStructType>("uint", llvm::Type::getInt64Ty(llvm_context));
    this->enviornment->parent->add(_uint);
    auto _uint32 = std::make_shared<enviornment::RecordStructType>("uint32", llvm::Type::getInt32Ty(llvm_context));
    this->enviornment->parent->add(_uint32);
    auto _uint128 = std::make_shared<enviornment::RecordStructType>("uint128", llvm::Type::getInt64Ty(llvm_context));
    this->enviornment->parent->add(_uint128);
    auto _float = std::make_shared<enviornment::RecordStructType>("float", llvm::Type::getDoubleTy(llvm_context));
    this->enviornment->parent->add(_float);
    auto _float32 = std::make_shared<enviornment::RecordStructType>("float32", llvm::Type::getFloatTy(llvm_context));
    this->enviornment->parent->add(_float32);
    auto _char = std::make_shared<enviornment::RecordStructType>("char", llvm::Type::getInt8Ty(llvm_context));
    this->enviornment->parent->add(_char);
    auto _string = std::make_shared<enviornment::RecordStructType>("str", llvm::PointerType::get(llvm::Type::getInt8Ty(llvm_context), 0));
    this->enviornment->parent->add(_string);
    auto _void = std::make_shared<enviornment::RecordStructType>("void", llvm::Type::getVoidTy(llvm_context));
    this->enviornment->parent->add(_void);
    auto _bool = std::make_shared<enviornment::RecordStructType>("bool", llvm::Type::getInt1Ty(llvm_context));
    this->enviornment->parent->add(_bool);
    // array standalone type
    auto _array = std::make_shared<enviornment::RecordStructType>("array", llvm::PointerType::get(llvm::Type::getVoidTy(llvm_context), 0));
    this->enviornment->parent->add(_array);
    auto _any = std::make_shared<enviornment::RecordModule>("Any");
    this->enviornment->parent->add(_any);

    // Create the global variable 'true'
    llvm::GlobalVariable* globalTrue = new llvm::GlobalVariable(*this->llvm_module, this->enviornment->parent->get_struct("bool")->stand_alone_type, true, llvm::GlobalValue::InternalLinkage,
                                                                llvm::ConstantInt::get(this->enviornment->parent->get_struct("bool")->stand_alone_type, 1), "True");

    // Create the global variable 'false'
    llvm::GlobalVariable* globalFalse = new llvm::GlobalVariable(*this->llvm_module, this->enviornment->parent->get_struct("bool")->stand_alone_type, true, llvm::GlobalValue::InternalLinkage,
                                                                 llvm::ConstantInt::get(this->enviornment->parent->get_struct("bool")->stand_alone_type, 0), "False");
    auto recordTrue = std::make_shared<enviornment::RecordVariable>("True", globalTrue, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->parent->get_struct("bool")));
    this->enviornment->parent->add(recordTrue);
    auto recordFalse = std::make_shared<enviornment::RecordVariable>("False", globalFalse, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->parent->get_struct("bool")));
    this->enviornment->parent->add(recordFalse);

    // Create the function type: void puts(const char*)
    llvm::FunctionType* putsType = llvm::FunctionType::get(_void->stand_alone_type, _string->stand_alone_type, false);
    auto puts = llvm::Function::Create(putsType, llvm::Function::ExternalLinkage, "puts", this->llvm_module.get());
    std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructInstance>>> putsParams = {{"string", std::make_shared<enviornment::RecordStructInstance>(_string)}};
    this->enviornment->parent->add(std::make_shared<enviornment::RecordFunction>("puts", puts, putsType, putsParams, std::make_shared<enviornment::RecordStructInstance>(_void), true));
    // Create the function type: int printf(const char*, ...)
    llvm::FunctionType* printfType = llvm::FunctionType::get(_int->stand_alone_type, {_string->stand_alone_type}, true);
    auto printfFunc = llvm::Function::Create(printfType, llvm::Function::ExternalLinkage, "printf", this->llvm_module.get());
    std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructInstance>>> printfParams = {{"format", std::make_shared<enviornment::RecordStructInstance>(_string)}};
    this->enviornment->parent->add(std::make_shared<enviornment::RecordFunction>("printf", printfFunc, printfType, printfParams, std::make_shared<enviornment::RecordStructInstance>(_int), true));

    // Create the function type: int scanf(const char*, ...)
    llvm::FunctionType* scanfType = llvm::FunctionType::get(_int->stand_alone_type, {_string->stand_alone_type}, true);
    auto scanfFunc = llvm::Function::Create(scanfType, llvm::Function::ExternalLinkage, "scanf", this->llvm_module.get());
    std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructInstance>>> scanfParams = {{"format", std::make_shared<enviornment::RecordStructInstance>(_string)}};
    this->enviornment->parent->add(std::make_shared<enviornment::RecordFunction>("scanf", scanfFunc, scanfType, scanfParams, std::make_shared<enviornment::RecordStructInstance>(_int), true));
}

void compiler::Compiler::compile(std::shared_ptr<AST::Node> node) {
    switch(node->type()) {
    case AST::NodeType::Program:
        this->_visitProgram(std::static_pointer_cast<AST::Program>(node));
        break;
    case AST::NodeType::ExpressionStatement: {
        this->_visitExpressionStatement(std::static_pointer_cast<AST::ExpressionStatement>(node));
        break;
    }
    case AST::NodeType::InfixedExpression: {
        this->_visitInfixExpression(std::static_pointer_cast<AST::InfixExpression>(node));
        break;
    }
    case AST::NodeType::IndexExpression: {
        this->_visitIndexExpression(std::static_pointer_cast<AST::IndexExpression>(node));
        break;
    }
    case AST::NodeType::VariableDeclarationStatement: {
        this->_visitVariableDeclarationStatement(std::static_pointer_cast<AST::VariableDeclarationStatement>(node));
        break;
    }
    case AST::NodeType::VariableAssignmentStatement: {
        this->_visitVariableAssignmentStatement(std::static_pointer_cast<AST::VariableAssignmentStatement>(node));
        break;
    }
    case AST::NodeType::IfElseStatement: {
        this->_visitIfElseStatement(std::static_pointer_cast<AST::IfElseStatement>(node));
        break;
    }
    case AST::NodeType::FunctionStatement: {
        this->_visitFunctionDeclarationStatement(std::static_pointer_cast<AST::FunctionStatement>(node));
        break;
    }
    case AST::NodeType::CallExpression: {
        this->_visitCallExpression(std::static_pointer_cast<AST::CallExpression>(node));
        break;
    }
    case AST::NodeType::ReturnStatement: {
        this->_visitReturnStatement(std::static_pointer_cast<AST::ReturnStatement>(node));
        break;
    }
    case AST::NodeType::BlockStatement: {
        this->_visitBlockStatement(std::static_pointer_cast<AST::BlockStatement>(node));
        break;
    }
    case AST::NodeType::WhileStatement: {
        this->_visitWhileStatement(std::static_pointer_cast<AST::WhileStatement>(node));
        break;
    }
    case AST::NodeType::BreakStatement: {
        if(this->enviornment->loop_end_block.empty()) {
            std::cerr << "Break statement outside loop" << std::endl;
            exit(1);
        }
        auto f_node = std::static_pointer_cast<AST::BreakStatement>(node);
        auto breakInst = this->llvm_ir_builder.CreateBr(this->enviornment->loop_end_block.at(this->enviornment->loop_end_block.size() - f_node->loopIdx - 1));
        break;
    }
    case AST::NodeType::ContinueStatement: {
        if(this->enviornment->loop_condition_block.empty()) {
            std::cerr << "Continue statement outside loop" << std::endl;
            exit(1);
        }
        auto f_node = std::static_pointer_cast<AST::ContinueStatement>(node);
        auto continueInst = this->llvm_ir_builder.CreateBr(this->enviornment->loop_condition_block.at(this->enviornment->loop_condition_block.size() - f_node->loopIdx - 1));
        break;
    }
    case AST::NodeType::StructStatement: {
        this->_visitStructStatement(std::static_pointer_cast<AST::StructStatement>(node));
        break;
    }
    case AST::NodeType::ImportStatement: {
        this->_visitImportStatement(std::static_pointer_cast<AST::ImportStatement>(node));
        break;
    }
    default:
        errors::CompletionError("Unknown node type", this->source, node->meta_data.st_line_no, node->meta_data.end_line_no, "Unknown node type: " + *AST::nodeTypeToString(node->type())).raise();
        break;
    }
};

void compiler::Compiler::_visitProgram(std::shared_ptr<AST::Program> program) {
    for(auto stmt : program->statements) {
        this->compile(stmt);
    }
};

void compiler::Compiler::_visitExpressionStatement(std::shared_ptr<AST::ExpressionStatement> expression_statement) { this->compile(expression_statement->expr); };

void compiler::Compiler::_visitBlockStatement(std::shared_ptr<AST::BlockStatement> block_statement) {
    for(auto stmt : block_statement->statements) {
        this->compile(stmt);
    }
};


std::tuple<llvm::Value*, llvm::Value*,
           std::variant<std::shared_ptr<enviornment::RecordStructInstance>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, compiler::resolveType>
compiler::Compiler::_CallGfunc(std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> gfuncs, std::string name, std::vector<llvm::Value*> args,
                               std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_types) {
    for(auto gfunc : gfuncs) {
        if(gfunc->env->is_function(name, params_types)) {
            auto func = gfunc->env->get_function(name, params_types);
            auto returnValue = this->llvm_ir_builder.CreateCall(func->function, args);
            return {returnValue, nullptr, func->return_inst, compiler::resolveType::StructInst};
        }
    }
    auto prev_env = this->enviornment;
    for(auto gfunc : gfuncs) {
        this->enviornment = std::make_shared<enviornment::Enviornment>(prev_env, std::vector<std::tuple<std::string, std::shared_ptr<enviornment::Record>>>{}, name);
        bool x = false;
        for(auto [gparam, pparam] : llvm::zip(gfunc->func->parameters, params_types)) {
            if(gparam->value_type->name->type() == AST::NodeType::IdentifierLiteral) {
                auto struc = std::make_shared<enviornment::RecordStructType>(*pparam->struct_type);
                struc->name = std::static_pointer_cast<AST::IdentifierLiteral>(gparam->value_type->name)->value;
                std::cout << "Type Name: " << struc->name << std::endl;
                this->enviornment->add(struc);
            } else if(std::get<3>(this->_resolveValue(gparam->value_type->name)) == compiler::resolveType::StructType) {
            } else {
                x = true;
            }
        }
        if(x)
            continue;
        auto body = gfunc->func->body;
        auto params = gfunc->func->parameters;
        std::vector<std::string> param_name;
        std::vector<llvm::Type*> param_types;
        std::vector<std::shared_ptr<enviornment::RecordStructInstance>> param_inst_record;
        for(auto [param, param_type] : llvm::zip(params, params_types)) {
            auto param_name_str = std::static_pointer_cast<AST::IdentifierLiteral>(param->name)->value;
            param_name.push_back(param_name_str);
            param_inst_record.push_back(param_type);
            param_types.push_back(param_type->struct_type->stand_alone_type ? param_type->struct_type->stand_alone_type : llvm::PointerType::get(param_type->struct_type->struct_type, 0));
        }
        auto return_type = this->_parseType(gfunc->func->return_type);
        auto llvm_return_type = return_type->struct_type->stand_alone_type ? return_type->struct_type->stand_alone_type : return_type->struct_type->struct_type->getPointerTo();
        auto func_type = llvm::FunctionType::get(llvm_return_type, param_types, false);
        auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, this->fc_st_name_prefix != "main.gc.." ? this->fc_st_name_prefix + name : name, this->llvm_module.get());
        this->ir_gc_map_json["functions"][name] = func->getName().str();
        unsigned idx = 0;
        for(auto& arg : func->args()) {
            arg.setName(param_name[idx++]);
        }
        std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructInstance>>> arguments;
        auto func_record = std::make_shared<enviornment::RecordFunction>(name, func, func_type, arguments, return_type);
        if(body) {
            auto bb = llvm::BasicBlock::Create(this->llvm_context, "entry", func);
            this->function_entery_block.push_back(bb);
            this->llvm_ir_builder.SetInsertPoint(bb);
            this->enviornment->current_function = func_record;
            for(const auto& [arg, param_type_record] : llvm::zip(func->args(), param_inst_record)) {
                llvm::AllocaInst* alloca = nullptr;
                if(!arg.getType()->isPointerTy() || enviornment::_checkType(param_type_record, this->enviornment->get_struct("array"))) {
                    alloca = this->llvm_ir_builder.CreateAlloca(arg.getType(), nullptr, arg.getName());
                    auto storeInst = this->llvm_ir_builder.CreateStore(&arg, alloca);
                } else {
                    alloca = this->llvm_ir_builder.CreateAlloca(param_type_record->struct_type->struct_type, nullptr, arg.getName());
                    auto loaded_arg = this->llvm_ir_builder.CreateLoad(param_type_record->struct_type->struct_type, &arg, arg.getName() + ".load");
                    auto storeInst = this->llvm_ir_builder.CreateStore(loaded_arg, alloca);
                }
                auto record = std::make_shared<enviornment::RecordVariable>(std::string(arg.getName()), &arg, alloca, param_type_record);
                func_record->arguments.push_back({arg.getName().str(), param_type_record});
                this->enviornment->add(record);
            }
            func_record->set_meta_data(gfunc->func->meta_data.st_line_no, gfunc->func->meta_data.st_col_no, gfunc->func->meta_data.end_line_no, gfunc->func->meta_data.end_col_no);
            this->enviornment->add(func_record);
            try {
                this->compile(body);
            } catch(compiler::DoneRet) {
                // Ignoringrecord
            }
            gfunc->env->add(func_record);
            this->function_entery_block.pop_back();
            if(!this->function_entery_block.empty()) {
                this->llvm_ir_builder.SetInsertPoint(this->function_entery_block.at(this->function_entery_block.size() - 1));
            }
        } else {
            for(const auto& [arg, param_type_record] : llvm::zip(func->args(), param_inst_record)) {
                func_record->arguments.push_back({arg.getName().str(), param_type_record});
            };
            gfunc->env->add(func_record);
        }
        auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, args);
        this->enviornment = prev_env;
        return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
    }
    std::cerr << "No Mach Viable Overload can be created" << std::endl;
    exit(1);
};



std::tuple<llvm::Value*, llvm::Value*,
           std::variant<std::shared_ptr<enviornment::RecordStructInstance>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, compiler::resolveType>
compiler::Compiler::_visitInfixExpression(std::shared_ptr<AST::InfixExpression> infixed_expression) {
    auto op = infixed_expression->op;
    auto left = infixed_expression->left;
    auto right = infixed_expression->right;
    auto [left_value, left_alloca, _left_type, ltt] = this->_resolveValue(left);
    if(op == token::TokenType::Dot) {
        if(right->type() == AST::NodeType::IdentifierLiteral) {
            if(ltt == compiler::resolveType::Module) {
                auto module = std::get<std::shared_ptr<enviornment::RecordModule>>(_left_type);
                auto name = std::static_pointer_cast<AST::IdentifierLiteral>(right)->value;
                if(module->is_module(name)) {
                    return std::make_tuple(nullptr, nullptr, module->get_module(name), compiler::resolveType::Module);
                } else if(module->is_struct(name)) {
                    return std::make_tuple(nullptr, nullptr, module->get_struct(name), compiler::resolveType::StructType);
                } else {
                    std::cerr << "no member `" << name << "` not found in module " << module->name << std::endl;
                    exit(1);
                }
            } else if(ltt == compiler::resolveType::StructInst) {
                auto left_type = std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_left_type);
                if(left_type->struct_type->sub_types.contains(std::static_pointer_cast<AST::IdentifierLiteral>(right)->value)) {
                    unsigned int idx = 0;
                    for(auto field : left_type->struct_type->fields) {
                        if(field == std::static_pointer_cast<AST::IdentifierLiteral>(right)->value) {
                            break;
                        }
                        idx++;
                    }
                    auto type = left_type->struct_type->sub_types[std::static_pointer_cast<AST::IdentifierLiteral>(right)->value];
                    llvm::Value* gep = this->llvm_ir_builder.CreateStructGEP(left_type->struct_type->struct_type, left_alloca, idx);
                    if (left_type->struct_type->struct_type) {
                        return {gep, gep, type, compiler::resolveType::StructInst};
                    }
                    llvm::Value* loaded = this->llvm_ir_builder.CreateLoad(type->struct_type->stand_alone_type, gep); // TODO: Fix
                    return {loaded, gep, type, compiler::resolveType::StructInst};
                } else {
                    std::cerr << "Struct does not have member " + std::static_pointer_cast<AST::IdentifierLiteral>(right)->value << std::endl;
                    exit(1);
                }
            }
        } else if(right->type() == AST::NodeType::CallExpression) {
            auto call_expression = std::static_pointer_cast<AST::CallExpression>(right);
            auto name = std::static_pointer_cast<AST::IdentifierLiteral>(call_expression->name)->value;
            auto param = call_expression->arguments;
            std::vector<llvm::Value*> args;
            std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_types;
            for(auto arg : param) {
                auto [value, val_alloca, param_type, ptt] = this->_resolveValue(arg);
                if(ptt == compiler::resolveType::Module) {
                    std::cerr << "Cannot pass module to function" << std::endl;
                    exit(1);
                } else if(ptt == compiler::resolveType::StructType) {
                    std::cerr << "Cannot pass struct type to function" << std::endl;
                    exit(1);
                }
                params_types.push_back(std::get<std::shared_ptr<enviornment::RecordStructInstance>>(param_type));
                args.push_back(value);
            }
            if(ltt == compiler::resolveType::Module) {
                auto left_type = std::get<std::shared_ptr<enviornment::RecordModule>>(_left_type);
                if(left_type->is_function(name, params_types)) {
                    auto func = left_type->get_function(name, params_types);
                    auto returnValue = this->llvm_ir_builder.CreateCall(func->function, args);
                    return {returnValue, nullptr, func->return_inst, compiler::resolveType::StructInst};
                } else if(left_type->is_Gfunc(name)) {
                    auto gfuncs = left_type->get_Gfunc(name);
                    return this->_CallGfunc(gfuncs, name, args, params_types);
                } else if(left_type->is_struct(name)) {
                    auto struct_record = left_type->get_struct(name);
                    auto struct_type = struct_record->struct_type;
                    auto alloca = this->llvm_ir_builder.CreateAlloca(struct_type, nullptr, name);
                    if(struct_record->is_method(name, params_types)) {
                        auto method = struct_record->get_method(std::static_pointer_cast<AST::IdentifierLiteral>(right)->value, params_types);
                        this->llvm_ir_builder.CreateCall(method->function, args);
                    } else if(struct_record->fields.size() != params_types.size()) {
                        std::cerr << "Struct Type MissMatch" << std::endl;
                        exit(1);
                    }
                    for(unsigned int i = 0; i < args.size(); ++i) {
                        if(!enviornment::_checkType(struct_record->sub_types[struct_record->fields[i]], params_types[i])) {
                            std::cerr << "Struct Type MissMatch" << std::endl;
                            exit(1);
                        }
                        auto field_ptr = this->llvm_ir_builder.CreateStructGEP(struct_type, alloca, i);
                        this->llvm_ir_builder.CreateStore(args[i], field_ptr);
                    }
                    return {alloca, alloca, std::make_shared<enviornment::RecordStructInstance>(struct_record), compiler::resolveType::StructInst};
                } else {
                    std::cerr << "Struct Or Function " << name << " overload Dose Not Exit." << std::endl;
                    exit(1);
                }
            } else if(ltt == compiler::resolveType::StructInst) {
                auto left_type = std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_left_type);
                if(left_type->struct_type->is_method(std::static_pointer_cast<AST::IdentifierLiteral>(right)->value, params_types)) {
                    auto method = left_type->struct_type->get_method(std::static_pointer_cast<AST::IdentifierLiteral>(right)->value, params_types);
                    auto returnValue = this->llvm_ir_builder.CreateCall(method->function, args);
                    return {returnValue, nullptr, method->return_inst, compiler::resolveType::StructInst};
                } else {
                    std::cerr << "Struct does not have any method overload for " + std::static_pointer_cast<AST::IdentifierLiteral>(right)->value << std::endl;
                    exit(1);
                }
            }
        } else {
            std::cerr << "Member access should be identifier of method" << std::endl;
            exit(1);
        }
    }
    auto [right_value, right_alloca, _right_type, rtt] = this->_resolveValue(right);
    if(ltt != compiler::resolveType::StructInst || rtt != compiler::resolveType::StructInst) {
        std::cerr << "Infix Expression Value Error" << std::endl;
        exit(1);
    }
    auto left_val = left_value;
    auto right_val = right_value;
    auto left_type = std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_left_type);
    auto right_type = std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_right_type);
    std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_type1{left_type, right_type};
    std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_type2{right_type, left_type};
    if(left_type->struct_type->struct_type != nullptr || right_type->struct_type->struct_type != nullptr) {
        switch(op) {
        case token::TokenType::Plus: {
            if(left_type->struct_type->is_method("__add__", params_type1)) {
                auto func_record = left_type->struct_type->get_method("__add__", params_type1);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else if(right_type->struct_type->is_method("__add__", params_type2)) {
                auto func_record = right_type->struct_type->get_method("__add__", params_type2);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else {
                std::cerr << "Cant Add 2 Struct" << std::endl;
                exit(1);
            }
        }
        case token::TokenType::Dash: {
            if(left_type->struct_type->is_method("__sub__", params_type1)) {
                auto func_record = left_type->struct_type->get_method("__sub__", params_type1);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else if(right_type->struct_type->is_method("__sub__", params_type2)) {
                auto func_record = right_type->struct_type->get_method("__sub__", params_type2);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else {
                std::cerr << "Cant Substract 2 Struct" << std::endl;
                exit(1);
            }
        }
        case token::TokenType::Asterisk: {
            if(left_type->struct_type->is_method("__mul__", params_type1)) {
                auto func_record = left_type->struct_type->get_method("__mul__", params_type1);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else if(right_type->struct_type->is_method("__mul__", params_type2)) {
                auto func_record = right_type->struct_type->get_method("__mul__", params_type2);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else {
                std::cerr << "Cant Multiply 2 Struct" << std::endl;
                exit(1);
            }
        }
        case token::TokenType::ForwardSlash: {
            if(left_type->struct_type->is_method("__div__", params_type1)) {
                auto func_record = left_type->struct_type->get_method("__div__", params_type1);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else if(right_type->struct_type->is_method("__div__", params_type2)) {
                auto func_record = right_type->struct_type->get_method("__div__", params_type2);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else {
                std::cerr << "Cant Divide 2 Struct" << std::endl;
                exit(1);
            }
        }
        case token::TokenType::Percent: {
            if(left_type->struct_type->is_method("__mod__", params_type1)) {
                auto func_record = left_type->struct_type->get_method("__mod__", params_type1);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else if(right_type->struct_type->is_method("__mod__", params_type2)) {
                auto func_record = right_type->struct_type->get_method("__mod__", params_type2);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else {
                std::cerr << "Cant Modulate 2 Struct" << std::endl;
                exit(1);
            }
        }
        case token::TokenType::EqualEqual: {
            if(left_type->struct_type->is_method("__eq__", params_type1)) {
                auto func_record = left_type->struct_type->get_method("__eq__", params_type1);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else if(right_type->struct_type->is_method("__eq__", params_type2)) {
                auto func_record = right_type->struct_type->get_method("__eq__", params_type2);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else {
                std::cerr << "Cant Compare Equal 2 Struct" << std::endl;
                exit(1);
            }
        }
        case token::TokenType::NotEquals: {
            if(left_type->struct_type->is_method("__neq__", params_type1)) {
                auto func_record = left_type->struct_type->get_method("__neq__", params_type1);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else if(right_type->struct_type->is_method("__neq__", params_type2)) {
                auto func_record = right_type->struct_type->get_method("__neq__", params_type2);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else {
                std::cerr << "Cant Compare Not Equal 2 Struct" << std::endl;
                exit(1);
            }
        }
        case token::TokenType::LessThan: {
            if(left_type->struct_type->is_method("__lt__", params_type1)) {
                auto func_record = left_type->struct_type->get_method("__lt__", params_type1);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else if(right_type->struct_type->is_method("__lt__", params_type2)) {
                auto func_record = right_type->struct_type->get_method("__lt__", params_type2);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else {
                std::cerr << "Cant Compare Lessthan 2 Struct" << std::endl;
                exit(1);
            }
        }
        case token::TokenType::GreaterThan: {
            if(left_type->struct_type->is_method("__gt__", params_type1)) {
                auto func_record = left_type->struct_type->get_method("__gt__", params_type1);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else if(right_type->struct_type->is_method("__gt__", params_type2)) {
                auto func_record = right_type->struct_type->get_method("__gt__", params_type2);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else {
                std::cerr << "Cant Compare Graterthan 2 Struct" << std::endl;
                exit(1);
            }
        }
        case token::TokenType::LessThanOrEqual: {
            if(left_type->struct_type->is_method("__lte__", params_type1)) {
                auto func_record = left_type->struct_type->get_method("__lte__", params_type1);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else if(right_type->struct_type->is_method("__lte__", params_type2)) {
                auto func_record = right_type->struct_type->get_method("__lte__", params_type2);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else {
                std::cerr << "Cant Compare Lessthan or Equals 2 Struct" << std::endl;
                exit(1);
            }
        }
        case token::TokenType::GreaterThanOrEqual: {
            if(left_type->struct_type->is_method("__gte__", params_type1)) {
                auto func_record = left_type->struct_type->get_method("__gte__", params_type1);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else if(right_type->struct_type->is_method("__gte__", params_type2)) {
                auto func_record = right_type->struct_type->get_method("__gte__", params_type2);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else {
                std::cerr << "Cant Compare Greatherthan or Equals 2 Struct" << std::endl;
                exit(1);
            }
        }
        case token::TokenType::AsteriskAsterisk: {
            if(left_type->struct_type->is_method("__pow__", params_type1)) {
                auto func_record = left_type->struct_type->get_method("__pow__", params_type1);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else if(right_type->struct_type->is_method("__pow__", params_type2)) {
                auto func_record = right_type->struct_type->get_method("__pow__", params_type2);
                auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
                return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
            } else {
                std::cerr << "Cant Power 2 Struct" << std::endl;
                exit(1);
            }
        }
        default: {
            std::cerr << "Unknown Operator: " << *token::tokenTypeString(op) << std::endl;
            exit(1);
        }
        }
    }
    if(!enviornment::_checkType(left_type, right_type)) {
        std::cerr << "Type mismatch" << std::endl;
        exit(1);
    }

    llvm::Value* left_val_converted = left_val;
    llvm::Value* right_val_converted = right_val;
    auto common_type = left_type;

    // Handle type conversion
    if((!left_type->struct_type->stand_alone_type->isIntegerTy(1)) && left_type->struct_type->stand_alone_type->isIntegerTy() && right_type->struct_type->stand_alone_type->isDoubleTy()) {
        left_val_converted = this->llvm_ir_builder.CreateSIToFP(left_val, llvm::Type::getDoubleTy(this->llvm_context));
        common_type = right_type;
    } else if(left_type->struct_type->stand_alone_type->isDoubleTy() && right_type->struct_type->stand_alone_type->isIntegerTy() && (!right_type->struct_type->stand_alone_type->isIntegerTy(1))) {
        right_val_converted = this->llvm_ir_builder.CreateSIToFP(right_val, llvm::Type::getDoubleTy(this->llvm_context));
    } else if((!left_type->struct_type->stand_alone_type->isIntegerTy(1)) && left_type->struct_type->stand_alone_type->isIntegerTy() && right_type->struct_type->stand_alone_type->isFloatTy()) {
        left_val_converted = this->llvm_ir_builder.CreateSIToFP(left_val, llvm::Type::getFloatTy(this->llvm_context));
        common_type = right_type;
    } else if(left_type->struct_type->stand_alone_type->isFloatTy() && right_type->struct_type->stand_alone_type->isIntegerTy() && (!right_type->struct_type->stand_alone_type->isIntegerTy(1))) {
        right_val_converted = this->llvm_ir_builder.CreateSIToFP(right_val, llvm::Type::getFloatTy(this->llvm_context));
    } else if((left_type->struct_type->stand_alone_type->isIntegerTy() && right_type->struct_type->stand_alone_type->isIntegerTy()) ||
              (left_type->struct_type->stand_alone_type->isIntegerTy(1) && right_type->struct_type->stand_alone_type->isIntegerTy(1)) ||
              left_type->struct_type->stand_alone_type->isFloatTy() && right_type->struct_type->stand_alone_type->isFloatTy() ||
              left_type->struct_type->stand_alone_type->isDoubleTy() && right_type->struct_type->stand_alone_type->isDoubleTy()) {
        // No conversion Needed
    } else {
        std::cerr << "Unknown Type" << std::endl;
        exit(1);
    }

    if(common_type->struct_type->stand_alone_type->isIntegerTy()) {
        switch(op) {
        case(token::TokenType::Plus): {
            auto inst = this->llvm_ir_builder.CreateAdd(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("int")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::Dash): {
            auto inst = this->llvm_ir_builder.CreateSub(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("int")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::Asterisk): {
            auto inst = this->llvm_ir_builder.CreateMul(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("int")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::ForwardSlash): {
            llvm::Value* inst;
            if(left_type->struct_type->name == "int" || left_type->struct_type->name == "int32" || left_type->struct_type->name == "int128")
                inst = this->llvm_ir_builder.CreateSDiv(left_val_converted, right_val_converted);
            else
                inst = this->llvm_ir_builder.CreateUDiv(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("int")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::Percent): {
            llvm::Value* inst;
            if(left_type->struct_type->name == "int" || left_type->struct_type->name == "int32" || left_type->struct_type->name == "int128")
                inst = this->llvm_ir_builder.CreateSRem(left_val_converted, right_val_converted);
            else
                inst = this->llvm_ir_builder.CreateURem(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("int")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::EqualEqual): {
            auto inst = this->llvm_ir_builder.CreateICmpEQ(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::NotEquals): {
            auto inst = this->llvm_ir_builder.CreateICmpNE(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::LessThan): {
            llvm::Value* inst;
            if(left_type->struct_type->name == "int" || left_type->struct_type->name == "int32" || left_type->struct_type->name == "int128")
                inst = this->llvm_ir_builder.CreateICmpSLT(left_val_converted, right_val_converted);
            else
                inst = this->llvm_ir_builder.CreateICmpULT(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::GreaterThan): {
            llvm::Value* inst;
            if(left_type->struct_type->name == "int" || left_type->struct_type->name == "int32" || left_type->struct_type->name == "int128")
                inst = this->llvm_ir_builder.CreateICmpSGT(left_val_converted, right_val_converted);
            else
                inst = this->llvm_ir_builder.CreateICmpUGT(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::LessThanOrEqual): {
            llvm::Value* inst;
            if(left_type->struct_type->name == "int" || left_type->struct_type->name == "int32" || left_type->struct_type->name == "int128")
                inst = this->llvm_ir_builder.CreateICmpSLE(left_val_converted, right_val_converted);
            else
                inst = this->llvm_ir_builder.CreateICmpULE(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::GreaterThanOrEqual): {
            llvm::Value* inst;
            if(left_type->struct_type->name == "int" || left_type->struct_type->name == "int32" || left_type->struct_type->name == "int128")
                inst = this->llvm_ir_builder.CreateICmpSGE(left_val_converted, right_val_converted);
            else
                inst = this->llvm_ir_builder.CreateICmpUGE(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::AsteriskAsterisk): {
            std::cerr << "Power operator not supported for int" << std::endl;
            exit(1);
        }
        default: {
            std::cerr << "Unknown operator: " << *token::tokenTypeString(op) << std::endl;
            exit(1);
        }
        }
    } else if(common_type->struct_type->stand_alone_type->isDoubleTy()) {
        switch(op) {
        case(token::TokenType::Plus): {
            auto inst = this->llvm_ir_builder.CreateFAdd(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("float")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::Dash): {
            auto inst = this->llvm_ir_builder.CreateFSub(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("float")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::Asterisk): {
            auto inst = this->llvm_ir_builder.CreateFMul(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("float")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::ForwardSlash): {
            auto inst = this->llvm_ir_builder.CreateFDiv(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("float")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::EqualEqual): {
            auto inst = this->llvm_ir_builder.CreateFCmpOEQ(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::NotEquals): {
            auto inst = this->llvm_ir_builder.CreateFCmpONE(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::LessThan): {
            auto inst = this->llvm_ir_builder.CreateFCmpOLT(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::GreaterThan): {
            auto inst = this->llvm_ir_builder.CreateFCmpOGT(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::LessThanOrEqual): {
            auto inst = this->llvm_ir_builder.CreateFCmpOLE(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::GreaterThanOrEqual): {
            auto inst = this->llvm_ir_builder.CreateFCmpOGE(left_val_converted, right_val_converted);
            return {inst, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
        }
        case(token::TokenType::AsteriskAsterisk): {
            std::cerr << "Power operator not supported for float" << std::endl;
            exit(1);
        }
        default: {
            std::cerr << "Unknown operator: " << *token::tokenTypeString(op) << std::endl;
            exit(1);
        }
        }
    } else {
        std::cerr << "Unknown Type" << std::endl;
        exit(1);
    }
};

std::tuple<llvm::Value*, llvm::Value*,
           std::variant<std::shared_ptr<enviornment::RecordStructInstance>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, compiler::resolveType>
compiler::Compiler::_visitIndexExpression(std::shared_ptr<AST::IndexExpression> index_expression) {
    auto [left, _, _left_generic, ltt] = this->_resolveValue(index_expression->left);
    if(ltt == compiler::resolveType::StructInst) {
        std::cerr << "Cant index Module" << std::endl;
        exit(1);
    }
    auto left_generic = std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_left_generic);
    auto [index, __, _index_generic, itt] = this->_resolveValue(index_expression->index);
    if(itt == compiler::resolveType::StructInst) {
        std::cerr << "Index Must be Intiger Not Module" << std::endl;
        exit(1);
    }
    auto index_generic = std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_index_generic);
    if(!enviornment::_checkType(left_generic, this->enviornment->get_struct("array"))) {
        std::cerr << "Error: Left type is not an array. Left type: " << left_generic->struct_type->name << std::endl;
        exit(1);
    }
    if(!enviornment::_checkType(index_generic, this->enviornment->get_struct("int"))) {
        std::cerr << "Error: Index type is not an int. Index type: " << index_generic->struct_type->name << std::endl;
        exit(1);
    }
    auto element = this->llvm_ir_builder.CreateGEP(
        left_generic->generic[0]->struct_type->stand_alone_type ? left_generic->generic[0]->struct_type->stand_alone_type : left_generic->generic[0]->struct_type->struct_type, left, index, "element");
    auto load = left_generic->generic[0]->struct_type->stand_alone_type ? this->llvm_ir_builder.CreateLoad(left_generic->generic[0]->struct_type->stand_alone_type, element) : element;
    return {load, element, left_generic->generic[0], compiler::resolveType::StructInst};
};

void compiler::Compiler::_visitVariableDeclarationStatement(std::shared_ptr<AST::VariableDeclarationStatement> variable_declaration_statement) {
    std::cerr << "Entering _visitVariableDeclarationStatement" << std::endl;
    auto var_name = std::static_pointer_cast<AST::IdentifierLiteral>(variable_declaration_statement->name);
    std::cerr << "Variable name: " << var_name->value << std::endl;
    auto var_value = variable_declaration_statement->value;
    std::cerr << "Resolving variable type" << std::endl;
    auto var_type = this->_parseType(variable_declaration_statement->value_type);
    std::cerr << "Variable type: " << var_type->struct_type->name << std::endl;
    auto [var_value_resolved, var_value_alloca, _var_generic, vartt] = this->_resolveValue(var_value);
    std::cerr << "Resolved variable value" << std::endl;
    if(vartt != compiler::resolveType::StructInst) {
        std::cerr << "Cant Assign Modult to Variable" << std::endl;
        exit(1);
    }
    auto var_generic = std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_var_generic);
    if(!enviornment::_checkType(var_generic, this->_parseType(variable_declaration_statement->value_type))) {
        std::cerr << "Cannot assign missmatch type" << std::endl;
        exit(1);
    }
    if(vartt == compiler::resolveType::StructInst) {
        if(var_type->struct_type->struct_type == nullptr) {
            std::cerr << "Allocating standalone type" << std::endl;
            auto alloca = this->llvm_ir_builder.CreateAlloca(var_type->struct_type->stand_alone_type);
            auto store = this->llvm_ir_builder.CreateStore(var_value_resolved, alloca, variable_declaration_statement->is_volatile);
            auto var = std::make_shared<enviornment::RecordVariable>(var_name->value, var_value_resolved, alloca, var_generic);
            this->enviornment->add(var);
        } else {
            std::cerr << "Allocating struct type" << std::endl;
            auto alloca = this->llvm_ir_builder.CreateAlloca(var_type->struct_type->struct_type, nullptr);
            auto load = this->llvm_ir_builder.CreateLoad(var_type->struct_type->struct_type, var_value_resolved);
            auto store = this->llvm_ir_builder.CreateStore(load, alloca, variable_declaration_statement->is_volatile);
            auto var = std::make_shared<enviornment::RecordVariable>(var_name->value, var_value_resolved, alloca, var_generic);
            var->variableType = var_generic;
            this->enviornment->add(var);
        }
    } else {
        std::cerr << "Variable declaration with multiple values" << std::endl;
        exit(1);
    }
    std::cerr << "Exiting _visitVariableDeclarationStatement" << std::endl;
}

std::vector<std::string> splitString(const std::string& input) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = input.find('.');
    while(end != std::string::npos) {
        result.push_back(input.substr(start, end - start));
        start = end + 1;
        end = input.find('.', start);
    }
    result.push_back(input.substr(start));
    return result;
}

void compiler::Compiler::_visitVariableAssignmentStatement(std::shared_ptr<AST::VariableAssignmentStatement> variable_assignment_statement) {
    auto var_value = variable_assignment_statement->value;
    auto [value, value_aloca, _assignmentType, vtt] = this->_resolveValue(var_value);
    auto assignmentType = std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_assignmentType);
    if(vtt != compiler::resolveType::StructInst) {
        std::cerr << "Cant Assign Modult or type to Variable" << std::endl;
        exit(1);
    }
    auto [_, alloca, _var_type, att] = this->_resolveValue(variable_assignment_statement->name);
    if(att != compiler::resolveType::StructInst) {
        std::cerr << "Cant Assign to ltype" << std::endl;
        exit(1);
    }
    auto var_type = std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_var_type);
    if(!enviornment::_checkType(var_type, assignmentType)) {
        std::cerr << "Cannot assign missmatch type" << std::endl;
        exit(1);
    }
    if(assignmentType->struct_type->stand_alone_type) {
        auto store = this->llvm_ir_builder.CreateStore(value, alloca);
    } else {
        auto load = this->llvm_ir_builder.CreateLoad(assignmentType->struct_type->struct_type, value);
        auto store = this->llvm_ir_builder.CreateStore(load, alloca);
    }
};

std::tuple<llvm::Value*, llvm::Value*,
           std::variant<std::shared_ptr<enviornment::RecordStructInstance>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, compiler::resolveType>
compiler::Compiler::_resolveValue(std::shared_ptr<AST::Node> node) {
    switch(node->type()) {
    case AST::NodeType::IntegerLiteral: {
        auto integer_literal = std::static_pointer_cast<AST::IntegerLiteral>(node);
        auto value = llvm::ConstantInt::get(llvm_context, llvm::APInt(64, integer_literal->value));
        return {value, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("int")), compiler::resolveType::StructInst};
    }
    case AST::NodeType::FloatLiteral: {
        auto float_literal = std::static_pointer_cast<AST::FloatLiteral>(node);
        auto value = llvm::ConstantFP::get(llvm_context, llvm::APFloat(float_literal->value));
        return {value, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("float")), compiler::resolveType::StructInst};
    }
    case AST::NodeType::StringLiteral: {
        auto string_literal = std::static_pointer_cast<AST::StringLiteral>(node);
        auto value = this->llvm_ir_builder.CreateGlobalStringPtr(string_literal->value);
        return {value, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("str")), compiler::resolveType::StructInst};
    }
    case AST::NodeType::IdentifierLiteral: {
        auto identifier_literal = std::static_pointer_cast<AST::IdentifierLiteral>(node);
        std::shared_ptr<enviornment::RecordStructInstance> currentStructType = nullptr;
        if(this->enviornment->is_variable(identifier_literal->value)) {
            currentStructType = this->enviornment->get_variable(identifier_literal->value)->variableType;
            if(currentStructType->struct_type->stand_alone_type) {
                auto loadInst = this->llvm_ir_builder.CreateLoad(currentStructType->struct_type->stand_alone_type, this->enviornment->get_variable(identifier_literal->value)->allocainst);
                return {loadInst, this->enviornment->get_variable(identifier_literal->value)->allocainst, currentStructType, compiler::resolveType::StructInst};
            } else {
                return {this->enviornment->get_variable(identifier_literal->value)->allocainst, this->enviornment->get_variable(identifier_literal->value)->allocainst, currentStructType,
                        compiler::resolveType::StructInst};
            }
        } else if(this->enviornment->is_module(identifier_literal->value)) {
            return {nullptr, nullptr, this->enviornment->get_module(identifier_literal->value), compiler::resolveType::Module};
        } else if(this->enviornment->is_struct(identifier_literal->value)) {
            return {nullptr, nullptr, this->enviornment->get_struct(identifier_literal->value), compiler::resolveType::StructType};
        }
        std::cerr << "Variable not defined: " << identifier_literal->value << std::endl;
        exit(1);
    }
    case AST::NodeType::InfixedExpression: {
        return this->_visitInfixExpression(std::static_pointer_cast<AST::InfixExpression>(node));
    }
    case AST::NodeType::IndexExpression: {
        return this->_visitIndexExpression(std::static_pointer_cast<AST::IndexExpression>(node));
    }
    case AST::NodeType::CallExpression: {
        return this->_visitCallExpression(std::static_pointer_cast<AST::CallExpression>(node));
    }
    case AST::NodeType::BooleanLiteral: {
        auto boolean_literal = std::static_pointer_cast<AST::BooleanLiteral>(node);
        auto value = boolean_literal->value ? this->enviornment->get_variable("True")->value : this->enviornment->get_variable("False")->value;
        return {value, nullptr, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("bool")), compiler::resolveType::StructInst};
    }
    case AST::NodeType::ArrayLiteral: {
        return this->_visitArrayLiteral(std::static_pointer_cast<AST::ArrayLiteral>(node));
    }
    default: {
        std::cerr << "Compiling unknown node type: " << *AST::nodeTypeToString(node->type()) << std::endl;
        exit(1);
    }
    }
};

std::tuple<llvm::Value*, llvm::Value*,
           std::variant<std::shared_ptr<enviornment::RecordStructInstance>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, compiler::resolveType>
compiler::Compiler::_visitArrayLiteral(std::shared_ptr<AST::ArrayLiteral> array_literal) {
    std::vector<llvm::Value*> values;
    std::shared_ptr<enviornment::RecordStructType> struct_type = nullptr;
    std::vector<std::shared_ptr<enviornment::RecordStructInstance>> generics;
    std::shared_ptr<enviornment::RecordStructInstance> first_generic;

    for(auto element : array_literal->elements) {
        auto [value, value_alloca, _generic, vtt] = this->_resolveValue(element);
        if(vtt != compiler::resolveType::StructInst) {
            std::cerr << "Cant add Module or type in Array" << std::endl;
            exit(1);
        }
        auto generic = std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_generic);
        if(!struct_type) {
            struct_type = generic->struct_type;
            first_generic = generic;
            generics.push_back(generic);
        }
        if(!_checkType(first_generic, generic)) {
            errors::CompletionError("Array with multiple types or generics", this->source, array_literal->meta_data.st_line_no, array_literal->meta_data.end_line_no,
                                    "Array contains elements of different types or generics")
                .raise();
            exit(1);
        }
        auto loadInst = struct_type->struct_type ? value : this->llvm_ir_builder.CreateLoad(struct_type->stand_alone_type, value_alloca);
        values.push_back(loadInst);
    }
    auto array_type = llvm::ArrayType::get(struct_type->stand_alone_type ? struct_type->stand_alone_type : struct_type->struct_type, values.size());
    auto array = this->llvm_ir_builder.CreateAlloca(array_type, nullptr);
    for(int i = 0; i < values.size(); i++) {
        auto element = this->llvm_ir_builder.CreateGEP(array_type, array, {this->llvm_ir_builder.getInt64(0), this->llvm_ir_builder.getInt64(i)});
        auto storeInst = this->llvm_ir_builder.CreateStore(values[i], element);
    }
    return {array, array, std::make_shared<enviornment::RecordStructInstance>(this->enviornment->get_struct("array"), generics), compiler::resolveType::StructInst};
};

void compiler::Compiler::_visitReturnStatement(std::shared_ptr<AST::ReturnStatement> return_statement) {
    auto value = return_statement->value;
    auto [return_value, return_alloca, _, rtt] = this->_resolveValue(value);
    if(this->enviornment->current_function == nullptr) {
        std::cerr << "Return Outside of function" << std::endl;
        exit(1);
    }
    if(rtt != compiler::resolveType::StructInst) {
        if(rtt == compiler::resolveType::StructType && std::get<std::shared_ptr<enviornment::RecordStructType>>(_)->name == "void") {
            this->llvm_ir_builder.CreateRetVoid();
            return;
        } else {
            std::cerr << "Return statement with multiple values" << std::endl;
            exit(1);
        }
    }
    if(!enviornment::_checkType(this->enviornment->current_function->return_inst, std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_))) {
        std::cerr << "Return Type Miss Match" << std::endl;
        exit(1);
    }
    if(this->enviornment->current_function->function->getReturnType()->isPointerTy() && return_value->getType()->isPointerTy()) {
        this->llvm_ir_builder.CreateRet(return_value);
    } else if(this->enviornment->current_function->function->getReturnType()->isPointerTy() && !return_value->getType()->isPointerTy()) {
        this->llvm_ir_builder.CreateRet(return_alloca);
    } else if(!this->enviornment->current_function->function->getReturnType()->isPointerTy() && return_value->getType()->isPointerTy()) {
        this->llvm_ir_builder.CreateRet(this->llvm_ir_builder.CreateLoad(this->enviornment->current_function->function->getReturnType(), return_value));
    } else {
        this->llvm_ir_builder.CreateRet(return_value);
    }
    throw compiler::DoneRet();
};

std::shared_ptr<enviornment::RecordStructInstance> compiler::Compiler::_parseType(std::shared_ptr<AST::Type> type) {
    std::vector<std::shared_ptr<enviornment::RecordStructInstance>> generics = {};
    for(auto gen : type->generics) {
        generics.push_back(this->_parseType(gen));
    }
    auto [_, __, _struct, stt] = this->_resolveValue(type->name);
    if(stt != compiler::resolveType::StructType) {
        std::cerr << "Type is Wrong" << std::endl;
        exit(1);
    }
    auto struct_ = std::get<std::shared_ptr<enviornment::RecordStructType>>(_struct);
    auto x = std::make_shared<enviornment::RecordStructInstance>(struct_, generics);
    return x;
};

void compiler::Compiler::_visitFunctionDeclarationStatement(std::shared_ptr<AST::FunctionStatement> function_declaration_statement, std::shared_ptr<enviornment::RecordStructType> struct_) {
    auto name = std::static_pointer_cast<AST::IdentifierLiteral>(function_declaration_statement->name)->value;
    if(function_declaration_statement->generic.size() != 0) {
        auto func_gen_rec = std::make_shared<enviornment::RecordGenericFunction>(name, function_declaration_statement, this->enviornment);
        this->enviornment->add(func_gen_rec);
        return;
    }
    auto body = function_declaration_statement->body;
    auto params = function_declaration_statement->parameters;
    std::vector<std::string> param_name;
    std::vector<llvm::Type*> param_types;
    std::vector<std::shared_ptr<enviornment::RecordStructInstance>> param_inst_record;
    for(auto param : params) {
        auto param_name_str = std::static_pointer_cast<AST::IdentifierLiteral>(param->name)->value;
        param_name.push_back(param_name_str);
        auto param_type = this->_parseType(param->value_type);
        param_inst_record.push_back(param_type);
        param_types.push_back(param_type->struct_type->stand_alone_type ? param_type->struct_type->stand_alone_type : llvm::PointerType::get(param_type->struct_type->struct_type, 0));
    }
    auto return_type = this->_parseType(function_declaration_statement->return_type);
    auto llvm_return_type = return_type->struct_type->stand_alone_type ? return_type->struct_type->stand_alone_type : return_type->struct_type->struct_type->getPointerTo();
    auto func_type = llvm::FunctionType::get(llvm_return_type, param_types, false);
    auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, this->fc_st_name_prefix != "main.gc.." ? this->fc_st_name_prefix + name : name, this->llvm_module.get());
    this->ir_gc_map_json["functions"][name] = func->getName().str();
    unsigned idx = 0;
    for(auto& arg : func->args()) {
        arg.setName(param_name[idx++]);
    }
    std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructInstance>>> arguments;
    auto func_record = std::make_shared<enviornment::RecordFunction>(name, func, func_type, arguments, return_type);
    if(body) {
        auto bb = llvm::BasicBlock::Create(this->llvm_context, "entry", func);
        this->function_entery_block.push_back(bb);
        this->llvm_ir_builder.SetInsertPoint(bb);
        auto prev_env = this->enviornment;
        this->enviornment = std::make_shared<enviornment::Enviornment>(prev_env, std::vector<std::tuple<std::string, std::shared_ptr<enviornment::Record>>>{}, name);
        this->enviornment->current_function = func_record;
        for(const auto& [arg, param_type_record] : llvm::zip(func->args(), param_inst_record)) {
            llvm::Value* alloca = nullptr;
            if(!arg.getType()->isPointerTy() || enviornment::_checkType(param_type_record, this->enviornment->get_struct("array"))) {
                alloca = this->llvm_ir_builder.CreateAlloca(arg.getType(), nullptr, arg.getName());
                auto storeInst = this->llvm_ir_builder.CreateStore(&arg, alloca);
            } else {
                alloca = &arg;
                // alloca = this->llvm_ir_builder.CreateAlloca(param_type_record->struct_type->struct_type, nullptr, arg.getName());
                // auto loaded_arg = this->llvm_ir_builder.CreateLoad(param_type_record->struct_type->struct_type, &arg, arg.getName() + ".load");
                // auto storeInst = this->llvm_ir_builder.CreateStore(loaded_arg, alloca);
            }
            auto record = std::make_shared<enviornment::RecordVariable>(std::string(arg.getName()), &arg, alloca, param_type_record);
            func_record->arguments.push_back({arg.getName().str(), param_type_record});
            this->enviornment->add(record);
        }
        func_record->set_meta_data(function_declaration_statement->meta_data.st_line_no, function_declaration_statement->meta_data.st_col_no, function_declaration_statement->meta_data.end_line_no,
                                   function_declaration_statement->meta_data.end_col_no);
        this->enviornment->add(func_record);
        try {
            this->compile(body);
        } catch(compiler::DoneRet) {
            // Ignoring
        }
        this->enviornment = prev_env;
        this->function_entery_block.pop_back();
        if(!this->function_entery_block.empty()) {
            this->llvm_ir_builder.SetInsertPoint(this->function_entery_block.at(this->function_entery_block.size() - 1));
        }
    } else {
        for(const auto& [arg, param_type_record] : llvm::zip(func->args(), param_inst_record)) {
            func_record->arguments.push_back({arg.getName().str(), param_type_record});
        };
    }
    func_record->meta_data.more_data["name_line_no"] = function_declaration_statement->name->meta_data.st_line_no;
    func_record->meta_data.more_data["name_st_col_no"] = function_declaration_statement->name->meta_data.st_col_no;
    func_record->meta_data.more_data["name_end_col_no"] = function_declaration_statement->name->meta_data.end_col_no;
    func_record->meta_data.more_data["name_end_line_no"] = function_declaration_statement->name->meta_data.end_line_no;
    if(struct_)
        struct_->methods.push_back({name, func_record});
    else
        this->enviornment->add(func_record);
};

std::tuple<llvm::Value*, llvm::Value*,
           std::variant<std::shared_ptr<enviornment::RecordStructInstance>, std::shared_ptr<enviornment::RecordModule>, std::shared_ptr<enviornment::RecordStructType>>, compiler::resolveType>
compiler::Compiler::_visitCallExpression(std::shared_ptr<AST::CallExpression> call_expression) {
    auto name = std::static_pointer_cast<AST::IdentifierLiteral>(call_expression->name)->value;
    auto param = call_expression->arguments;
    std::vector<llvm::Value*> args;
    std::vector<std::shared_ptr<enviornment::RecordStructInstance>> params_types;
    if(this->enviornment->is_function(name, params_types)) {
        for(auto arg : param) {
            auto [value, _, param_type, ptt] = this->_resolveValue(arg);
            if(ptt != compiler::resolveType::StructInst) {
                std::cerr << "Cant pass Module or type to the Function" << std::endl;
                exit(1);
            }
            params_types.push_back(std::get<std::shared_ptr<enviornment::RecordStructInstance>>(param_type));
            args.push_back(value);
        }
        auto func_record = this->enviornment->get_function(name, params_types);
        auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, args);
        return {returnValue, nullptr, func_record->return_inst, compiler::resolveType::StructInst};
    } else if(this->enviornment->is_Gfunc(name)) {
        for(auto arg : param) {
            auto [value, _, param_type, ptt] = this->_resolveValue(arg);
            if(ptt != compiler::resolveType::StructInst) {
                std::cerr << "Cant pass Module or type to the Function" << std::endl;
                exit(1);
            }
            params_types.push_back(std::get<std::shared_ptr<enviornment::RecordStructInstance>>(param_type));
            args.push_back(value);
        }
        auto gfuncs = this->enviornment->get_Gfunc(name);
        return this->_CallGfunc(gfuncs, name, args, params_types);
    } else if(this->enviornment->is_struct(name)) {
        auto struct_record = this->enviornment->get_struct(name);
        auto struct_type = struct_record->struct_type;
        auto alloca = this->llvm_ir_builder.CreateAlloca(struct_type, nullptr, name);
        params_types.push_back(std::make_shared<enviornment::RecordStructInstance>(struct_record));
        args.push_back(alloca);
        for(auto arg : param) {
            auto [value, _, param_type, ptt] = this->_resolveValue(arg);
            if(ptt != compiler::resolveType::StructInst) {
                std::cerr << "Cant pass Module or type to the struct Initilazior" << std::endl;
                exit(1);
            }
            params_types.push_back(std::get<std::shared_ptr<enviornment::RecordStructInstance>>(param_type));
            args.push_back(value);
        }
        auto func = struct_record->get_method(struct_record->name, params_types);
        if(func)
            this->llvm_ir_builder.CreateCall(func->function, args);
        else {
            std::cerr << "Initilaxation method is not Created for the struct: `" + struct_record->name + "`" << std::endl;
            exit(1);
        }
        return {alloca, alloca, std::make_shared<enviornment::RecordStructInstance>(struct_record), compiler::resolveType::StructInst};
    }
    errors::CompletionError("Function not defined", this->source, call_expression->meta_data.st_line_no, call_expression->meta_data.end_line_no, "Function `" + name + "` not defined").raise();
    exit(1);
};

void compiler::Compiler::_visitIfElseStatement(std::shared_ptr<AST::IfElseStatement> if_statement) {
    auto condition = if_statement->condition;
    auto consequence = if_statement->consequence;
    auto alternative = if_statement->alternative;
    auto [condition_val, _, _condition, ctt] = this->_resolveValue(condition);
    if(ctt != compiler::resolveType::StructInst) {
        std::cerr << "Condition Cant Be Module" << std::endl;
        exit(1);
    }
    auto bool_condition = std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_condition);
    if(!enviornment::_checkType(bool_condition, this->enviornment->get_struct("bool"))) {
        std::cerr << "Condition type Must be Bool" << std::endl;
        exit(1);
    }
    if(alternative == nullptr) {
        auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(llvm_context, "then", func);
        llvm::BasicBlock* ContBB = llvm::BasicBlock::Create(llvm_context, "cont", func);
        auto condBr = this->llvm_ir_builder.CreateCondBr(condition_val, ThenBB, ContBB);
        this->llvm_ir_builder.SetInsertPoint(ThenBB);
        try {
            this->compile(consequence);
            auto brThen = this->llvm_ir_builder.CreateBr(ContBB);
            this->llvm_ir_builder.SetInsertPoint(ContBB);
        } catch(compiler::DoneRet) {
            // Doing Noting cz We want to Ignore to compile the foollowing comands if DoneRet exception occur;
        }
    } else {
        auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(llvm_context, "then", func);
        llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(llvm_context, "else", func);
        auto condBr = this->llvm_ir_builder.CreateCondBr(condition_val, ThenBB, ElseBB);
        try {
            this->llvm_ir_builder.SetInsertPoint(ThenBB);
            this->compile(consequence);
            llvm::BasicBlock* ContBB = llvm::BasicBlock::Create(llvm_context, "cont", func);
            this->llvm_ir_builder.SetInsertPoint(ContBB);
        } catch(compiler::DoneRet) {
            // Doing Noting cz We want to Ignore to compile the foollowing comands if DoneRet exception occur;
        }
        try {
            this->llvm_ir_builder.SetInsertPoint(ElseBB);
            this->compile(alternative);
            llvm::BasicBlock* ContBB = llvm::BasicBlock::Create(llvm_context, "cont", func);
            this->llvm_ir_builder.SetInsertPoint(ContBB);
        } catch(compiler::DoneRet) {
            // Doing Noting cz We want to Ignore to compile the foollowing comands if DoneRet exception occur;
        }
    }
};

void compiler::Compiler::_visitWhileStatement(std::shared_ptr<AST::WhileStatement> while_statement) {
    auto condition = while_statement->condition;
    auto body = while_statement->body;
    auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* CondBB = llvm::BasicBlock::Create(llvm_context, "cond", func);
    llvm::BasicBlock* BodyBB = llvm::BasicBlock::Create(llvm_context, "body", func);
    llvm::BasicBlock* ContBB = llvm::BasicBlock::Create(llvm_context, "cont", func);
    auto brToCond = this->llvm_ir_builder.CreateBr(CondBB);
    this->llvm_ir_builder.SetInsertPoint(CondBB);
    auto [condition_val, _, _condition, ctt] = this->_resolveValue(condition);
    if(ctt != compiler::resolveType::StructInst) {
        std::cerr << "Condition Cant Be Module" << std::endl;
        exit(1);
    }
    auto bool_condition = std::get<std::shared_ptr<enviornment::RecordStructInstance>>(_condition);
    if(!enviornment::_checkType(bool_condition, this->enviornment->get_struct("bool"))) {
        std::cerr << "Condition type Must be Bool" << std::endl;
        exit(1);
    }
    auto condBr = this->llvm_ir_builder.CreateCondBr(condition_val, BodyBB, ContBB);
    this->enviornment->loop_body_block.push_back(BodyBB);
    this->enviornment->loop_end_block.push_back(ContBB);
    this->enviornment->loop_condition_block.push_back(CondBB);
    this->llvm_ir_builder.SetInsertPoint(BodyBB);
    try {
        this->compile(body);
        auto brToCondAgain = this->llvm_ir_builder.CreateBr(CondBB);
        this->llvm_ir_builder.SetInsertPoint(ContBB);
    } catch(compiler::DoneRet) {
        // Doing Noting cz We want to Ignore to compile the foollowing comands if DoneRet exception occur;
    }
    this->enviornment->loop_body_block.pop_back();
    this->enviornment->loop_end_block.pop_back();
    this->enviornment->loop_condition_block.pop_back();
};

void compiler::Compiler::_visitStructStatement(std::shared_ptr<AST::StructStatement> struct_statement) {
    std::string struct_name = std::static_pointer_cast<AST::IdentifierLiteral>(struct_statement->name)->value;
    std::vector<llvm::Type*> field_types;
    auto fields = struct_statement->fields;
    auto struct_record = std::make_shared<enviornment::RecordStructType>(struct_name);
    this->enviornment->add(struct_record);
    for(auto field : fields) {
        if(field->type() == AST::NodeType::VariableDeclarationStatement) {
            auto field_decl = std::static_pointer_cast<AST::VariableDeclarationStatement>(field);
            std::string field_name = std::static_pointer_cast<AST::IdentifierLiteral>(field_decl->name)->value;
            struct_record->fields.push_back(field_name);
            auto field_type = this->_parseType(field_decl->value_type);
            if(field_type->struct_type->stand_alone_type == nullptr) {
                field_types.push_back(field_type->struct_type->struct_type);
            } else {
                field_types.push_back(field_type->struct_type->stand_alone_type);
            }
            struct_record->sub_types[field_name] = field_type;
            auto struct_type = llvm::StructType::create(this->llvm_context, field_types, this->fc_st_name_prefix + struct_name);
            struct_type->setBody(field_types);
            struct_record->struct_type = struct_type;
        } else if(field->type() == AST::NodeType::FunctionStatement) {
            auto func_dec = std::static_pointer_cast<AST::FunctionStatement>(field);
            if(func_dec->generic.size() != 0) {
                std::cerr << "Struct Methods DO not Suport the GenericFunctions" << std::endl;
                exit(1);
            }
            this->_visitFunctionDeclarationStatement(func_dec, struct_record);
        }
    }
    this->ir_gc_map_json["structs"][struct_name] = struct_record->struct_type->getName().str();
};

// Function to read the file content into a string
const std::string readFileToString(const std::string& filePath); // Defined in main.cpp

void compiler::Compiler::_visitImportStatement(std::shared_ptr<AST::ImportStatement> import_statement, std::shared_ptr<enviornment::RecordModule> module) {
    std::string relative_path = import_statement->relativePath;
    auto module_name = relative_path.substr(relative_path.find_last_of('/') + 1);
    std::replace(module_name.begin(), module_name.end(), '.', '_');
    if(!module) {
        module = std::make_shared<enviornment::RecordModule>(module_name);
        this->enviornment->add(module);
    } else {
        auto new_mod = std::make_shared<enviornment::RecordModule>(module_name);
        module->record_map.push_back({module_name, new_mod});
        module = new_mod;
    }
    if(relative_path.ends_with(".c")) {
        std::cerr << "cant import C code" << std::endl;
        exit(1);
        // Handle C file import
        auto ir_gc_map = std::filesystem::path(this->ir_gc_map.parent_path().string() + "/" + relative_path + ".json");
        std::ifstream ir_gc_map_file(ir_gc_map);
        if(!ir_gc_map_file.is_open()) {
            std::cerr << "Failed to open ir_gc_map file: " << ir_gc_map << std::endl;
            throw std::runtime_error("Failed to open ir_gc_map file: " + ir_gc_map.string());
        }
        ir_gc_map_file >> ir_gc_map_json;
        ir_gc_map_file.close();
        bool uptodate = ir_gc_map_json["uptodate"];
        if(!uptodate) {
            std::cerr << "IR GC map not uptodate, throwing NotCompiledError" << std::endl;
            throw compiler::NotCompiledError((this->file_path.parent_path() / relative_path).string());
        }
        return;
    } else if(relative_path.ends_with(".py")) {
        std::cerr << "Cant Import Python code" << std::endl;
        exit(1);
    }
    auto gc_source_path = std::filesystem::path(this->file_path.parent_path().string() + "/" + relative_path + ".gc");
    if(relative_path.starts_with("std/")) {
        gc_source_path = std::filesystem::path((GC_STD_DIR / relative_path.substr(4)).string() + ".gc");
    }
    nlohmann::json ir_gc_map_json;
    auto ir_gc_map = std::filesystem::path(this->ir_gc_map.parent_path().string() + "/" + relative_path + ".gc" + ".json");
    if(relative_path.starts_with("std/")) {
        ir_gc_map = std::filesystem::path(GC_STD_IRGCMAP / (relative_path.substr(4) + ".gc" + ".json"));
    }
    std::ifstream ir_gc_map_file(ir_gc_map);
    if(!ir_gc_map_file.is_open()) {
        std::cerr << "Failed to open ir_gc_map file: " << ir_gc_map << std::endl;
        throw std::runtime_error("Failed to open ir_gc_map file: " + ir_gc_map.string());
    }
    ir_gc_map_file >> ir_gc_map_json;
    ir_gc_map_file.close();
    bool uptodate = ir_gc_map_json["uptodate"];
    if(!uptodate) {
        std::cerr << "IR GC map not uptodate, throwing NotCompiledError" << std::endl;
        throw compiler::NotCompiledError(gc_source_path.string());
    }
    auto prev_path = this->file_path;
    auto gc_source = readFileToString(gc_source_path.string());
    Lexer lexer(gc_source);
    // Parser
    parser::Parser parsr(std::make_shared<Lexer>(lexer));
    auto program = parsr.parseProgram();
    for(auto& err : parsr.errors) {
        err->raise(false);
    }
    if(parsr.errors.size() > 0) {
        std::cerr << "Parser errors found, returning" << std::endl;
        return;
    }
    for(auto& stmt : program->statements) {
        switch(stmt->type()) {
        case AST::NodeType::FunctionStatement: {
            this->_importFunctionDeclarationStatement(std::static_pointer_cast<AST::FunctionStatement>(stmt), module, ir_gc_map_json);
            break;
        }
        case AST::NodeType::StructStatement: {
            this->_importStructStatement(std::static_pointer_cast<AST::StructStatement>(stmt), module, ir_gc_map_json);
            break;
        }
        case AST::NodeType::ImportStatement: {
            this->file_path = gc_source_path;
            this->_visitImportStatement(std::static_pointer_cast<AST::ImportStatement>(stmt), module);
            this->file_path = prev_path;
            break;
        }
        default:
            std::cerr << "Unknown statement type, skipping" << std::endl;
            break;
        }
    }
}

void compiler::Compiler::_importFunctionDeclarationStatement(std::shared_ptr<AST::FunctionStatement> function_declaration_statement, std::shared_ptr<enviornment::RecordModule> module,
                                                             nlohmann::json& ir_gc_map_json) {
    auto name = std::static_pointer_cast<AST::IdentifierLiteral>(function_declaration_statement->name)->value;
    auto params = function_declaration_statement->parameters;
    std::vector<std::string> param_names;
    std::vector<llvm::Type*> param_types;
    std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructInstance>>> arguments;

    for(auto param : params) {
        param_names.push_back(std::static_pointer_cast<AST::IdentifierLiteral>(param->name)->value);
        param_types.push_back(this->_parseType(param->value_type)->struct_type->stand_alone_type ? this->_parseType(param->value_type)->struct_type->stand_alone_type
                                                                                                 : llvm::PointerType::get(this->_parseType(param->value_type)->struct_type->struct_type, 0));
        arguments.push_back({std::static_pointer_cast<AST::IdentifierLiteral>(param->name)->value, this->_parseType(param->value_type)});
    }

    auto return_type = this->_parseType(function_declaration_statement->return_type);
    auto llvm_return_type = return_type->struct_type->stand_alone_type ? return_type->struct_type->stand_alone_type : return_type->struct_type->struct_type->getPointerTo();
    auto func_type = llvm::FunctionType::get(llvm_return_type, param_types, false);
    auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, ir_gc_map_json["functions"][name].get<std::string>(), this->llvm_module.get());
    unsigned idx = 0;
    for(auto& arg : func->args()) {
        arg.setName(param_names[idx++]);
    }

    auto func_record = std::make_shared<enviornment::RecordFunction>(name, func, func_type, arguments, return_type);
    module->record_map.push_back({func_record->name, func_record});
}

void compiler::Compiler::_importStructStatement(std::shared_ptr<AST::StructStatement> struct_statement, std::shared_ptr<enviornment::RecordModule> module, nlohmann::json& ir_gc_map_json) {
    std::string struct_name = std::static_pointer_cast<AST::IdentifierLiteral>(struct_statement->name)->value;
    std::vector<llvm::Type*> field_types;
    auto fields = struct_statement->fields;
    auto struct_record = std::make_shared<enviornment::RecordStructType>(struct_name);

    for(auto field : fields) {
        if(field->type() == AST::NodeType::VariableDeclarationStatement) {
            auto field_decl = std::static_pointer_cast<AST::VariableDeclarationStatement>(field);
            std::string field_name = std::static_pointer_cast<AST::IdentifierLiteral>(field_decl->name)->value;
            struct_record->fields.push_back(field_name);
            auto field_type = this->_parseType(field_decl->value_type);
            if(field_type->struct_type->stand_alone_type == nullptr) {
                field_types.push_back(field_type->struct_type->struct_type);
            } else {
                field_types.push_back(field_type->struct_type->stand_alone_type);
            }
            struct_record->sub_types[field_name] = field_type;
        } else if(field->type() == AST::NodeType::FunctionStatement) {
            auto field_decl = std::static_pointer_cast<AST::FunctionStatement>(field);
            auto method_name = std::static_pointer_cast<AST::IdentifierLiteral>(field_decl->name)->value;
            auto method_params = field_decl->parameters;
            std::vector<std::string> param_names;
            std::vector<llvm::Type*> param_types;
            std::vector<std::shared_ptr<enviornment::RecordStructInstance>> param_inst_records;
            std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructInstance>>> arguments;

            for(auto param : method_params) {
                param_names.push_back(std::static_pointer_cast<AST::IdentifierLiteral>(param->name)->value);
                param_inst_records.push_back(this->_parseType(param->value_type));
                param_types.push_back(param_inst_records.back()->struct_type->stand_alone_type ? param_inst_records.back()->struct_type->stand_alone_type
                                                                                               : llvm::PointerType::get(param_inst_records.back()->struct_type->struct_type, 0));
                arguments.push_back({std::static_pointer_cast<AST::IdentifierLiteral>(param->name)->value, this->_parseType(param->value_type)});
            }

            auto return_type = this->_parseType(field_decl->return_type);
            auto llvm_return_type = return_type->struct_type->stand_alone_type ? return_type->struct_type->stand_alone_type : return_type->struct_type->struct_type->getPointerTo();
            auto func_type = llvm::FunctionType::get(llvm_return_type, param_types, false);
            auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, struct_name + "::" + method_name, this->llvm_module.get());

            unsigned idx = 0;
            for(auto& arg : func->args()) {
                arg.setName(param_names[idx++]);
            }

            auto func_record = std::make_shared<enviornment::RecordFunction>(method_name, func, func_type, arguments, return_type);
            struct_record->methods.push_back({method_name, func_record});
        }
    }

    auto struct_type = llvm::StructType::create(this->llvm_context, field_types, ir_gc_map_json["structs"][struct_name].get<std::string>());
    struct_type->setBody(field_types);
    struct_record->struct_type = struct_type;

    module->record_map.push_back({struct_record->name, struct_record});
}
