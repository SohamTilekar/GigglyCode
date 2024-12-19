// TODO: Add Meta Data to all of the Record.
#include "compiler.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/ADT/APInt.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/Linker/Linker.h>
#include <llvm/Support/Alignment.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/SourceMgr.h>
#include <llvm/Support/raw_ostream.h>
#include <memory>
#include <regex.h>
#include <string>
#include <tuple>
#include <unordered_map>
#include <vector>

#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include "enviornment/enviornment.hpp"

using json = nlohmann::json;

compiler::Compiler::Compiler(const std::string& source, std::filesystem::path file_path, std::filesystem::path ir_gc_map, std::filesystem::path buildDir, std::filesystem::path relativePath)
    : llvm_context(llvm::LLVMContext()), llvm_ir_builder(llvm_context), source(source), file_path(file_path), ir_gc_map(ir_gc_map), buildDir(buildDir), relativePath(relativePath) {

    // Convert file path to string
    std::string path_str = file_path.string();

    // Find the position of "src" in the path
    size_t pos = path_str.rfind("src");
    // If "src" is found, move past it; otherwise, start from the beginning
    pos = (pos != std::string::npos) ? pos + 4 : 0;

    // Extract the substring from the position
    this->fc_st_name_prefix = path_str.substr(pos);

    // Replace '/' and '\\' with ".." in the prefix
    for (char delimiter : {'/', '\\'}) {
        pos = 0;
        while ((pos = this->fc_st_name_prefix.find(delimiter, pos)) != std::string::npos) {
            this->fc_st_name_prefix.replace(pos, 1, "..");
            pos += 2;
        }
    }

    // Create a new LLVM module with the modified prefix
    this->llvm_module = std::make_unique<llvm::Module>(this->fc_st_name_prefix, llvm_context);
    this->fc_st_name_prefix += "..";
    this->llvm_module->setSourceFileName(file_path.string());

    // Initialize the environment with an empty parent
    this->enviornment =
        std::make_shared<enviornment::Enviornment>(std::make_shared<enviornment::Enviornment>(nullptr, std::vector<std::tuple<std::string, std::shared_ptr<enviornment::Record>>>(), "buildtins"),
                                                   std::vector<std::tuple<std::string, std::shared_ptr<enviornment::Record>>>());

    // Initialize built-in functions and types
    this->_initializeBuiltins();

    // Initialize JSON structure for IR-GC map
    this->ir_gc_map_json["functions"] = json::object();
    this->ir_gc_map_json["structs"] = json::object();
    this->ir_gc_map_json["GSinstance"] = json::array();
    this->ir_gc_map_json["GFinstance"] = json::array();
}

void addBuiltinType(compiler::Compiler* compiler, const std::string& name, llvm::Type* type) {
    auto record = std::make_shared<enviornment::RecordStructType>(name, type);
    compiler->enviornment->parent->addRecord(record);
}

void addBuiltinFunction(compiler::Compiler* compiler, const std::string& name, llvm::FunctionType* funcType,
                        const std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructType>, bool>>& params, std::shared_ptr<enviornment::RecordStructType> returnType) {
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, compiler->llvm_module.get());
    compiler->enviornment->parent->addRecord(std::make_shared<enviornment::RecordFunction>(name, func, funcType, params, returnType, true));
}

void compiler::Compiler::_initializeBuiltins() {
    this->GC_pointer = llvm::PointerType::get(llvm_context, 0);
    addBuiltinType(this, "int", llvm::Type::getInt64Ty(llvm_context));
    this->GC_int = this->enviornment->parent->getStruct("int")->stand_alone_type;
    addBuiltinType(this, "int32", llvm::Type::getInt32Ty(llvm_context));
    this->GC_int32 = this->enviornment->parent->getStruct("int32")->stand_alone_type;
    addBuiltinType(this, "uint", llvm::Type::getInt64Ty(llvm_context));
    addBuiltinType(this, "uint32", llvm::Type::getInt32Ty(llvm_context));
    addBuiltinType(this, "float", llvm::Type::getDoubleTy(llvm_context));
    addBuiltinType(this, "float32", llvm::Type::getFloatTy(llvm_context));
    addBuiltinType(this, "char", llvm::Type::getInt8Ty(llvm_context));
    addBuiltinType(this, "str", llvm::PointerType::get(llvm::Type::getInt8Ty(llvm_context), 0));
    addBuiltinType(this, "void", llvm::Type::getVoidTy(llvm_context));
    this->GC_void = this->enviornment->parent->getStruct("void")->stand_alone_type;
    addBuiltinType(this, "bool", llvm::Type::getInt1Ty(llvm_context));
    addBuiltinType(this, "array", llvm::PointerType::get(llvm_context, 0));
    this->GC_shared_ptr = llvm::StructType::create(this->llvm_context, {/*Value: */ this->GC_pointer, /*RC: */ this->GC_pointer}, "shared_ptr");

    auto _any = std::make_shared<enviornment::RecordModule>("Any");
    this->enviornment->parent->addRecord(_any);

    addBuiltinFunction(this, "puts", llvm::FunctionType::get(this->enviornment->parent->getStruct("void")->stand_alone_type, this->enviornment->parent->getStruct("str")->stand_alone_type, false),
                       {{"string", this->enviornment->parent->getStruct("str"), false}}, this->enviornment->parent->getStruct("void"));
    addBuiltinFunction(this, "printf", llvm::FunctionType::get(this->enviornment->parent->getStruct("int")->stand_alone_type, {this->enviornment->parent->getStruct("str")->stand_alone_type}, true),
                       {{"format", this->enviornment->parent->getStruct("str"), false}}, this->enviornment->parent->getStruct("int"));
    addBuiltinFunction(this, "scanf", llvm::FunctionType::get(this->enviornment->parent->getStruct("int")->stand_alone_type, {this->enviornment->parent->getStruct("str")->stand_alone_type}, true),
                       {{"format", this->enviornment->parent->getStruct("str"), false}}, this->enviornment->parent->getStruct("int"));
    addBuiltinFunction(this, "malloc", llvm::FunctionType::get(llvm::PointerType::get(llvm_context, 0), {this->enviornment->parent->getStruct("int")->stand_alone_type}, true),
                       {{"bits", this->enviornment->parent->getStruct("int"), false}}, this->enviornment->parent->getStruct("void"));
    addBuiltinFunction(this, "free", llvm::FunctionType::get(this->enviornment->parent->getStruct("void")->stand_alone_type, {llvm::PointerType::get(llvm_context, 0)}, true), {},
                       this->enviornment->parent->getStruct("void"));
}

void compiler::Compiler::_incrementRC(llvm::Value* value) {
    auto RC_ptr = this->llvm_ir_builder.CreateLoad(this->GC_pointer, this->llvm_ir_builder.CreateStructGEP(this->GC_shared_ptr, value, 1));
    auto is_null = this->llvm_ir_builder.CreateIsNotNull(RC_ptr);
    auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* increment = llvm::BasicBlock::Create(this->llvm_context, "increment", func);
    llvm::BasicBlock* conti = llvm::BasicBlock::Create(this->llvm_context, "conti", func);
    this->llvm_ir_builder.CreateCondBr(is_null, increment, conti);
    this->llvm_ir_builder.SetInsertPoint(increment);
    auto RC_val = this->llvm_ir_builder.CreateLoad(this->GC_int, RC_ptr);
    auto incremented = this->llvm_ir_builder.CreateAdd(RC_val, llvm::ConstantInt::get(this->GC_int, 1));
    this->llvm_ir_builder.CreateStore(incremented, RC_ptr);
    this->llvm_ir_builder.CreateBr(conti);
    this->llvm_ir_builder.SetInsertPoint(conti);
};

void compiler::Compiler::_decrementRC(llvm::Value* value) {
    auto RC_ptr = this->llvm_ir_builder.CreateLoad(this->GC_pointer, this->llvm_ir_builder.CreateStructGEP(this->GC_shared_ptr, value, 1));
    auto RC_value = this->llvm_ir_builder.CreateLoad(this->GC_int, RC_ptr);
    auto is_null = this->llvm_ir_builder.CreateIsNotNull(RC_ptr);
    auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* decrement = llvm::BasicBlock::Create(this->llvm_context, "decrement", func);
    llvm::BasicBlock* FreeBB = llvm::BasicBlock::Create(llvm_context, "free", func);
    llvm::BasicBlock* conti = llvm::BasicBlock::Create(this->llvm_context, "conti", func);
    this->llvm_ir_builder.CreateCondBr(is_null, decrement, conti);
    this->llvm_ir_builder.SetInsertPoint(decrement);
    auto RC_val = this->llvm_ir_builder.CreateLoad(this->GC_int, RC_ptr);
    auto decremented = this->llvm_ir_builder.CreateSub(RC_val, llvm::ConstantInt::get(this->GC_int, 1));
    this->llvm_ir_builder.CreateStore(decremented, RC_ptr);
    auto is_zero = this->llvm_ir_builder.CreateICmpEQ(decremented, llvm::ConstantInt::get(this->GC_int, 0));
    this->llvm_ir_builder.CreateCondBr(is_zero, FreeBB, conti);
    this->llvm_ir_builder.SetInsertPoint(FreeBB);
    this->llvm_ir_builder.CreateCall(this->enviornment->getFunction("free", {this->enviornment->getStruct("int")})->function,
                                     {this->llvm_ir_builder.CreateLoad(this->GC_pointer, this->llvm_ir_builder.CreateStructGEP(this->GC_shared_ptr, value, 0))});
    this->llvm_ir_builder.CreateCall(this->enviornment->getFunction("free", {this->enviornment->getStruct("int")})->function, {RC_ptr});
    this->llvm_ir_builder.CreateBr(conti);
    this->llvm_ir_builder.SetInsertPoint(conti);
};

void compiler::Compiler::compile(std::shared_ptr<AST::Node> node) {
    switch (node->type()) {
        case AST::NodeType::Program:
            this->_visitProgram(std::static_pointer_cast<AST::Program>(node));
            break;
        case AST::NodeType::ExpressionStatement:
            this->_visitExpressionStatement(std::static_pointer_cast<AST::ExpressionStatement>(node));
            break;
        case AST::NodeType::InfixedExpression:
            this->_visitInfixExpression(std::static_pointer_cast<AST::InfixExpression>(node));
            break;
        case AST::NodeType::IndexExpression:
            this->_visitIndexExpression(std::static_pointer_cast<AST::IndexExpression>(node));
            break;
        case AST::NodeType::VariableDeclarationStatement:
            this->_visitVariableDeclarationStatement(std::static_pointer_cast<AST::VariableDeclarationStatement>(node));
            break;
        case AST::NodeType::VariableAssignmentStatement:
            this->_visitVariableAssignmentStatement(std::static_pointer_cast<AST::VariableAssignmentStatement>(node));
            break;
        case AST::NodeType::IfElseStatement:
            this->_visitIfElseStatement(std::static_pointer_cast<AST::IfElseStatement>(node));
            break;
        case AST::NodeType::FunctionStatement:
            this->_visitFunctionDeclarationStatement(std::static_pointer_cast<AST::FunctionStatement>(node));
            break;
        case AST::NodeType::CallExpression:
            this->_visitCallExpression(std::static_pointer_cast<AST::CallExpression>(node));
            break;
        case AST::NodeType::ReturnStatement:
            this->_visitReturnStatement(std::static_pointer_cast<AST::ReturnStatement>(node));
            break;
        case AST::NodeType::RaiseStatement:
            this->_visitRaiseStatement(std::static_pointer_cast<AST::RaiseStatement>(node));
            break;
        case AST::NodeType::TryCatchStatement:
            this->_visitTryCatchStatement(std::static_pointer_cast<AST::TryCatchStatement>(node));
            break;
        case AST::NodeType::BlockStatement:
            return this->_visitBlockStatement(std::static_pointer_cast<AST::BlockStatement>(node));
            break;
        case AST::NodeType::WhileStatement:
            return this->_visitWhileStatement(std::static_pointer_cast<AST::WhileStatement>(node));
            break;
        case AST::NodeType::ForStatement:
            return this->_visitForStatement(std::static_pointer_cast<AST::ForStatement>(node));
            break;
        case AST::NodeType::BreakStatement:
            if (this->enviornment->loop_conti_block.empty()) {
                errors::NodeOutside("Break outside loop", this->source, *node, errors::outsideNodeType::Break, "Break statement outside the Loop", "Remove the Break statement, it is not necessary")
                    .raise();
                exit(1);
            }
            {
                auto f_node = std::static_pointer_cast<AST::BreakStatement>(node);
                if (f_node->loopIdx >= this->enviornment->loop_ifbreak_block.size()) {
                    errors::CompletionError("WrongLoopIdx", this->source, f_node->meta_data.st_line_no, f_node->meta_data.end_line_no, "Loop Index is out of range", "Remember: LoopIdx start with `0`").raise();
                }
                if (this->enviornment->loop_ifbreak_block.at(this->enviornment->loop_ifbreak_block.size() - f_node->loopIdx - 1))
                    this->llvm_ir_builder.CreateBr(this->enviornment->loop_ifbreak_block.at(this->enviornment->loop_ifbreak_block.size() - f_node->loopIdx - 1));
                else
                    this->llvm_ir_builder.CreateBr(this->enviornment->loop_conti_block.at(this->enviornment->loop_conti_block.size() - f_node->loopIdx - 1));
            }
            throw compiler::DoneBr();
        case AST::NodeType::ContinueStatement:
            if (this->enviornment->loop_condition_block.empty()) {
                errors::NodeOutside("Continue outside loop", this->source, *node, errors::outsideNodeType::Continue, "Continue statement outside the Loop",
                                    "Remove the Continue statement, it is not necessary")
                    .raise();
                exit(1);
            }
            {
                auto f_node = std::static_pointer_cast<AST::ContinueStatement>(node);
                if (f_node->loopIdx >= this->enviornment->loop_ifbreak_block.size()) {
                    errors::CompletionError("WrongLoopIdx", this->source, f_node->meta_data.st_line_no, f_node->meta_data.end_line_no, "Loop Index is out of range", "Remember: LoopIdx start with `0`").raise();
                }
                this->llvm_ir_builder.CreateBr(this->enviornment->loop_condition_block.at(this->enviornment->loop_condition_block.size() - f_node->loopIdx - 1));
            }
            throw compiler::DoneBr();
        case AST::NodeType::StructStatement:
            this->_visitStructStatement(std::static_pointer_cast<AST::StructStatement>(node));
            break;
        case AST::NodeType::ImportStatement:
            this->_visitImportStatement(std::static_pointer_cast<AST::ImportStatement>(node));
            break;
        default:
            errors::CompletionError("Unknown node type", this->source, node->meta_data.st_line_no, node->meta_data.end_line_no, "Unknown node type: " + AST::nodeTypeToString(node->type())).raise();
            break;
    }
}

void compiler::Compiler::_visitProgram(std::shared_ptr<AST::Program> program) {
    for (const auto& stmt : program->statements) {
        this->compile(stmt);
    }
}

void compiler::Compiler::_visitExpressionStatement(std::shared_ptr<AST::ExpressionStatement> expression_statement) { this->compile(expression_statement->expr); }

void compiler::Compiler::_visitBlockStatement(std::shared_ptr<AST::BlockStatement> block_statement) {
    for (const auto& stmt : block_statement->statements) {
        this->compile(stmt);
    }
}

void compiler::Compiler::_checkCallType(std::shared_ptr<enviornment::RecordFunction> func_record, std::shared_ptr<AST::CallExpression> func_call, std::vector<llvm::Value*>& args,
                                        const std::vector<std::shared_ptr<enviornment::RecordStructType>>& params_types) {

    unsigned short idx = 0;
    std::vector<unsigned short> mismatches;

    for (const auto& [pt, pst] : llvm::zip(func_record->arguments, params_types)) {
        auto expected_type = std::get<1>(pt);
        if (enviornment::_checkType(expected_type, pst)) {
        } else if (this->canConvertType(pst, expected_type)) {
            args[idx] = this->convertType({args[idx], nullptr, pst}, expected_type).value;
        } else {
            mismatches.push_back(idx);
        }
        idx++;
    }
    if (!mismatches.empty()) {
        errors::NoOverload(this->source, {mismatches}, func_call, "Cannot call the function with wrong type").raise();
    }
}


compiler::Compiler::ResolvedValue compiler::Compiler::_CallGfunc(std::vector<std::shared_ptr<enviornment::RecordGenericFunction>> gfuncs, std::shared_ptr<AST::CallExpression> func_call,
                                                                 std::string name, std::vector<llvm::Value*> args, const std::vector<std::shared_ptr<enviornment::RecordStructType>> params_types) {

    for (const auto& gfunc : gfuncs) {
        if (gfunc->env->isFunction(name, params_types, false, true)) {
            auto func_record = gfunc->env->getFunction(name, params_types, false, true);
            this->_checkCallType(func_record, func_call, args, params_types);
            auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, args, name + "_result");
            return {returnValue, nullptr, func_record->return_type, compiler::resolveType::StructInst};
        }
    }

    auto prev_env = this->enviornment;
    std::vector<std::vector<unsigned short>> mismatches;

    for (const auto& gfunc : gfuncs) {
        this->enviornment = std::make_shared<enviornment::Enviornment>(prev_env, std::vector<std::tuple<std::string, std::shared_ptr<enviornment::Record>>>{}, name);
        unsigned short param_idx = 0;
        std::vector<unsigned short> mismatch;

        for (const auto& [gparam, pparam] : llvm::zip(gfunc->func->parameters, params_types)) {
            auto gparam_type = this->_resolveValue(gparam->value_type->name).type;
            auto gparam_variant = std::get<std::shared_ptr<enviornment::RecordStructType>>(this->_resolveValue(gparam->value_type->name).variant);

            if (gparam->value_type->name->type() == AST::NodeType::IdentifierLiteral) {
                auto struc = std::make_shared<enviornment::RecordStructType>(*pparam);
                struc->name = std::static_pointer_cast<AST::IdentifierLiteral>(gparam->value_type->name)->value;
                this->enviornment->addRecord(struc);
            } else if (gparam_type == compiler::resolveType::StructType && enviornment::_checkType(gparam_variant, pparam)) {
                // Do nothing
            } else if (gparam_type == compiler::resolveType::StructType && this->canConvertType(gparam_variant, pparam)) {
                // Do nothing
            } else {
                mismatch.push_back(param_idx);
            }
            param_idx++;
        }

        if (!mismatch.empty()) {
            mismatches.push_back(mismatch);
            continue;
        }

        auto body = gfunc->func->body;
        auto params = gfunc->func->parameters;
        std::vector<std::string> param_names;
        std::vector<llvm::Type*> param_types;
        std::vector<std::shared_ptr<enviornment::RecordStructType>> param_inst_records;
        std::vector<bool> param_refrences;

        for (const auto& [param, param_type] : llvm::zip(params, params_types)) {
            auto param_name_str = std::static_pointer_cast<AST::IdentifierLiteral>(param->name)->value;
            param_names.push_back(param_name_str);
            param_inst_records.push_back(param_type);
            param_refrences.push_back(param->value_type->refrence);
            param_types.push_back(param_type->struct_type || name == "array" ? this->GC_shared_ptr : param_type->stand_alone_type);
        }

        auto return_type = this->_parseType(gfunc->func->return_type);
        auto llvm_return_type = return_type->struct_type || return_type->name == "array" ? this->GC_shared_ptr : return_type->stand_alone_type;
        auto func_type = llvm::FunctionType::get(llvm_return_type, param_types, false);
        auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, this->fc_st_name_prefix != "main.gc.." ? this->fc_st_name_prefix + name : name, this->llvm_module.get());
        this->ir_gc_map_json["functions"][name] = func->getName().str();

        unsigned short idx = 0;
        for (auto& arg : func->args()) {
            arg.setName(param_names[idx++]);
        }

        std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructType>, bool>> arguments;
        auto func_record = std::make_shared<enviornment::RecordFunction>(name, func, func_type, arguments, return_type, gfunc->func->extra_info);
        func_record->env = this->enviornment;

        if (body) {
            auto bb = llvm::BasicBlock::Create(this->llvm_context, "entry", func);
            this->function_entery_block.push_back(bb);
            this->llvm_ir_builder.SetInsertPoint(bb);
            this->enviornment->current_function = func_record.get();

            for (const auto& [arg, param_type_record, refrence] : llvm::zip(func->args(), param_inst_records, param_refrences)) {
                llvm::Value* alloca = &arg;
                if (!arg.getType()->isPointerTy() || enviornment::_checkType(param_type_record, this->enviornment->getStruct("array"))) {
                    alloca = this->llvm_ir_builder.CreateAlloca(arg.getType(), nullptr, arg.getName() + "_og");
                    this->llvm_ir_builder.CreateStore(&arg, alloca);
                }
                auto arg_type = param_type_record->stand_alone_type ? param_type_record->stand_alone_type : param_type_record->stand_alone_type;
                llvm::Value* arg_value;
                if (refrence)
                    arg_value = this->llvm_ir_builder.CreateLoad(arg_type, &arg, "loded_" + arg.getName());
                auto record = std::make_shared<enviornment::RecordVariable>(std::string(arg.getName()), arg_value, alloca, param_type_record);
                func_record->arguments.push_back({arg.getName().str(), param_type_record, refrence});
                this->enviornment->addRecord(record);
            }

            func_record->set_meta_data(gfunc->func->meta_data.st_line_no, gfunc->func->meta_data.st_col_no, gfunc->func->meta_data.end_line_no, gfunc->func->meta_data.end_col_no);
            this->enviornment->addRecord(func_record);

            try {
                this->compile(body);
            } catch (compiler::DoneRet) {
                // Ignoring
            } catch (compiler::DoneBr) {
                // Ignore to compile the following commands if DoneRaise exception occurs
            }

            gfunc->env->addRecord(func_record);
            this->function_entery_block.pop_back();

            if (!this->function_entery_block.empty()) {
                this->llvm_ir_builder.SetInsertPoint(this->function_entery_block.back());
            }
        } else {
            for (const auto& [arg, param_type_record, param_refrence] : llvm::zip(func->args(), param_inst_records, param_refrences)) {
                func_record->arguments.push_back({arg.getName().str(), param_type_record, param_refrence});
            }
            gfunc->env->addRecord(func_record);
        }

        this->_checkCallType(func_record, func_call, args, params_types);

        for (const auto& [gparam, pparam] : llvm::zip(gfunc->func->parameters, params_types)) {
            if (gparam->value_type->name->type() == AST::NodeType::IdentifierLiteral) {
                auto name = std::static_pointer_cast<AST::IdentifierLiteral>(gparam->value_type->name)->value;
                if (this->enviornment->isStruct(name)) {
                    auto _struct = this->enviornment->getStruct(name);
                    _struct->name = pparam->name;
                }
            }
        }
        auto returnValue = this->llvm_ir_builder.CreateCall(func, args, name + "_result");
        this->enviornment = prev_env;
        return {returnValue, nullptr, func_record->return_type, compiler::resolveType::StructInst};
    }

    if (mismatches.empty()) {
        errors::NoOverload(this->source, mismatches, func_call, "Function does not exist.", "Check the function name or define the function.").raise();
    } else {
        errors::NoOverload(this->source, mismatches, func_call, "Argument types do not match any overload.", "Check the argument types or define an appropriate overload.").raise();
    }

    exit(1);
};


compiler::Compiler::ResolvedValue compiler::Compiler::_CallGstruct(std::vector<std::shared_ptr<enviornment::RecordGenericStructType>> gstructs, std::shared_ptr<AST::CallExpression> func_call,
                                                                   std::string name, std::vector<llvm::Value*> args, std::vector<std::shared_ptr<enviornment::RecordStructType>> params_types) {
    auto prev_env = this->enviornment;

    for (auto gstruct : gstructs) {
        if (params_types.size() < gstruct->structAST->generics.size()) {
            continue;
        }

        this->enviornment = std::make_shared<enviornment::Enviornment>(gstruct->env);
        std::vector<std::shared_ptr<enviornment::RecordStructType>> generics;
        std::vector<std::string> generic_names;
        std::vector<llvm::Value*> remaining_args(args);
        std::vector<std::shared_ptr<enviornment::RecordStructType>> remaining_params_types(params_types);

        for (auto [pt, generic] : llvm::zip(params_types, gstruct->structAST->generics)) {
            pt = std::make_shared<enviornment::RecordStructType>(*pt);
            generic_names.push_back(pt->name);
            pt->name = std::static_pointer_cast<AST::IdentifierLiteral>(generic->name)->value;
            this->enviornment->addRecord(pt);
            generics.push_back(pt);
            remaining_args.erase(remaining_args.begin());
            remaining_params_types.erase(remaining_params_types.begin());
        }

        std::string struct_name = std::static_pointer_cast<AST::IdentifierLiteral>(gstruct->structAST->name)->value;
        auto struct_record = std::make_shared<enviornment::RecordStructType>(struct_name);

        if (!gstruct->env->isStruct(struct_name, false, generics)) {
            std::vector<llvm::Type*> field_types;
            auto fields = gstruct->structAST->fields;
            this->enviornment->addRecord(struct_record);

            for (auto field : fields) {
                if (field->type() == AST::NodeType::VariableDeclarationStatement) {
                    auto field_decl = std::static_pointer_cast<AST::VariableDeclarationStatement>(field);
                    std::string field_name = std::static_pointer_cast<AST::IdentifierLiteral>(field_decl->name)->value;
                    // struct_record->fields.push_back(field_name);
                    auto field_type = this->_parseType(field_decl->value_type);
                    field_types.push_back(field_type->stand_alone_type || field_type->name == "array" ? this->GC_shared_ptr : field_type->stand_alone_type);
                    struct_record->addSubType(field_name, field_type);

                    if (field_decl->value_type->type() == AST::NodeType::IdentifierLiteral) {
                        auto field_value = std::static_pointer_cast<AST::IdentifierLiteral>(field_decl->value_type->name)->value;
                        bool is_generic = false;
                        for (const auto& generic : gstruct->structAST->generics) {
                            if (field_value == std::static_pointer_cast<AST::IdentifierLiteral>(generic->name)->value) {
                                is_generic = true;
                                break;
                            }
                        }

                        if (is_generic) {
                            struct_record->generic_sub_types.push_back(field_type);
                        }
                    }

                    auto struct_type = llvm::StructType::create(this->llvm_context, field_types, this->fc_st_name_prefix + struct_name);
                    struct_type->setBody(field_types);
                    struct_record->struct_type = struct_type;
                } else if (field->type() == AST::NodeType::FunctionStatement) {
                    auto func_dec = std::static_pointer_cast<AST::FunctionStatement>(field);
                    if (func_dec->generic.size() != 0) {
                        continue;
                    }
                    this->_visitFunctionDeclarationStatement(func_dec, struct_record);
                }
            }
        } else {
            struct_record = gstruct->env->getStruct(struct_name, false, generics);
        }

        auto struct_type = struct_record->struct_type;
        llvm::Value* alloca;
        if (func_call->_new) {
            llvm::Value* gep = this->llvm_ir_builder.CreateGEP(struct_type, llvm::ConstantPointerNull::get(struct_type->getPointerTo()),
                                                               llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_context), 1), struct_name + "_size_get");
            llvm::Value* size = this->llvm_ir_builder.CreatePtrToInt(gep, this->GC_int, struct_name + "_size");
            alloca = this->llvm_ir_builder.CreateCall(this->enviornment->getFunction("malloc", {this->enviornment->getStruct("int")})->function, {size});
        } else
            alloca = this->llvm_ir_builder.CreateAlloca(struct_type, nullptr, name);
        remaining_params_types.insert(remaining_params_types.begin(), struct_record);
        remaining_args.insert(remaining_args.begin(), alloca);
        auto func = struct_record->get_method("__init__", remaining_params_types);

        if (func) {
            this->_checkCallType(func, func_call, args, params_types);
            this->llvm_ir_builder.CreateCall(func->function, remaining_args, struct_name + "initilizer");
        } else {
            errors::NoOverload(this->source, {}, func_call, "Initialization method does not exist for struct " + struct_name + ".", "Check the initialization method name or define the method.")
                .raise();
            exit(1);
        }

        gstruct->env->addRecord(struct_record);
        this->enviornment = prev_env;
        return {alloca, alloca, struct_record, compiler::resolveType::StructInst};
    }

    errors::NoOverload(this->source, {}, func_call, "Struct overload does not exist.", "Check the argument types or define an appropriate overload, first pass types & then init function params.")
        .raise();
    exit(1);
};


compiler::Compiler::ResolvedValue compiler::Compiler::convertType(std::tuple<llvm::Value*, llvm::Value*, std::shared_ptr<enviornment::RecordStructType>> of,
                                                                  std::shared_ptr<enviornment::RecordStructType> to) {
    auto [ofloadedval, ofalloca, oftype] = of;

    if (enviornment::_checkType(oftype, to)) {
        return {ofloadedval, ofalloca, oftype, compiler::resolveType::StructInst};
    }

    if (oftype->name == "int32" && to->name == "int") {
        auto int_type = this->llvm_ir_builder.CreateSExt(ofloadedval, this->enviornment->getStruct("int")->stand_alone_type, "int32_to_int");
        return {int_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "int" && to->name == "int32") {
        auto int32_type = this->llvm_ir_builder.CreateTrunc(ofloadedval, this->enviornment->getStruct("int32")->stand_alone_type, "int_to_int32");
        return {int32_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "uint32" && to->name == "uint") {
        auto uint_type = this->llvm_ir_builder.CreateZExt(ofloadedval, this->enviornment->getStruct("uint")->stand_alone_type, "uint32_to_uint");
        return {uint_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "uint" && to->name == "uint32") {
        auto uint32_type = this->llvm_ir_builder.CreateTrunc(ofloadedval, this->enviornment->getStruct("uint32")->stand_alone_type, "uint_to_uint32");
        return {uint32_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "float32" && to->name == "float") {
        auto float_type = this->llvm_ir_builder.CreateFPExt(ofloadedval, this->enviornment->getStruct("float")->stand_alone_type, "float32_to_float");
        return {float_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "float" && to->name == "float32") {
        auto float32_type = this->llvm_ir_builder.CreateFPTrunc(ofloadedval, this->enviornment->getStruct("float32")->stand_alone_type, "float_to_float32");
        return {float32_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "int" && to->name == "float") {
        auto float_type = this->llvm_ir_builder.CreateSIToFP(ofloadedval, this->enviornment->getStruct("float")->stand_alone_type, "int_to_float");
        return {float_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "int32" && to->name == "float32") {
        auto float32_type = this->llvm_ir_builder.CreateSIToFP(ofloadedval, this->enviornment->getStruct("float32")->stand_alone_type, "int32_to_float32");
        return {float32_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "uint" && to->name == "float") {
        auto float_type = this->llvm_ir_builder.CreateUIToFP(ofloadedval, this->enviornment->getStruct("float")->stand_alone_type, "uint_to_float");
        return {float_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "uint32" && to->name == "float32") {
        auto float32_type = this->llvm_ir_builder.CreateUIToFP(ofloadedval, this->enviornment->getStruct("float32")->stand_alone_type, "uint32_to_float32");
        return {float32_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "float" && to->name == "int") {
        auto int_type = this->llvm_ir_builder.CreateFPToSI(ofloadedval, this->enviornment->getStruct("int")->stand_alone_type, "float_to_int");
        return {int_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "float32" && to->name == "int32") {
        auto int32_type = this->llvm_ir_builder.CreateFPToSI(ofloadedval, this->enviornment->getStruct("int32")->stand_alone_type, "float32_to_int32");
        return {int32_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "float" && to->name == "uint") {
        auto uint_type = this->llvm_ir_builder.CreateFPToUI(ofloadedval, this->enviornment->getStruct("uint")->stand_alone_type, "float_to_uint");
        return {uint_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "float32" && to->name == "uint32") {
        auto uint32_type = this->llvm_ir_builder.CreateFPToUI(ofloadedval, this->enviornment->getStruct("uint32")->stand_alone_type, "float32_to_uint32");
        return {uint32_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "bool" && to->name == "int") {
        auto int_type = this->llvm_ir_builder.CreateZExt(ofloadedval, this->enviornment->getStruct("int")->stand_alone_type, "bool_to_int");
        return {int_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "bool" && to->name == "uint") {
        auto uint_type = this->llvm_ir_builder.CreateZExt(ofloadedval, this->enviornment->getStruct("uint")->stand_alone_type, "bool_to_uint");
        return {uint_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "bool" && to->name == "float") {
        auto float_type = this->llvm_ir_builder.CreateUIToFP(ofloadedval, this->enviornment->getStruct("float")->stand_alone_type, "bool_to_float");
        return {float_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->name == "bool" && to->name == "float32") {
        auto float32_type = this->llvm_ir_builder.CreateUIToFP(ofloadedval, this->enviornment->getStruct("float32")->stand_alone_type, "bool_to_float32");
        return {float32_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if ((oftype->name == "int" || oftype->name == "float" || oftype->name == "str") && to->name == "bool") {
        auto bool_type = this->llvm_ir_builder.CreateICmpNE(ofloadedval, llvm::Constant::getNullValue(ofloadedval->getType()), "to_bool");
        return {bool_type, ofalloca, to, compiler::resolveType::StructInst};
    } else if (oftype->struct_type) {
        if (oftype->is_method("", {oftype}, {{"autocast", true}}, to)) {
            auto method = oftype->get_method("", {oftype}, {{"autocast", true}}, to);
            auto returnValue = this->llvm_ir_builder.CreateCall(method->function, {ofalloca}, oftype->name + "_to_" + to->name);
            return {returnValue, nullptr, method->return_type, compiler::resolveType::StructInst};
        }
    }

    std::shared_ptr<enviornment::RecordModule> x = nullptr;
    return {nullptr, nullptr, x, compiler::resolveType::Module};
};

bool compiler::Compiler::canConvertType(std::shared_ptr<enviornment::RecordStructType> from, std::shared_ptr<enviornment::RecordStructType> to) {
    const std::vector<std::pair<std::string, std::string>> convertibleTypes = {
        {"int32", "int"},     {"int", "int32"},  {"uint32", "uint"},    {"uint", "uint32"},  {"float32", "float"}, {"float", "float32"}, {"int", "float"},
        {"int32", "float32"}, {"uint", "float"}, {"uint32", "float32"}, {"float", "int"},    {"float32", "int32"}, {"float", "uint"},    {"float32", "uint32"},
        {"bool", "int"},      {"bool", "uint"},  {"bool", "float"},     {"bool", "float32"}, {"int", "bool"},      {"float", "bool"},    {"str", "bool"}};

    for (const auto& [fromType, toType] : convertibleTypes) {
        if (from->name == fromType && to->name == toType) {
            return true;
        }
    }
    if (from->struct_type && from->is_method("", {from}, {{"autocast", true}}, to, true)) {
        return true;
    }
    return false;
};

bool compiler::Compiler::conversionPrecidence(std::shared_ptr<enviornment::RecordStructType> from, std::shared_ptr<enviornment::RecordStructType> to) {
    const std::vector<std::pair<std::string, std::string>> precedencePairs = {{"int32", "int"},      {"uint32", "int"},   {"uint", "int"},   {"uint32", "uint"}, {"float32", "float"},
                                                                              {"int32", "float"},    {"uint32", "float"}, {"uint", "float"}, {"int", "float"},   {"int32", "float32"},
                                                                              {"uint32", "float32"}, {"uint", "float32"}, {"int", "float32"}};

    for (const auto& pair : precedencePairs) {
        if (from->name == pair.first && to->name == pair.second) {
            return true;
        }
    }

    return false;
};


compiler::Compiler::ResolvedValue compiler::Compiler::_memberAccess(std::shared_ptr<AST::InfixExpression> infixed_expression) {
    auto left = infixed_expression->left;
    auto right = infixed_expression->right;
    auto [left_value, left_alloca, _left_type, ltt] = this->_resolveValue(left);
    if (right->type() == AST::NodeType::IdentifierLiteral) {
        if (ltt == compiler::resolveType::Module) {
            auto module = std::get<std::shared_ptr<enviornment::RecordModule>>(_left_type);
            auto name = std::static_pointer_cast<AST::IdentifierLiteral>(right)->value;
            if (module->is_module(name)) {
                return compiler::Compiler::ResolvedValue(nullptr, nullptr, module->get_module(name), compiler::resolveType::Module);
            } else if (module->is_struct(name)) {
                return compiler::Compiler::ResolvedValue(nullptr, nullptr, module->get_struct(name), compiler::resolveType::StructType);
            } else if (module->isGenericStruct(name)) {
                return compiler::Compiler::ResolvedValue(nullptr, nullptr, module->getGenericStruct(name), compiler::resolveType::GStructType);
            } else {
                errors::DosentContain(this->source, std::static_pointer_cast<AST::IdentifierLiteral>(right), left, "no member `" + name + "` not found in module " + module->name,
                                      "Check the Member Name in the module is correct")
                    .raise();
                exit(1);
            }
        } else if (ltt == compiler::resolveType::StructInst) {
            auto left_type = std::get<std::shared_ptr<enviornment::RecordStructType>>(_left_type);
            if (left_type->sub_types.contains(std::static_pointer_cast<AST::IdentifierLiteral>(right)->value)) {
                unsigned short idx = 0;
                for (auto field : left_type->getFields()) {
                    if (field == std::static_pointer_cast<AST::IdentifierLiteral>(right)->value) {
                        break;
                    }
                    idx++;
                }
                auto type = left_type->sub_types[std::static_pointer_cast<AST::IdentifierLiteral>(right)->value];
                llvm::Value* gep = this->llvm_ir_builder.CreateStructGEP(left_type->struct_type,
                                                                         this->llvm_ir_builder.CreateLoad(this->GC_pointer, this->llvm_ir_builder.CreateStructGEP(this->GC_shared_ptr, left_alloca, 0)),
                                                                         idx, "accesed" + std::static_pointer_cast<AST::IdentifierLiteral>(right)->value + "_from_" + left_type->name);
                if (type->struct_type || type->name == "array") {
                    return {gep, gep, type, compiler::resolveType::StructInst};
                }
                llvm::Value* loaded = this->llvm_ir_builder.CreateLoad(type->stand_alone_type, gep, "loded" + std::static_pointer_cast<AST::IdentifierLiteral>(right)->value);
                return {loaded, gep, type, compiler::resolveType::StructInst};
            } else {
                errors::DosentContain(this->source, std::static_pointer_cast<AST::IdentifierLiteral>(right), left,
                                      "no member `" + std::static_pointer_cast<AST::IdentifierLiteral>(right)->value + "` not found in struct " + left_type->name,
                                      "Check the Member Name in the struct is correct")
                    .raise();
                exit(1);
            }
        }
    } else if (right->type() == AST::NodeType::CallExpression) {
        auto call_expression = std::static_pointer_cast<AST::CallExpression>(right);
        auto name = std::static_pointer_cast<AST::IdentifierLiteral>(call_expression->name)->value;
        auto params = call_expression->arguments;
        std::vector<llvm::Value*> args;
        std::vector<llvm::Value*> arg_allocas;
        std::vector<std::shared_ptr<enviornment::RecordStructType>> params_types;
        for (auto arg : params) {
            auto [value, val_alloca, param_type, ptt] = this->_resolveValue(arg);
            if (ptt == compiler::resolveType::Module) {
                errors::WrongType(this->source, arg, {}, "Cant pass Module to the Function").raise();
                exit(1);
            } else if (ptt == compiler::resolveType::StructType || ptt == compiler::resolveType::GStructType) {
                errors::WrongType(this->source, arg, {}, "Cant pass type to the Function").raise();
                exit(1);
            }
            params_types.push_back(std::get<std::shared_ptr<enviornment::RecordStructType>>(param_type));
            arg_allocas.push_back(val_alloca);
            args.push_back(value);
        }
        if (ltt == compiler::resolveType::Module) {
            auto left_type = std::get<std::shared_ptr<enviornment::RecordModule>>(_left_type);
            if (left_type->isGenericFunc(name) ? left_type->isFunction(name, params_types, true) : left_type->isFunction(name, params_types)) {
                auto func = left_type->getFunction(name, params_types);
                unsigned short idx = 0;
                for (auto [arg_alloca, param_type, argument] : llvm::zip(arg_allocas, params_types, func->arguments)) {
                    if (param_type->stand_alone_type && std::get<2>(argument)) {
                        args[idx] = arg_alloca;
                    }
                    idx++;
                }
                auto returnValue = this->llvm_ir_builder.CreateCall(func->function, args, name + "_result");
                return {returnValue, nullptr, func->return_type, compiler::resolveType::StructInst};
            } else if (left_type->isGenericFunc(name)) {
                auto gfuncs = left_type->get_GenericFunc(name);
                return this->_CallGfunc(gfuncs, call_expression, name, args, params_types);
            } else if (left_type->isGenericStruct(name)) {
                auto gstruct = left_type->getGenericStruct(name);
                return this->_CallGstruct(gstruct, call_expression, name, args, params_types);
            } else if (left_type->is_struct(name)) {
                auto struct_record = left_type->get_struct(name);
                auto struct_type = struct_record->struct_type;
                llvm::Value* alloca;
                if (call_expression->_new) {
                    llvm::Value* gep = this->llvm_ir_builder.CreateGEP(struct_type, llvm::ConstantPointerNull::get(struct_type->getPointerTo()),
                                                                       llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_context), 1), "gep" + name);
                    llvm::Value* size = this->llvm_ir_builder.CreatePtrToInt(gep, this->GC_int, name + "_size");
                    alloca = this->llvm_ir_builder.CreateCall(this->enviornment->getFunction("malloc", {this->enviornment->getStruct("int")})->function, {size}, name + "_heap_pointer");
                } else
                    alloca = this->llvm_ir_builder.CreateAlloca(struct_type, nullptr, name);
                params_types.insert(params_types.begin(), struct_record);
                args.insert(args.begin(), alloca);
                if (struct_record->is_method("__init__", params_types)) {
                    auto method = struct_record->get_method("__init__", params_types);
                    this->llvm_ir_builder.CreateCall(method->function, args, name + "_initilizer");
                } else {
                    errors::NoOverload(this->source, {}, call_expression, "Initialization method does not exist for struct " + name + ".", "Check the initialization method name or define the method.")
                        .raise();
                    exit(1);
                }
                return {alloca, alloca, struct_record, compiler::resolveType::StructInst};
            } else {
                errors::DosentContain(this->source, std::static_pointer_cast<AST::IdentifierLiteral>(call_expression->name), left, "Struct Or Function " + name + " overload Dose Not Exit.",
                                      "Check the Name is Correct or the params are correct")
                    .raise();
                exit(1);
            }
        } else if (ltt == compiler::resolveType::StructInst) {
            auto left_type = std::get<std::shared_ptr<enviornment::RecordStructType>>(_left_type);
            params_types.insert(params_types.begin(), left_type);
            args.insert(args.begin(), left_alloca);
            arg_allocas.insert(arg_allocas.begin(), left_alloca);
            auto name = std::static_pointer_cast<AST::IdentifierLiteral>(std::static_pointer_cast<AST::CallExpression>(right)->name)->value;
            if (left_type->is_method(name, params_types)) {
                auto method = left_type->get_method(name, params_types);
                unsigned short idx = 0;
                for (auto [arg_alloca, param_type, argument] : llvm::zip(arg_allocas, params_types, method->arguments)) {
                    if (param_type->stand_alone_type && std::get<2>(argument)) {
                        args[idx] = arg_alloca;
                    }
                    idx++;
                }
                auto returnValue = this->llvm_ir_builder.CreateCall(method->function, args, name + "_reuturn_value");
                return {returnValue, nullptr, method->return_type, compiler::resolveType::StructInst};
            } else {
                errors::NoOverload(this->source, {}, call_expression, "method does not exist for struct " + left_type->name + ".", "Check the initialization method name or define the method.")
                    .raise();
                exit(1);
            }
        }
    }
    errors::CompletionError("Wrong Member Access", this->source, right->meta_data.st_line_no, right->meta_data.end_line_no,
                            "Member access should be identifier of method not " + AST::nodeTypeToString(right->type()))
        .raise();
    exit(1);
};

compiler::Compiler::ResolvedValue compiler::Compiler::_StructInfixCall(const std::string& op_method, const std::string& op, std::shared_ptr<enviornment::RecordStructType> left_type,
                                                                       std::shared_ptr<enviornment::RecordStructType> right_type, std::shared_ptr<AST::Expression> left,
                                                                       std::shared_ptr<AST::Expression> right, llvm::Value* left_value, llvm::Value* right_value) {
    std::vector<std::shared_ptr<enviornment::RecordStructType>> params_type1{left_type, right_type};
    std::vector<std::shared_ptr<enviornment::RecordStructType>> params_type2{right_type, left_type};
    if (left_type->is_method(op_method, params_type1)) {
        auto func_record = left_type->get_method(op_method, params_type1);
        auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
        return {returnValue, nullptr, func_record->return_type, compiler::resolveType::StructInst};
    } else if (right_type->is_method(op_method, params_type2)) {
        auto func_record = right_type->get_method(op_method, params_type2);
        auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
        return {returnValue, nullptr, func_record->return_type, compiler::resolveType::StructInst};
    } else {
        errors::WrongInfix(this->source, left, right, op, "Cant " + op + " 2 structs", "Add the `" + op_method + "` method in structs in either one of the struct").raise();
        exit(1);
    }
};


compiler::Compiler::ResolvedValue compiler::Compiler::_visitInfixExpression(std::shared_ptr<AST::InfixExpression> infixed_expression) {
    auto op = infixed_expression->op;
    auto left = infixed_expression->left;
    auto right = infixed_expression->right;
    auto [left_value, left_alloca, _left_type, ltt] = this->_resolveValue(left);
    if (op == token::TokenType::Dot) {
        return this->_memberAccess(infixed_expression);
    }
    auto [right_value, right_alloca, _right_type, rtt] = this->_resolveValue(right);
    if (ltt != compiler::resolveType::StructInst || rtt != compiler::resolveType::StructInst) {
        errors::WrongInfix(this->source, left, right, token::tokenTypeString(op), "Cant " + token::tokenTypeString(op) + " 2 types or modules").raise();
        exit(1);
    }
    // Handle type conversion
    auto left_val = left_value;
    auto right_val = right_value;
    auto left_type = std::get<std::shared_ptr<enviornment::RecordStructType>>(_left_type);
    auto right_type = std::get<std::shared_ptr<enviornment::RecordStructType>>(_right_type);
    if (!enviornment::_checkType(left_type, right_type) && (this->canConvertType(left_type, right_type) || this->canConvertType(right_type, left_type)) && left_type->stand_alone_type &&
        right_type->stand_alone_type) {
        if (this->conversionPrecidence(left_type, right_type)) {
            auto x = this->convertType({left_val, left_alloca, left_type}, right_type);
            right_val = x.value;
            right_alloca = x.alloca;
            right_type = std::get<std::shared_ptr<enviornment::RecordStructType>>(x.variant);
        } else {
            auto x = this->convertType({right_val, right_alloca, right_type}, left_type);
            left_val = x.value;
            left_alloca = x.alloca;
            left_type = std::get<std::shared_ptr<enviornment::RecordStructType>>(x.variant);
        }
    }
    std::vector<std::shared_ptr<enviornment::RecordStructType>> params_type1{left_type, right_type};
    std::vector<std::shared_ptr<enviornment::RecordStructType>> params_type2{right_type, left_type};
    if (left_type->struct_type != nullptr || right_type->struct_type != nullptr) {
        switch (op) {
            case token::TokenType::Plus: {
                return this->_StructInfixCall("__add__", "add", left_type, right_type, left, right, left_value, right_value);
            }
            case token::TokenType::Dash: {
                return this->_StructInfixCall("__sub__", "substract", left_type, right_type, left, right, left_value, right_value);
            }
            case token::TokenType::Asterisk: {
                return this->_StructInfixCall("__mul__", "multiply", left_type, right_type, left, right, left_value, right_value);
            }
            case token::TokenType::ForwardSlash: {
                return this->_StructInfixCall("__div__", "divide", left_type, right_type, left, right, left_value, right_value);
            }
            case token::TokenType::Percent: {
                return this->_StructInfixCall("__mod__", "%", left_type, right_type, left, right, left_value, right_value);
            }
            case token::TokenType::EqualEqual: {
                return this->_StructInfixCall("__eq__", "compare equals", left_type, right_type, left, right, left_value, right_value);
            }
            case token::TokenType::NotEquals: {
                return this->_StructInfixCall("__neq__", "compare not equals", left_type, right_type, left, right, left_value, right_value);
            }
            case token::TokenType::LessThan: {
                return this->_StructInfixCall("__lt__", "compare less than", left_type, right_type, left, right, left_value, right_value);
            }
            case token::TokenType::GreaterThan: {
                return this->_StructInfixCall("__gt__", "compare greater than", left_type, right_type, left, right, left_value, right_value);
            }
            case token::TokenType::LessThanOrEqual: {
                return this->_StructInfixCall("__lte__", "compare less than equals", left_type, right_type, left, right, left_value, right_value);
            }
            case token::TokenType::GreaterThanOrEqual: {
                return this->_StructInfixCall("__gte__", "compare greater than equals", left_type, right_type, left, right, left_value, right_value);
            }
            case token::TokenType::AsteriskAsterisk: {
                return this->_StructInfixCall("__pow__", "**", left_type, right_type, left, right, left_value, right_value);
            }
            default: {
                errors::WrongInfix(this->source, left, right, token::tokenTypeString(op), "Cant operator: `" + token::tokenTypeString(op) + "` 2 structs", "").raise();
                exit(1);
            }
        }
    }
    llvm::Value* left_val_converted = left_val;
    llvm::Value* right_val_converted = right_val;
    auto common_type = left_type;
    if (left_type != right_type) {
        if (this->conversionPrecidence(left_type, right_type)) {
            auto lct = this->convertType({left_val, left_alloca, left_type}, right_type);
            left_val_converted = lct.value;
            common_type = right_type;
        } else if (this->conversionPrecidence(right_type, left_type)) {
            auto rct = this->convertType({right_val, right_alloca, right_type}, left_type);
            right_val_converted = rct.value;
        } else {
            auto rct = this->convertType({right_val, right_alloca, right_type}, left_type);
            right_val_converted = rct.value;
        }
    }

    if (common_type->stand_alone_type->isIntegerTy()) {
        switch (op) {
            case (token::TokenType::Plus): {
                auto inst = this->llvm_ir_builder.CreateAdd(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("int"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::Dash): {
                auto inst = this->llvm_ir_builder.CreateSub(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("int"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::Asterisk): {
                auto inst = this->llvm_ir_builder.CreateMul(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("int"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::ForwardSlash): {
                llvm::Value* inst;
                if (left_type->name == "int" || left_type->name == "int32")
                    inst = this->llvm_ir_builder.CreateSDiv(left_val_converted, right_val_converted);
                else
                    inst = this->llvm_ir_builder.CreateUDiv(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("int"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::Percent): {
                llvm::Value* inst;
                if (left_type->name == "int" || left_type->name == "int32")
                    inst = this->llvm_ir_builder.CreateSRem(left_val_converted, right_val_converted);
                else
                    inst = this->llvm_ir_builder.CreateURem(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("int"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::EqualEqual): {
                auto inst = this->llvm_ir_builder.CreateICmpEQ(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::NotEquals): {
                auto inst = this->llvm_ir_builder.CreateICmpNE(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::LessThan): {
                llvm::Value* inst;
                if (left_type->name == "int" || left_type->name == "int32")
                    inst = this->llvm_ir_builder.CreateICmpSLT(left_val_converted, right_val_converted);
                else
                    inst = this->llvm_ir_builder.CreateICmpULT(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::GreaterThan): {
                llvm::Value* inst;
                if (left_type->name == "int" || left_type->name == "int32")
                    inst = this->llvm_ir_builder.CreateICmpSGT(left_val_converted, right_val_converted);
                else
                    inst = this->llvm_ir_builder.CreateICmpUGT(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::LessThanOrEqual): {
                llvm::Value* inst;
                if (left_type->name == "int" || left_type->name == "int32")
                    inst = this->llvm_ir_builder.CreateICmpSLE(left_val_converted, right_val_converted);
                else
                    inst = this->llvm_ir_builder.CreateICmpULE(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::GreaterThanOrEqual): {
                llvm::Value* inst;
                if (left_type->name == "int" || left_type->name == "int32")
                    inst = this->llvm_ir_builder.CreateICmpSGE(left_val_converted, right_val_converted);
                else
                    inst = this->llvm_ir_builder.CreateICmpUGE(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::AsteriskAsterisk): {
                errors::WrongInfix(this->source, left, right, token::tokenTypeString(op), "Power operator not supported for int").raise();
                exit(1);
            }
            default: {
                errors::WrongInfix(this->source, left, right, token::tokenTypeString(op), "Unknown operator: " + token::tokenTypeString(op)).raise();
                exit(1);
            }
        }
    } else if (common_type->stand_alone_type->isDoubleTy()) {
        switch (op) {
            case (token::TokenType::Plus): {
                auto inst = this->llvm_ir_builder.CreateFAdd(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("float"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::Dash): {
                auto inst = this->llvm_ir_builder.CreateFSub(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("float"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::Asterisk): {
                auto inst = this->llvm_ir_builder.CreateFMul(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("float"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::ForwardSlash): {
                auto inst = this->llvm_ir_builder.CreateFDiv(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("float"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::EqualEqual): {
                auto inst = this->llvm_ir_builder.CreateFCmpOEQ(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::NotEquals): {
                auto inst = this->llvm_ir_builder.CreateFCmpONE(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::LessThan): {
                auto inst = this->llvm_ir_builder.CreateFCmpOLT(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::GreaterThan): {
                auto inst = this->llvm_ir_builder.CreateFCmpOGT(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::LessThanOrEqual): {
                auto inst = this->llvm_ir_builder.CreateFCmpOLE(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::GreaterThanOrEqual): {
                auto inst = this->llvm_ir_builder.CreateFCmpOGE(left_val_converted, right_val_converted);
                return {inst, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
            }
            case (token::TokenType::AsteriskAsterisk): {
                errors::WrongInfix(this->source, left, right, token::tokenTypeString(op), "Power operator not supported for float").raise();
                exit(1);
            }
            default: {
                errors::WrongInfix(this->source, left, right, token::tokenTypeString(op), "Unknown operator: " + token::tokenTypeString(op)).raise();
                exit(1);
            }
        }
    } else {
        errors::WrongType(this->source, infixed_expression, {this->enviornment->getStruct("int"), this->enviornment->getStruct("float")}, "Unknown Type", "Check the types of the operands.").raise();
        exit(1);
    }
};

compiler::Compiler::ResolvedValue compiler::Compiler::_visitIndexExpression(std::shared_ptr<AST::IndexExpression> index_expression) {
    auto [left, left_alloca, _left_generic, ltt] = this->_resolveValue(index_expression->left);

    if (ltt != compiler::resolveType::StructInst) {
        errors::Cantindex(this->source, index_expression, false, "Cannot index Module or type", "Ensure the left-hand side is an array or a valid indexable type.").raise();
        exit(1);
    }

    auto left_generic = std::get<std::shared_ptr<enviornment::RecordStructType>>(_left_generic);
    auto [index, __, _index_generic, itt] = this->_resolveValue(index_expression->index);

    if (itt != compiler::resolveType::StructInst) {
        errors::Cantindex(this->source, index_expression, true, "Index must be an integer, not a Module or type", "Ensure the index is an integer.").raise();
        exit(1);
    }

    auto index_generic = std::get<std::shared_ptr<enviornment::RecordStructType>>(_index_generic);

    if (enviornment::_checkType(left_generic, this->enviornment->getStruct("array"))) {
        if (!enviornment::_checkType(index_generic, this->enviornment->getStruct("int"))) {
            errors::Cantindex(this->source, index_expression, true, "Index must be an integer not `" + index_generic->name + "`", "Ensure the index is an integer.").raise();
        }
        auto array = this->llvm_ir_builder.CreateLoad(this->GC_pointer, this->llvm_ir_builder.CreateGEP(this->GC_shared_ptr, left, llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_context), 0)));
        auto element_type = left_generic->generic_sub_types[0];
        auto element_ptr_type = element_type->struct_type || element_type->name == "array" ? this->GC_shared_ptr : element_type->stand_alone_type;
        auto element = this->llvm_ir_builder.CreateGEP(element_ptr_type, array, index, "element");

        llvm::Value* load = element_type->struct_type || element_type->name == "array" ? this->llvm_ir_builder.CreateLoad(this->GC_shared_ptr, element)
                                                                                       : this->llvm_ir_builder.CreateLoad(element_type->stand_alone_type, element);

        return {load, element, element_type, compiler::resolveType::StructInst};
    } else {
        if (left_generic->is_method("__index__", {left_generic, index_generic})) {
            auto idx_method = left_generic->get_method("__index__", {left_generic, index_generic});
            auto returnValue = this->llvm_ir_builder.CreateCall(idx_method->function, {left_alloca, index});
            return {returnValue, nullptr, idx_method->return_type, compiler::resolveType::StructInst};
        } else {
            errors::NoOverload(this->source, {}, index_expression, "__index__ method does not exist for struct " + left_generic->name + ".", "define the __index__ method.").raise();
            exit(1);
        }
    }
    exit(1);
};


void compiler::Compiler::_visitVariableDeclarationStatement(std::shared_ptr<AST::VariableDeclarationStatement> variable_declaration_statement) {
    auto var_name = std::static_pointer_cast<AST::IdentifierLiteral>(variable_declaration_statement->name);
    auto var_value = variable_declaration_statement->value;
    auto var_type = this->_parseType(variable_declaration_statement->value_type);

    if (var_value == nullptr) {
        llvm::Value* alloca = var_type->struct_type ? this->llvm_ir_builder.CreateAlloca(this->GC_shared_ptr) : this->llvm_ir_builder.CreateAlloca(var_type->stand_alone_type);
        if (var_type->struct_type) {
            auto RC_gep = this->llvm_ir_builder.CreateStructGEP(this->GC_shared_ptr, alloca, 1);
            this->llvm_ir_builder.CreateStore(llvm::ConstantPointerNull::get(this->GC_pointer), RC_gep);
        }
        auto var = std::make_shared<enviornment::RecordVariable>(var_name->value, nullptr, alloca, var_type);
        this->enviornment->addRecord(var);
    } else {
        auto [var_value_resolved, var_value_alloca, _var_generic, vartt] = this->_resolveValue(var_value);
        if (vartt != compiler::resolveType::StructInst) {
            errors::WrongType(this->source, var_value, {var_type}, "Cannot assign module or type to variable").raise();
            exit(1);
        }
        auto var_generic = std::get<std::shared_ptr<enviornment::RecordStructType>>(_var_generic);
        if (!enviornment::_checkType(var_generic, var_type)) {
            if (this->canConvertType(var_generic, var_type)) {
                auto x = this->convertType({var_value_resolved, var_value_alloca, var_generic}, var_type);
                var_value_resolved = x.value;
                var_value_alloca = x.alloca;
                var_generic = std::get<std::shared_ptr<enviornment::RecordStructType>>(x.variant);
            } else {
                errors::WrongType(this->source, var_value, {var_type}, "Cannot assign mismatched type").raise();
                exit(1);
            }
        }
        if (vartt == compiler::resolveType::StructInst) {
            if (var_type->stand_alone_type) {
                auto alloca = this->llvm_ir_builder.CreateAlloca(var_type->stand_alone_type);
                this->llvm_ir_builder.CreateStore(var_value_resolved, alloca, variable_declaration_statement->is_volatile);
                auto var = std::make_shared<enviornment::RecordVariable>(var_name->value, var_value_resolved, alloca, var_generic);
                this->enviornment->addRecord(var);
            } else {
                this->_incrementRC(var_value_alloca);
                auto var = std::make_shared<enviornment::RecordVariable>(var_name->value, var_value_resolved, var_value_resolved, var_generic);
                var->variable_type = var_generic;
                this->enviornment->addRecord(var);
            }
        } else {
            errors::WrongType(this->source, var_value, {var_type}, "Cannot assign module or type to variable").raise();
            exit(1);
        }
    }
}

std::vector<std::string> splitString(const std::string& input) {
    std::vector<std::string> result;
    size_t start = 0;
    size_t end = input.find('.');

    while (end != std::string::npos) {
        result.push_back(input.substr(start, end - start));
        start = end + 1;
        end = input.find('.', start);
    }

    result.push_back(input.substr(start));
    return result;
}

void compiler::Compiler::_visitVariableAssignmentStatement(std::shared_ptr<AST::VariableAssignmentStatement> variable_assignment_statement) {
    auto var_value = variable_assignment_statement->value;
    auto [value, value_alloca, _assignmentType, vtt] = this->_resolveValue(var_value);
    auto assignmentType = std::get<std::shared_ptr<enviornment::RecordStructType>>(_assignmentType);

    if (vtt != compiler::resolveType::StructInst) {
        errors::WrongType(this->source, var_value, {assignmentType}, "Cannot assign module or type to variable").raise();
        exit(1);
    }

    auto [_, alloca, _var_type, att] = this->_resolveValue(variable_assignment_statement->name);
    if (att != compiler::resolveType::StructInst) {
        errors::WrongType(this->source, variable_assignment_statement->name, {std::get<std::shared_ptr<enviornment::RecordStructType>>(_var_type)}, "Cannot assign to ltype").raise();
        exit(1);
    }

    auto var_type = std::get<std::shared_ptr<enviornment::RecordStructType>>(_var_type);
    if (!enviornment::_checkType(var_type, assignmentType)) {
        if (this->canConvertType(assignmentType, var_type)) {
            auto converted = this->convertType({value, value_alloca, assignmentType}, var_type);
            value = converted.value;
            value_alloca = converted.alloca;
            assignmentType = std::get<std::shared_ptr<enviornment::RecordStructType>>(converted.variant);
        } else {
            errors::WrongType(this->source, var_value, {var_type}, "Cannot assign mismatched type").raise();
            exit(1);
        }
    }

    if (assignmentType->struct_type) {
        this->_incrementRC(value_alloca);
        this->_decrementRC(alloca);
        auto load = this->llvm_ir_builder.CreateLoad(var_type->struct_type, value);
        this->llvm_ir_builder.CreateStore(load, alloca);
    } else {
        this->llvm_ir_builder.CreateStore(value, alloca);
    }
}

compiler::Compiler::ResolvedValue compiler::Compiler::_resolveValue(std::shared_ptr<AST::Node> node) {
    switch (node->type()) {
        case AST::NodeType::IntegerLiteral: {
            auto integer_literal = std::static_pointer_cast<AST::IntegerLiteral>(node);
            auto value = llvm::ConstantInt::get(llvm_context, llvm::APInt(64, integer_literal->value));
            auto alloca = this->llvm_ir_builder.CreateAlloca(this->enviornment->getStruct("int")->stand_alone_type);
            this->llvm_ir_builder.CreateStore(value, alloca);
            return {value, alloca, this->enviornment->getStruct("int"), compiler::resolveType::StructInst};
        }
        case AST::NodeType::FloatLiteral: {
            auto float_literal = std::static_pointer_cast<AST::FloatLiteral>(node);
            auto value = llvm::ConstantFP::get(llvm_context, llvm::APFloat(float_literal->value));
            auto alloca = this->llvm_ir_builder.CreateAlloca(this->enviornment->getStruct("float")->stand_alone_type);
            this->llvm_ir_builder.CreateStore(value, alloca);
            return {value, alloca, this->enviornment->getStruct("float"), compiler::resolveType::StructInst};
        }
        case AST::NodeType::StringLiteral: {
            auto string_literal = std::static_pointer_cast<AST::StringLiteral>(node);
            auto value = this->llvm_ir_builder.CreateGlobalStringPtr(string_literal->value);
            return {value, value, this->enviornment->getStruct("str"), compiler::resolveType::StructInst};
        }
        case AST::NodeType::IdentifierLiteral: {
            auto identifier_literal = std::static_pointer_cast<AST::IdentifierLiteral>(node);
            if (this->enviornment->isVariable(identifier_literal->value)) {
                auto variable = this->enviornment->getVariable(identifier_literal->value);
                auto currentStructType = variable->variable_type;
                currentStructType->meta_data = node->meta_data;
                if (currentStructType->stand_alone_type) {
                    auto loadInst = this->llvm_ir_builder.CreateLoad(currentStructType->stand_alone_type, variable->allocainst);
                    return {loadInst, variable->allocainst, currentStructType, compiler::resolveType::StructInst};
                } else {
                    return {variable->allocainst, variable->allocainst, currentStructType, compiler::resolveType::StructInst};
                }
            } else if (this->enviornment->isModule(identifier_literal->value)) {
                return {nullptr, nullptr, this->enviornment->getModule(identifier_literal->value), compiler::resolveType::Module};
            } else if (this->enviornment->isStruct(identifier_literal->value)) {
                return {nullptr, nullptr, this->enviornment->getStruct(identifier_literal->value), compiler::resolveType::StructType};
            } else if (this->enviornment->isGenericStruct(identifier_literal->value)) {
                return {nullptr, nullptr, this->enviornment->getGenericStruct(identifier_literal->value), compiler::resolveType::GStructType};
            }
            errors::NotDefined(this->source, identifier_literal, "Variable or function or struct `" + identifier_literal->value + "` not defined", "Recheck the Name").raise();
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
            auto value = boolean_literal->value ? llvm::ConstantInt::getTrue(this->llvm_context) : llvm::ConstantInt::getFalse(this->llvm_context);
            auto alloca = this->llvm_ir_builder.CreateAlloca(this->enviornment->getStruct("bool")->stand_alone_type);
            this->llvm_ir_builder.CreateStore(value, alloca);
            return {value, nullptr, this->enviornment->getStruct("bool"), compiler::resolveType::StructInst};
        }
        case AST::NodeType::ArrayLiteral: {
            return this->_visitArrayLiteral(std::static_pointer_cast<AST::ArrayLiteral>(node));
        }
        default: {
            errors::CompletionError("Unknown node type", this->source, node->meta_data.st_line_no, node->meta_data.end_line_no, "Unknown node type: " + AST::nodeTypeToString(node->type())).raise();
            exit(1);
        }
    }
};

compiler::Compiler::ResolvedValue compiler::Compiler::_visitArrayLiteral(std::shared_ptr<AST::ArrayLiteral> array_literal) {
    std::vector<llvm::Value*> values;
    std::shared_ptr<enviornment::RecordStructType> struct_type = nullptr;
    std::shared_ptr<enviornment::RecordStructType> first_generic;

    for (auto element : array_literal->elements) {
        auto [value, value_alloca, _generic, vtt] = this->_resolveValue(element);
        if (vtt != compiler::resolveType::StructInst) {
            errors::WrongType(this->source, element, {first_generic}, "Cannot add Module or type in Array").raise();
            exit(1);
        }

        auto generic = std::get<std::shared_ptr<enviornment::RecordStructType>>(_generic);
        if (!struct_type) {
            struct_type = generic;
            first_generic = generic;
        }

        if (!enviornment::_checkType(first_generic, generic)) {
            if (this->canConvertType(generic, first_generic)) {
                auto x = this->convertType({value, value_alloca, generic}, first_generic);
                value = x.value;
                value_alloca = x.alloca;
                generic = std::get<std::shared_ptr<enviornment::RecordStructType>>(x.variant);
            } else {
                errors::WrongType(this->source, element, {first_generic}, "Array with multiple types or generics").raise();
                exit(1);
            }
        }

        values.push_back(value);
    }

    auto array_type = llvm::ArrayType::get(struct_type->struct_type ? this->GC_shared_ptr : struct_type->stand_alone_type, values.size());
    llvm::Value* array;

    if (array_literal->_new) {
        llvm::Value* gep = this->llvm_ir_builder.CreateGEP(array_type, llvm::ConstantPointerNull::get(this->GC_pointer), llvm::ConstantInt::get(this->GC_int, 1));
        llvm::Value* size = this->llvm_ir_builder.CreatePtrToInt(gep, this->GC_int);
        array = this->llvm_ir_builder.CreateCall(this->enviornment->getFunction("malloc", {this->enviornment->getStruct("int")})->function, {size});
    } else {
        array = this->llvm_ir_builder.CreateAlloca(array_type);
    }

    for (size_t i = 0; i < values.size(); ++i) {
        auto element = this->llvm_ir_builder.CreateGEP(array_type, array, {this->llvm_ir_builder.getInt64(0), this->llvm_ir_builder.getInt64(i)});
        if (first_generic->struct_type || first_generic->name == "array") {
            this->_incrementRC(values[i]);
        }
        this->llvm_ir_builder.CreateStore(values[i], element);
    }

    auto array_struct = std::make_shared<enviornment::RecordStructType>(*this->enviornment->getStruct("array"));
    array_struct->generic_sub_types.push_back(first_generic);
    auto shared_array = this->llvm_ir_builder.CreateAlloca(this->GC_shared_ptr);
    this->llvm_ir_builder.CreateStore(array, this->llvm_ir_builder.CreateStructGEP(this->GC_shared_ptr, shared_array, 0));
    if (array_literal->_new) {
        auto RC_alloca = this->llvm_ir_builder.CreateCall(this->enviornment->getFunction("malloc", {this->enviornment->getStruct("int")})->function,
                                                          {llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_context), this->GC_int->getScalarSizeInBits() / 8)});
        this->llvm_ir_builder.CreateStore(llvm::ConstantInt::get(this->GC_int, 0), RC_alloca);
        this->llvm_ir_builder.CreateStore(RC_alloca, this->llvm_ir_builder.CreateStructGEP(this->GC_shared_ptr, shared_array, 1));
    } else
        this->llvm_ir_builder.CreateStore(llvm::ConstantPointerNull::get(this->GC_pointer), this->llvm_ir_builder.CreateStructGEP(this->GC_shared_ptr, shared_array, 1));
    return {shared_array, shared_array, array_struct, compiler::resolveType::StructInst};
};

void compiler::Compiler::_visitReturnStatement(std::shared_ptr<AST::ReturnStatement> return_statement) {
    auto value = return_statement->value;
    if (!value && this->enviornment->current_function->return_type->name == "void") {
        this->llvm_ir_builder.CreateRetVoid();
        throw compiler::DoneRet(); // Throws `DoneRet` Error to Indicate that Dont parse the following statements in the current block statement
    }
    auto [return_value, return_alloca, _, rtt] = this->_resolveValue(value);

    if (this->enviornment->current_function == nullptr) {
        errors::NodeOutside("Return outside loop", this->source, *return_statement, errors::outsideNodeType::Retuen, "Return statement outside the Function").raise();
        exit(1);
    }

    if (rtt != compiler::resolveType::StructInst) {
        if (rtt == compiler::resolveType::StructType && std::get<std::shared_ptr<enviornment::RecordStructType>>(_)->name == "void") {
            this->llvm_ir_builder.CreateRetVoid();
            throw compiler::DoneRet(); // Throws `DoneRet` Error to Indicate that Dont parse the following statements in the current block statement
        } else {
            errors::WrongType(this->source, value, {this->enviornment->current_function->return_type}, "Cannot return module or type from function").raise();
            exit(1);
        }
    }

    auto return_type = std::get<std::shared_ptr<enviornment::RecordStructType>>(_);
    if (!enviornment::_checkType(this->enviornment->current_function->return_type, return_type)) {
        if (this->canConvertType(return_type, this->enviornment->current_function->return_type)) {
            auto [converted_value, converted_alloca, converted_type, _] = this->convertType({return_value, return_alloca, return_type}, this->enviornment->current_function->return_type);
            return_value = converted_value;
            return_alloca = converted_alloca;
            if (!return_value) {
                errors::WrongType(this->source, value, {this->enviornment->current_function->return_type}, "Return Type mismatch").raise();
                exit(1);
            }
        } else {
            errors::WrongType(this->source, value, {this->enviornment->current_function->return_type}, "Return Type mismatch").raise();
            exit(1);
        }
    }
    if (return_type->struct_type)
        this->_incrementRC(return_value);
    for (auto var : this->enviornment->current_function->env->getCurrentVars()) {
        if (var->variable_type->struct_type || var->variable_type->name == "array") {
            this->_decrementRC(var->value);
        }
    }
    auto function_return_type = this->enviornment->current_function->function->getReturnType();
    if (function_return_type->isPointerTy() && return_value->getType()->isPointerTy()) {
        this->llvm_ir_builder.CreateRet(return_value);
    } else if (function_return_type->isPointerTy() && !return_value->getType()->isPointerTy()) {
        this->llvm_ir_builder.CreateRet(return_alloca);
    } else if (!function_return_type->isPointerTy() && return_value->getType()->isPointerTy()) {
        auto loaded_value = this->llvm_ir_builder.CreateLoad(function_return_type, return_value);
        this->llvm_ir_builder.CreateRet(loaded_value);
    } else {
        this->llvm_ir_builder.CreateRet(return_value);
    }
    throw compiler::DoneRet(); // Throws `DoneRet` Error to Indicate that Dont parse the following statements in the current block statement
};

std::shared_ptr<enviornment::RecordStructType> compiler::Compiler::_parseType(std::shared_ptr<AST::Type> type) {
    std::vector<std::shared_ptr<enviornment::RecordStructType>> generics;
    for (auto gen : type->generics) {
        generics.push_back(this->_parseType(gen));
    }

    auto [_, __, _struct, stt] = this->_resolveValue(type->name);
    if (stt != compiler::resolveType::StructType) {
        if (stt == compiler::resolveType::GStructType) {
            auto gstructs = std::get<std::vector<std::shared_ptr<enviornment::RecordGenericStructType>>>(_struct);
            auto prev_env = this->enviornment;

            for (auto gstruct : gstructs) {
                if (generics.size() != gstruct->structAST->generics.size()) {
                    continue;
                }

                this->enviornment = std::make_shared<enviornment::Enviornment>(gstruct->env);
                std::string struct_name = std::static_pointer_cast<AST::IdentifierLiteral>(gstruct->structAST->name)->value;
                auto struct_record = std::make_shared<enviornment::RecordStructType>(struct_name);

                if (!gstruct->env->isStruct(struct_name, false, generics)) {
                    for (auto [generic, rg] : llvm::zip(generics, gstruct->structAST->generics)) {
                        auto generic_copy = std::make_shared<enviornment::RecordStructType>(*generic);
                        generic_copy->name = std::static_pointer_cast<AST::IdentifierLiteral>(rg->name)->value;
                        this->enviornment->addRecord(generic_copy);
                    }

                    std::vector<llvm::Type*> field_types;
                    auto fields = gstruct->structAST->fields;
                    this->enviornment->addRecord(struct_record);

                    for (auto field : fields) {
                        if (field->type() == AST::NodeType::VariableDeclarationStatement) {
                            auto field_decl = std::static_pointer_cast<AST::VariableDeclarationStatement>(field);
                            std::string field_name = std::static_pointer_cast<AST::IdentifierLiteral>(field_decl->name)->value;
                            auto field_type = this->_parseType(field_decl->value_type);
                            struct_record->addSubType(field_name, field_type);

                            field_types.push_back(field_type->stand_alone_type ? field_type->stand_alone_type : field_type->struct_type);

                            if (field_decl->value_type->type() == AST::NodeType::IdentifierLiteral) {
                                auto field_value = std::static_pointer_cast<AST::IdentifierLiteral>(field_decl->value_type->name)->value;
                                bool is_generic = false;

                                for (const auto& generic : gstruct->structAST->generics) {
                                    if (field_value == std::static_pointer_cast<AST::IdentifierLiteral>(generic->name)->value) {
                                        is_generic = true;
                                        break;
                                    }
                                }

                                if (is_generic) {
                                    struct_record->generic_sub_types.push_back(field_type);
                                }
                            }

                            auto struct_type = llvm::StructType::create(this->llvm_context, field_types, this->fc_st_name_prefix + struct_name);
                            struct_type->setBody(field_types);
                            struct_record->struct_type = struct_type;
                        } else if (field->type() == AST::NodeType::FunctionStatement) {
                            auto func_dec = std::static_pointer_cast<AST::FunctionStatement>(field);
                            if (func_dec->generic.size() != 0) {
                                continue;
                            }
                            this->_visitFunctionDeclarationStatement(func_dec, struct_record);
                        }
                    }
                } else {
                    struct_record = gstruct->env->getStruct(struct_name, false, generics);
                }

                gstruct->env->addRecord(struct_record);
                this->enviornment = prev_env;
                return struct_record;
            }

            errors::NoOverload(this->source, {}, type->name, "No GStruct Viable").raise();
        }

        errors::WrongType(this->source, type->name, {}, "module is not a Type").raise();
    }

    auto struct_ = std::get<std::shared_ptr<enviornment::RecordStructType>>(_struct);
    if (struct_->name == "array") {
        struct_ = std::make_shared<enviornment::RecordStructType>(*struct_);
        struct_->generic_sub_types.push_back(generics[0]);
    }
    return struct_;
};

void compiler::Compiler::_visitFunctionDeclarationStatement(std::shared_ptr<AST::FunctionStatement> function_declaration_statement, std::shared_ptr<enviornment::RecordStructType> struct_) {
    auto name = std::static_pointer_cast<AST::IdentifierLiteral>(function_declaration_statement->name)->value;

    if (!function_declaration_statement->generic.empty()) {
        auto func_gen_rec = std::make_shared<enviornment::RecordGenericFunction>(name, function_declaration_statement, this->enviornment);
        this->enviornment->addRecord(func_gen_rec);
        return;
    }

    auto body = function_declaration_statement->body;
    auto params = function_declaration_statement->parameters;
    std::vector<std::string> param_names;
    std::vector<llvm::Type*> param_types;
    std::vector<std::shared_ptr<enviornment::RecordStructType>> param_inst_records;
    std::vector<bool> refrences;

    for (const auto& param : params) {
        auto param_name_str = std::static_pointer_cast<AST::IdentifierLiteral>(param->name)->value;
        param_names.push_back(param_name_str);
        auto param_type = this->_parseType(param->value_type);
        param_inst_records.push_back(param_type);
        refrences.push_back(param->value_type->refrence);
        param_types.push_back(param_type->struct_type || param_type->name == "array"
                                  ? this->GC_shared_ptr
                                  : (param->value_type->refrence ? llvm::PointerType::getUnqual(param_type->stand_alone_type) : param_type->stand_alone_type));
    }

    std::shared_ptr<enviornment::RecordStructType> return_type;
    if (function_declaration_statement->return_type)
        return_type = this->_parseType(function_declaration_statement->return_type);
    else
        return_type = this->enviornment->getStruct("void");
    auto llvm_return_type = return_type->struct_type ? this->GC_shared_ptr : return_type->stand_alone_type;
    auto func_type = llvm::FunctionType::get(llvm_return_type, param_types, false);
    auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, this->fc_st_name_prefix != "main.gc.." ? this->fc_st_name_prefix + name : name, this->llvm_module.get());

    unsigned idx = 0;
    for (auto& arg : func->args()) {
        arg.setName(param_names[idx++]);
    }

    std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructType>, bool>> arguments;
    auto func_record = std::make_shared<enviornment::RecordFunction>(name, func, func_type, arguments, return_type, function_declaration_statement->extra_info);

    if (body) {
        auto bb = llvm::BasicBlock::Create(this->llvm_context, "entry", func);
        this->function_entery_block.push_back(bb);
        this->llvm_ir_builder.SetInsertPoint(bb);

        auto prev_env = this->enviornment;
        this->enviornment = std::make_shared<enviornment::Enviornment>(prev_env, std::vector<std::tuple<std::string, std::shared_ptr<enviornment::Record>>>{}, name);
        func_record->env = this->enviornment;
        this->enviornment->current_function = func_record.get();

        for (const auto& [arg, param_type_record, refrence] : llvm::zip(func->args(), param_inst_records, refrences)) {
            llvm::Value* alloca = &arg;
            if (!arg.getType()->isPointerTy() || enviornment::_checkType(param_type_record, this->enviornment->getStruct("array"))) {
                alloca = this->llvm_ir_builder.CreateAlloca(arg.getType(), nullptr, arg.getName());
                this->llvm_ir_builder.CreateStore(&arg, alloca);
            }
            auto arg_type = param_type_record->stand_alone_type ? param_type_record->stand_alone_type : param_type_record->struct_type;
            llvm::Value* arg_value;
            if (refrence)
                arg_value = this->llvm_ir_builder.CreateLoad(arg_type, &arg);
            else
                arg_value = &arg;

            auto record = std::make_shared<enviornment::RecordVariable>(std::string(arg.getName()), arg_value, alloca, param_type_record);

            func_record->arguments.push_back({arg.getName().str(), param_type_record, refrence});
            this->enviornment->addRecord(record);
        }

        func_record->set_meta_data(function_declaration_statement->meta_data.st_line_no, function_declaration_statement->meta_data.st_col_no, function_declaration_statement->meta_data.end_line_no,
                                   function_declaration_statement->meta_data.end_col_no);
        this->enviornment->addRecord(func_record);

        try {
            this->compile(body);
            if (function_declaration_statement->return_type && return_type->name != "void")
                this->llvm_ir_builder.CreateUnreachable();
            else
                this->llvm_ir_builder.CreateRetVoid();
        } catch (compiler::DoneRet) {
            // Ignoring
        } catch (compiler::DoneBr) {
            // Ignore to compile the following commands if DoneRaise exception occurs
        }

        this->enviornment = prev_env;
        this->function_entery_block.pop_back();
        if (!this->function_entery_block.empty()) {
            this->llvm_ir_builder.SetInsertPoint(this->function_entery_block.back());
        }
    } else {
        for (const auto& [arg, param_type_record] : llvm::zip(func->args(), param_inst_records)) {
            func_record->arguments.push_back({arg.getName().str(), param_type_record, false});
        }
    }

    func_record->meta_data.more_data["name_line_no"] = function_declaration_statement->name->meta_data.st_line_no;
    func_record->meta_data.more_data["name_st_col_no"] = function_declaration_statement->name->meta_data.st_col_no;
    func_record->meta_data.more_data["name_end_col_no"] = function_declaration_statement->name->meta_data.end_col_no;
    func_record->meta_data.more_data["name_end_line_no"] = function_declaration_statement->name->meta_data.end_line_no;

    if (struct_) {
        struct_->methods.push_back({name, func_record});
        this->ir_gc_map_json["structs"][struct_->name]["methods"][name] = func->getName().str();
    } else {
        this->enviornment->addRecord(func_record);
        this->ir_gc_map_json["functions"][name] = func->getName().str();
    }
}

compiler::Compiler::ResolvedValue compiler::Compiler::_visitCallExpression(std::shared_ptr<AST::CallExpression> call_expression) {
    auto name = std::static_pointer_cast<AST::IdentifierLiteral>(call_expression->name)->value;
    auto param = call_expression->arguments;
    std::vector<llvm::Value*> args;
    std::vector<llvm::Value*> arg_allocas;
    std::vector<std::shared_ptr<enviornment::RecordStructType>> params_types;

    for (auto arg : param) {
        auto [value, alloca, _param_type, ptt] = this->_resolveValue(arg);
        auto param_type = std::get<std::shared_ptr<enviornment::RecordStructType>>(_param_type);
        if (ptt != compiler::resolveType::StructInst && ptt != compiler::resolveType::StructType) {
            errors::WrongType(this->source, arg, {}, "Cannot pass Module or type to the Function or struct").raise();
            exit(1);
        }
        params_types.push_back(param_type);
        args.push_back(param_type->struct_type || param_type->name == "array" ? this->llvm_ir_builder.CreateLoad(this->GC_shared_ptr, alloca) : value);
        arg_allocas.push_back(alloca);
    }

    if (this->enviornment->isGenericFunc(name) ? this->enviornment->isFunction(name, params_types, false, true) : this->enviornment->isFunction(name, params_types)) {
        auto func = this->enviornment->getFunction(name, params_types, false, true);
        unsigned short idx = 0;
        for (auto [arg_alloca, param_type, argument] : llvm::zip(arg_allocas, params_types, func->arguments)) {
            if (param_type->stand_alone_type && std::get<2>(argument)) {
                args[idx] = arg_alloca;
            }
            idx++;
        }
        this->_checkCallType(func, call_expression, args, params_types);
        auto returnValue = this->llvm_ir_builder.CreateCall(func->function, args);
        return {returnValue, nullptr, func->return_type, compiler::resolveType::StructInst};
    } else if (this->enviornment->isGenericFunc(name)) {
        auto gfuncs = this->enviornment->getGenericFunc(name);
        return this->_CallGfunc(gfuncs, call_expression, name, args, params_types);
    } else if (this->enviornment->isGenericStruct(name)) {
        auto gstruct = this->enviornment->getGenericStruct(name);
        return this->_CallGstruct(gstruct, call_expression, name, args, params_types);
    } else if (this->enviornment->isStruct(name)) {
        auto struct_record = this->enviornment->getStruct(name);
        auto struct_type = struct_record->struct_type;
        llvm::Value* alloca = this->llvm_ir_builder.CreateAlloca(this->GC_shared_ptr, nullptr, name);
        if (call_expression->_new) {
            alloca = this->llvm_ir_builder.CreateAlloca(this->GC_shared_ptr, nullptr, name);
            auto value_gep = this->llvm_ir_builder.CreateGEP(this->GC_shared_ptr, alloca, llvm::ConstantInt::get(this->GC_int, 0));
            llvm::Value* gep =
                this->llvm_ir_builder.CreateGEP(struct_type, llvm::ConstantPointerNull::get(struct_type->getPointerTo()), llvm::ConstantInt::get(llvm::Type::getInt64Ty(llvm_context), 1));
            llvm::Value* size = this->llvm_ir_builder.CreatePtrToInt(gep, llvm::Type::getInt64Ty(llvm_context));
            auto struct_alloca = this->llvm_ir_builder.CreateCall(this->enviornment->getFunction("malloc", {this->enviornment->getStruct("int")})->function, {size});
            this->llvm_ir_builder.CreateStore(struct_alloca, value_gep);
            auto RC_alloca = this->llvm_ir_builder.CreateCall(this->enviornment->getFunction("malloc", {this->enviornment->getStruct("int")})->function,
                                                              {llvm::ConstantInt::get(this->GC_int, this->GC_int->getScalarSizeInBits())});
            auto RC_gep = this->llvm_ir_builder.CreateGEP(this->GC_shared_ptr, alloca, llvm::ConstantInt::get(this->GC_int, 1));
            this->llvm_ir_builder.CreateStore(llvm::ConstantInt::get(this->GC_int, 0), RC_alloca);
            this->llvm_ir_builder.CreateStore(RC_alloca, RC_gep);
        } else {
            auto struct_alloca = this->llvm_ir_builder.CreateAlloca(struct_type, nullptr, name);
            auto value_gep = this->llvm_ir_builder.CreateGEP(this->GC_shared_ptr, alloca, llvm::ConstantInt::get(this->GC_int, 0));
            this->llvm_ir_builder.CreateStore(struct_alloca, value_gep);
            auto RC_gep = this->llvm_ir_builder.CreateGEP(this->GC_shared_ptr, alloca, llvm::ConstantInt::get(this->GC_int, 1));
            this->llvm_ir_builder.CreateStore(llvm::ConstantPointerNull::get(this->GC_pointer), RC_gep);
        }
        params_types.insert(params_types.begin(), struct_record);
        args.insert(args.begin(), this->llvm_ir_builder.CreateLoad(this->GC_shared_ptr, alloca));
        auto func = struct_record->get_method("__init__", params_types);

        if (func) {
            this->_checkCallType(func, call_expression, args, params_types);
            this->llvm_ir_builder.CreateCall(func->function, args);
        } else {
            errors::NoOverload(this->source, {}, call_expression, "Initialization method does not exist for struct " + struct_record->name + ".",
                               "Check the initialization method name or define the method.")
                .raise();
            exit(1);
        }
        return {alloca, alloca, struct_record, compiler::resolveType::StructInst};
    }

    errors::CompletionError("Function not defined", this->source, call_expression->meta_data.st_line_no, call_expression->meta_data.end_line_no, "Function `" + name + "` not defined").raise();
    exit(1);
};

void compiler::Compiler::_visitIfElseStatement(std::shared_ptr<AST::IfElseStatement> if_statement) {
    auto [condition_val, _, _condition, ctt] = this->_resolveValue(if_statement->condition);
    if (ctt != compiler::resolveType::StructInst || !enviornment::_checkType(std::get<std::shared_ptr<enviornment::RecordStructType>>(_condition), this->enviornment->getStruct("bool"))) {
        errors::WrongType(this->source, if_statement->condition, {this->enviornment->getStruct("bool")}, "if else Condition must be bool").raise();
        exit(1);
    }

    auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(llvm_context, "then", func);
    llvm::BasicBlock* ContBB = llvm::BasicBlock::Create(llvm_context, "cont", func);
    llvm::BasicBlock* ElseBB = if_statement->alternative ? llvm::BasicBlock::Create(llvm_context, "else", func) : nullptr;

    this->llvm_ir_builder.CreateCondBr(condition_val, ThenBB, ElseBB ? ElseBB : ContBB);
    this->llvm_ir_builder.SetInsertPoint(ThenBB);
    try {
        this->compile(if_statement->consequence);
        this->llvm_ir_builder.CreateBr(ContBB);
    } catch (compiler::DoneRet) {
    } catch (compiler::DoneBr) {
    }
    if (ElseBB) {
        this->llvm_ir_builder.SetInsertPoint(ElseBB);
        try {
            this->compile(if_statement->alternative);
            this->llvm_ir_builder.CreateBr(ContBB);
        } catch (compiler::DoneRet) {
        } catch (compiler::DoneBr) {
        }
    }

    this->llvm_ir_builder.SetInsertPoint(ContBB);
}

void compiler::Compiler::_visitWhileStatement(std::shared_ptr<AST::WhileStatement> while_statement) {
    auto condition = while_statement->condition;
    auto body = while_statement->body;
    auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();

    llvm::BasicBlock* CondBB = llvm::BasicBlock::Create(llvm_context, "cond", func);
    llvm::BasicBlock* BodyBB = llvm::BasicBlock::Create(llvm_context, "body", func);
    llvm::BasicBlock* ContBB = llvm::BasicBlock::Create(llvm_context, "cont", func);

    this->llvm_ir_builder.CreateBr(CondBB);
    this->llvm_ir_builder.SetInsertPoint(CondBB);

    auto [condition_val, _, _condition, ctt] = this->_resolveValue(condition);
    if (ctt != compiler::resolveType::StructInst) {
        errors::WrongType(this->source, condition, {this->enviornment->getStruct("bool")}, "while loop Condition can't be module or type").raise();
        exit(1);
    }

    auto bool_condition = std::get<std::shared_ptr<enviornment::RecordStructType>>(_condition);
    if (!enviornment::_checkType(bool_condition, this->enviornment->getStruct("bool"))) {
        if (this->canConvertType(bool_condition, this->enviornment->getStruct("bool"))) {
            std::cout << "SOB" << std::endl;
            auto x = this->convertType({condition_val, _, bool_condition}, this->enviornment->getStruct("bool"));
            condition_val = x.value;
        } else {
            errors::WrongType(this->source, condition, {this->enviornment->getStruct("bool")}, "while loop Condition must be bool").raise();
            exit(1);
        }
    }


    this->enviornment->loop_body_block.push_back(BodyBB);
    this->enviornment->loop_conti_block.push_back(ContBB);
    this->enviornment->loop_condition_block.push_back(CondBB);
    llvm::BasicBlock* IfBreakBB = nullptr;
    llvm::BasicBlock* NotBreakBB = nullptr;
    if (while_statement->ifbreak) {
        IfBreakBB = llvm::BasicBlock::Create(llvm_context, "ifbreak", func);
        this->enviornment->loop_ifbreak_block.push_back(IfBreakBB);
    } else
        this->enviornment->loop_ifbreak_block.push_back(nullptr);
    if (while_statement->notbreak) {
        NotBreakBB = llvm::BasicBlock::Create(llvm_context, "notbreak", func);
        this->enviornment->loop_notbreak_block.push_back(NotBreakBB);
        this->llvm_ir_builder.CreateCondBr(condition_val, BodyBB, NotBreakBB);
    } else {
        this->enviornment->loop_notbreak_block.push_back(nullptr);
        this->llvm_ir_builder.CreateCondBr(condition_val, BodyBB, ContBB);
    }

    this->llvm_ir_builder.SetInsertPoint(BodyBB);
    try {
        this->compile(body);
        this->llvm_ir_builder.CreateBr(CondBB);
    } catch (compiler::DoneRet) {
        // Ignore to compile the following commands if DoneRet exception occurs
    } catch (compiler::DoneBr) {
        // Ignore to compile the following commands if DoneRaise exception occurs
    }

    if (IfBreakBB) {
        this->llvm_ir_builder.SetInsertPoint(IfBreakBB);
        try {
            this->compile(while_statement->ifbreak);
            this->llvm_ir_builder.CreateBr(ContBB);
        } catch (compiler::DoneRet) {
            // Ignore to compile the following commands if DoneRet exception occurs
        } catch (compiler::DoneBr) {
            // Ignore to compile the following commands if DoneRaise exception occurs
        }
    }
    if (NotBreakBB) {
        this->llvm_ir_builder.SetInsertPoint(NotBreakBB);
        try {
            this->compile(while_statement->notbreak);
            this->llvm_ir_builder.CreateBr(ContBB);
        } catch (compiler::DoneRet) {
            // Ignore to compile the following commands if DoneRet exception occurs
        } catch (compiler::DoneBr) {
            // Ignore to compile the following commands if DoneRaise exception occurs
        }
    }

    this->llvm_ir_builder.SetInsertPoint(ContBB);

    this->enviornment->loop_body_block.pop_back();
    this->enviornment->loop_conti_block.pop_back();
    this->enviornment->loop_condition_block.pop_back();
    this->enviornment->loop_ifbreak_block.pop_back();
    this->enviornment->loop_notbreak_block.pop_back();
}

void compiler::Compiler::_visitForStatement(std::shared_ptr<AST::ForStatement> for_statement) {
    auto [loop_from_value, loop_from_alloca, _loop_from_type, w] = this->_resolveValue(for_statement->from);
    if (w != compiler::resolveType::StructInst) {
        errors::WrongType(this->source, for_statement->from, {}, "cant loop over module or type").raise();
        exit(1);
    }

    auto loop_from_type = std::get<std::shared_ptr<enviornment::RecordStructType>>(_loop_from_type);
    if (loop_from_type->is_method("__iter__", {loop_from_type})) {
        auto iter_geter = loop_from_type->get_method("__iter__", {loop_from_type});
        auto iter_type = iter_geter->return_type;
        if (!(loop_from_type->is_method("__next__", {loop_from_type, iter_type}) && loop_from_type->is_method("__done__", {loop_from_type, iter_type}, {}, this->enviornment->getStruct("bool")))) {
            std::cerr << "__next__ or __done__ method not define" << std::endl;
            exit(1);
        }
        auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* CondBB = llvm::BasicBlock::Create(llvm_context, "cond", func);
        llvm::BasicBlock* BodyBB = llvm::BasicBlock::Create(llvm_context, "body", func);
        llvm::BasicBlock* ContBB = llvm::BasicBlock::Create(llvm_context, "cont", func);
        this->enviornment->loop_body_block.push_back(BodyBB);
        this->enviornment->loop_conti_block.push_back(ContBB);
        this->enviornment->loop_condition_block.push_back(CondBB);

        auto next_method = loop_from_type->get_method("__next__", {loop_from_type, iter_type});
        auto done_method = loop_from_type->get_method("__done__", {loop_from_type, iter_type});
        llvm::Value* iterator_alloca = this->llvm_ir_builder.CreateAlloca(iter_type->stand_alone_type);
        if (iter_type->stand_alone_type)
            this->llvm_ir_builder.CreateStore(this->llvm_ir_builder.CreateCall(iter_geter->function, {loop_from_alloca}), iterator_alloca);
        else
            iterator_alloca = this->llvm_ir_builder.CreateCall(iter_geter->function, {loop_from_alloca});
        llvm::BasicBlock* IfBreakBB = nullptr;
        llvm::BasicBlock* NotBreakBB = nullptr;
        if (for_statement->ifbreak) {
            IfBreakBB = llvm::BasicBlock::Create(llvm_context, "ifbreak", func);
            this->enviornment->loop_ifbreak_block.push_back(IfBreakBB);
        } else
            this->enviornment->loop_ifbreak_block.push_back(nullptr);
        this->llvm_ir_builder.CreateBr(CondBB);
        this->llvm_ir_builder.SetInsertPoint(CondBB);
        if (for_statement->notbreak) {
            NotBreakBB = llvm::BasicBlock::Create(llvm_context, "notbreak", func);
            this->enviornment->loop_notbreak_block.push_back(NotBreakBB);
            this->llvm_ir_builder.CreateCondBr(this->llvm_ir_builder.CreateCall(done_method->function, {loop_from_alloca, iterator_alloca}), NotBreakBB, BodyBB);
        } else {
            this->enviornment->loop_notbreak_block.push_back(nullptr);
            this->llvm_ir_builder.CreateCondBr(this->llvm_ir_builder.CreateCall(done_method->function, {loop_from_alloca, iterator_alloca}), ContBB, BodyBB);
        }
        this->llvm_ir_builder.SetInsertPoint(BodyBB);
        // Var
        auto prev_env = this->enviornment;
        this->enviornment = std::make_shared<enviornment::Enviornment>(this->enviornment);
        auto alloca = next_method->return_type->stand_alone_type ? this->llvm_ir_builder.CreateAlloca(next_method->return_type->stand_alone_type)
                                                                 : this->llvm_ir_builder.CreateAlloca(next_method->return_type->struct_type);
        if (next_method->return_type->struct_type) {
            this->llvm_ir_builder.CreateStore(
                this->llvm_ir_builder.CreateLoad(next_method->return_type->struct_type, this->llvm_ir_builder.CreateCall(next_method->function, {loop_from_alloca, iterator_alloca})), alloca);
        } else {
            this->llvm_ir_builder.CreateStore(this->llvm_ir_builder.CreateCall(next_method->function, {loop_from_alloca, iterator_alloca}), alloca);
        };

        std::shared_ptr<enviornment::RecordVariable> var;
        if (next_method->return_type->struct_type)
            var = std::make_shared<enviornment::RecordVariable>(for_statement->get->value, alloca, alloca, next_method->return_type);
        else
            var = std::make_shared<enviornment::RecordVariable>(for_statement->get->value, this->llvm_ir_builder.CreateLoad(next_method->return_type->stand_alone_type, alloca), alloca,
                                                                next_method->return_type);
        this->enviornment->addRecord(var);

        try {
            this->compile(for_statement->body);
            this->llvm_ir_builder.CreateBr(CondBB);
        } catch (compiler::DoneRet) {
            // Ignore to compile the following commands if DoneRet exception occurs
        } catch (compiler::DoneBr) {
            // Ignore to compile the following commands if DoneRaise exception occurs
        }

        if (IfBreakBB) {
            this->llvm_ir_builder.SetInsertPoint(IfBreakBB);
            try {
                this->compile(for_statement->ifbreak);
                this->llvm_ir_builder.CreateBr(ContBB);
            } catch (compiler::DoneRet) {
                // Ignore to compile the following commands if DoneRet exception occurs
            } catch (compiler::DoneBr) {
                // Ignore to compile the following commands if DoneRaise exception occurs
            }
        }
        if (NotBreakBB) {
            this->llvm_ir_builder.SetInsertPoint(NotBreakBB);
            try {
                this->compile(for_statement->notbreak);
                this->llvm_ir_builder.CreateBr(ContBB);
            } catch (compiler::DoneRet) {
                // Ignore to compile the following commands if DoneRet exception occurs
            } catch (compiler::DoneBr) {
                // Ignore to compile the following commands if DoneRaise exception occurs
            }
        }

        this->enviornment = prev_env;
        this->llvm_ir_builder.SetInsertPoint(ContBB);

        this->enviornment->loop_body_block.pop_back();
        this->enviornment->loop_conti_block.pop_back();
        this->enviornment->loop_condition_block.pop_back();
        this->enviornment->loop_ifbreak_block.pop_back();
        this->enviornment->loop_notbreak_block.pop_back();
    } else {
        std::cerr << "__iter__ method not define" << std::endl;
        exit(1);
    }
}

void compiler::Compiler::addFieldToStruct(std::shared_ptr<enviornment::RecordStructType> struct_record, std::shared_ptr<AST::VariableDeclarationStatement> field_decl,
                                          std::vector<llvm::Type*>& field_types, std::string struct_name) {
    std::string field_name = std::static_pointer_cast<AST::IdentifierLiteral>(field_decl->name)->value;

    auto field_type = this->_parseType(field_decl->value_type);
    field_types.push_back(field_type->struct_type || field_type->name == "array" ? this->GC_shared_ptr : field_type->stand_alone_type);
    struct_record->addSubType(field_name, field_type);

    auto struct_type = llvm::StructType::create(this->llvm_context, field_types, this->fc_st_name_prefix + struct_name);
    struct_type->setBody(field_types);
    struct_record->struct_type = struct_type;
};

void compiler::Compiler::_visitStructStatement(std::shared_ptr<AST::StructStatement> struct_statement) {
    std::string struct_name = std::static_pointer_cast<AST::IdentifierLiteral>(struct_statement->name)->value;
    this->ir_gc_map_json["structs"][struct_name] = json::object();
    this->ir_gc_map_json["structs"][struct_name]["methods"] = json::object();

    auto struct_record = std::make_shared<enviornment::RecordStructType>(struct_name);
    if (!struct_statement->generics.empty()) {
        auto gsr = std::make_shared<enviornment::RecordGenericStructType>(struct_name, struct_statement, this->enviornment);
        this->enviornment->addRecord(gsr);
        return;
    }

    std::vector<llvm::Type*> field_types;
    auto fields = struct_statement->fields;
    this->enviornment->addRecord(struct_record);

    for (auto field : fields) {
        if (field->type() == AST::NodeType::VariableDeclarationStatement) {
            this->addFieldToStruct(struct_record, std::static_pointer_cast<AST::VariableDeclarationStatement>(field), field_types, struct_name);
        } else if (field->type() == AST::NodeType::FunctionStatement) {
            auto func_dec = std::static_pointer_cast<AST::FunctionStatement>(field);
            if (!func_dec->generic.empty()) {
                errors::CompletionError("Doesn't Support", this->source, func_dec->meta_data.st_line_no, func_dec->meta_data.end_line_no, "Struct Methods do not support Generic Functions",
                                        "Set the Generic on the struct")
                    .raise();
                exit(1);
            }
            this->_visitFunctionDeclarationStatement(func_dec, struct_record);
        }
    }

    // Implementing the __clear__ method
    auto clear_func_type = llvm::FunctionType::get(llvm::Type::getVoidTy(this->llvm_context), {this->GC_shared_ptr}, false);
    auto clear_func = llvm::Function::Create(clear_func_type, llvm::Function::ExternalLinkage, struct_name + "__clear__", this->llvm_module.get());
    auto bb = llvm::BasicBlock::Create(this->llvm_context, "entry", clear_func);
    this->llvm_ir_builder.SetInsertPoint(bb);

    for (size_t i = 0; i < struct_record->getFields().size(); ++i) {
        auto field_name = struct_record->getFields()[i];
        auto field_type = struct_record->sub_types[field_name];

        if (field_type->struct_type || field_type->name == "array") {
            auto rc_pointer = this->llvm_ir_builder.CreateExtractValue(clear_func->arg_begin(), 1);

            auto is_not_null = this->llvm_ir_builder.CreateICmpNE(rc_pointer, llvm::ConstantPointerNull::get(this->GC_pointer));
            auto then_bb = llvm::BasicBlock::Create(this->llvm_context, "then", clear_func);
            auto cont_bb = llvm::BasicBlock::Create(this->llvm_context, "cont", clear_func);
            this->llvm_ir_builder.CreateCondBr(is_not_null, then_bb, cont_bb);

            this->llvm_ir_builder.SetInsertPoint(then_bb);
            auto new_rc_value = this->llvm_ir_builder.CreateSub(this->llvm_ir_builder.CreateLoad(this->GC_int, rc_pointer), llvm::ConstantInt::get(this->GC_int, 1));
            auto updated_shared_ptr = this->llvm_ir_builder.CreateStore(new_rc_value, rc_pointer);

            auto is_zero = this->llvm_ir_builder.CreateICmpEQ(new_rc_value, llvm::ConstantInt::get(this->GC_int, 0));
            auto free_bb = llvm::BasicBlock::Create(this->llvm_context, "free", clear_func);
            this->llvm_ir_builder.CreateCondBr(is_zero, free_bb, cont_bb);

            this->llvm_ir_builder.SetInsertPoint(free_bb);
            this->llvm_ir_builder.CreateCall(this->enviornment->getFunction("free", {this->enviornment->getStruct("int")})->function,
                                             {this->llvm_ir_builder.CreateExtractValue(clear_func->arg_begin(), 0)});
            this->llvm_ir_builder.CreateBr(cont_bb);

            this->llvm_ir_builder.SetInsertPoint(cont_bb);
        }
    }

    this->llvm_ir_builder.CreateRetVoid();
    struct_record->gc_struct_clear = std::make_shared<enviornment::RecordFunction>(
        "__clear__", clear_func, clear_func_type, std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructType>, bool>>(), this->enviornment->getStruct("void"));
    this->ir_gc_map_json["structs"][struct_name]["name"] = struct_record->struct_type->getName().str();
}

void compiler::Compiler::_visitTryCatchStatement(std::shared_ptr<AST::TryCatchStatement> tc_statement) {
    std::cerr << "TODO: Add Suport to Handel Exception" << std::endl;
    exit(1);
}

void compiler::Compiler::_visitRaiseStatement(std::shared_ptr<AST::RaiseStatement> raise_statement) {
    std::cerr << "TODO: Add Suport to raise Exception" << std::endl;
    exit(1);
}

const std::string readFileToString(const std::string& filePath); // Defined in main.cpp

void compiler::Compiler::_visitImportStatement(std::shared_ptr<AST::ImportStatement> import_statement, std::shared_ptr<enviornment::RecordModule> module) {
    std::string relative_path = import_statement->relativePath;
    auto module_name = relative_path.substr(relative_path.find_last_of('/') + 1);
    std::replace(module_name.begin(), module_name.end(), '.', '_');

    if (!module) {
        module = std::make_shared<enviornment::RecordModule>(module_name);
        this->enviornment->addRecord(module);
    } else {
        auto new_mod = std::make_shared<enviornment::RecordModule>(module_name);
        module->record_map.push_back({module_name, new_mod});
        module = new_mod;
    }

    if (relative_path.ends_with(".c")) {
        std::cerr << "Cannot import C code" << std::endl;
        exit(1);
    } else if (relative_path.ends_with(".py")) {
        std::cerr << "Cannot import Python code" << std::endl;
        exit(1);
    }

    auto gc_source_path = std::filesystem::path(this->file_path.parent_path().string() + "/" + relative_path + ".gc");
    if (relative_path.starts_with("std/")) {
        gc_source_path = std::filesystem::path((GC_STD_DIR / relative_path.substr(4)).string() + ".gc");
    }

    json ir_gc_map_json;
    auto ir_gc_map = std::filesystem::path(this->ir_gc_map.parent_path().string() + "/" + relative_path + ".gc" + ".json");
    if (relative_path.starts_with("std/")) {
        ir_gc_map = std::filesystem::path(GC_STD_IRGCMAP / (relative_path.substr(4) + ".gc" + ".json"));
    }

    std::ifstream ir_gc_map_file(ir_gc_map);
    if (!ir_gc_map_file.is_open()) {
        std::cerr << "Failed to open ir_gc_map file: " << ir_gc_map << std::endl;
        throw std::runtime_error("Failed to open ir_gc_map file: " + ir_gc_map.string());
    }
    ir_gc_map_file >> ir_gc_map_json;
    ir_gc_map_file.close();

    if (!ir_gc_map_json["uptodate"]) {
        std::cerr << "IR GC map not up-to-date, throwing NotCompiledError" << std::endl;
        throw compiler::NotCompiledError(gc_source_path.string());
    }

    auto prev_path = this->file_path;
    this->file_path = gc_source_path;
    auto gc_source = readFileToString(gc_source_path.string());
    auto prev_source = this->source;
    this->source = gc_source;

    Lexer lexer(gc_source);
    parser::Parser parser(std::make_shared<Lexer>(lexer));
    auto program = parser.parseProgram();

    auto prev_env = this->enviornment;
    this->enviornment = std::make_shared<enviornment::Enviornment>(this->enviornment->parent, std::vector<std::tuple<std::string, std::shared_ptr<enviornment::Record>>>(), module_name);

    for (auto& stmt : program->statements) {
        switch (stmt->type()) {
            case AST::NodeType::FunctionStatement:
                this->_importFunctionDeclarationStatement(std::static_pointer_cast<AST::FunctionStatement>(stmt), module, ir_gc_map_json);
                break;
            case AST::NodeType::StructStatement:
                this->_importStructStatement(std::static_pointer_cast<AST::StructStatement>(stmt), module, ir_gc_map_json);
                break;
            case AST::NodeType::ImportStatement:
                this->_visitImportStatement(std::static_pointer_cast<AST::ImportStatement>(stmt), module);
                break;
            default:
                std::cerr << "Unknown statement type, skipping" << std::endl;
                break;
        }
    }

    this->file_path = prev_path;
    this->enviornment = prev_env;
    this->source = prev_source;
}

void compiler::Compiler::_importFunctionDeclarationStatement(std::shared_ptr<AST::FunctionStatement> function_declaration_statement, std::shared_ptr<enviornment::RecordModule> module,
                                                             json& ir_gc_map_json) {

    auto name = std::static_pointer_cast<AST::IdentifierLiteral>(function_declaration_statement->name)->value;

    if (!function_declaration_statement->generic.empty()) {
        auto gsr = std::make_shared<enviornment::RecordGenericFunction>(name, function_declaration_statement, this->enviornment);
        this->enviornment->addRecord(gsr);
        module->record_map.push_back({name, gsr});
        return;
    }

    auto params = function_declaration_statement->parameters;
    std::vector<std::string> param_names;
    std::vector<llvm::Type*> param_types;
    std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructType>, bool>> arguments;

    for (const auto& param : params) {
        auto param_name = std::static_pointer_cast<AST::IdentifierLiteral>(param->name)->value;
        param_names.push_back(param_name);

        auto param_type = this->_parseType(param->value_type);
        auto llvm_param_type = param_type->stand_alone_type ? param_type->stand_alone_type : llvm::PointerType::get(param_type->struct_type, 0);
        param_types.push_back(llvm_param_type);

        arguments.push_back({param_name, param_type, param->value_type->refrence});
    }

    auto return_type = this->_parseType(function_declaration_statement->return_type);
    auto llvm_return_type = return_type->stand_alone_type ? return_type->stand_alone_type : return_type->struct_type->getPointerTo();
    auto func_type = llvm::FunctionType::get(llvm_return_type, param_types, false);
    auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, ir_gc_map_json["functions"][name].get<std::string>(), this->llvm_module.get());

    unsigned idx = 0;
    for (auto& arg : func->args()) {
        arg.setName(param_names[idx++]);
    }

    auto func_record = std::make_shared<enviornment::RecordFunction>(name, func, func_type, arguments, return_type, function_declaration_statement->extra_info);
    module->record_map.push_back({func_record->name, func_record});
    this->enviornment->addRecord(func_record);
}

void compiler::Compiler::_importStructStatement(std::shared_ptr<AST::StructStatement> struct_statement, std::shared_ptr<enviornment::RecordModule> module, json& ir_gc_map_json) {
    std::string struct_name = std::static_pointer_cast<AST::IdentifierLiteral>(struct_statement->name)->value;

    if (!struct_statement->generics.empty()) {
        auto gsr = std::make_shared<enviornment::RecordGenericStructType>(struct_name, struct_statement, this->enviornment);
        this->enviornment->addRecord(gsr);
        module->record_map.push_back({struct_name, gsr});
        return;
    }

    std::vector<llvm::Type*> field_types;
    auto fields = struct_statement->fields;
    auto struct_record = std::make_shared<enviornment::RecordStructType>(struct_name);
    this->enviornment->addRecord(struct_record);

    for (auto field : fields) {
        if (field->type() == AST::NodeType::VariableDeclarationStatement) {
            this->addFieldToStruct(struct_record, std::static_pointer_cast<AST::VariableDeclarationStatement>(field), field_types, struct_name);
        } else if (field->type() == AST::NodeType::FunctionStatement) {
            auto field_decl = std::static_pointer_cast<AST::FunctionStatement>(field);
            auto method_name = std::static_pointer_cast<AST::IdentifierLiteral>(field_decl->name)->value;
            auto method_params = field_decl->parameters;

            std::vector<std::string> param_names;
            std::vector<llvm::Type*> param_types;
            std::vector<std::shared_ptr<enviornment::RecordStructType>> param_inst_records;
            std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordStructType>, bool>> arguments;

            for (auto param : method_params) {
                auto param_name = std::static_pointer_cast<AST::IdentifierLiteral>(param->name)->value;
                param_names.push_back(param_name);

                auto param_type = this->_parseType(param->value_type);
                param_inst_records.push_back(param_type);
                param_types.push_back(param_type->stand_alone_type ? param_type->stand_alone_type : llvm::PointerType::get(param_type->struct_type, 0));
                arguments.push_back({param_name, param_type, param->value_type->refrence});
            }

            auto return_type = this->_parseType(field_decl->return_type);
            auto llvm_return_type = return_type->stand_alone_type ? return_type->stand_alone_type : return_type->struct_type->getPointerTo();
            auto func_type = llvm::FunctionType::get(llvm_return_type, param_types, false);
            auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, ir_gc_map_json["structs"][struct_name]["methods"][method_name].get<std::string>(), this->llvm_module.get());

            unsigned short idx = 0;
            for (auto& arg : func->args()) {
                arg.setName(param_names[idx++]);
            }

            auto func_record = std::make_shared<enviornment::RecordFunction>(method_name, func, func_type, arguments, return_type, field_decl->extra_info);
            struct_record->methods.push_back({method_name, func_record});
        }
    }

    module->record_map.push_back({struct_name, struct_record});
}
