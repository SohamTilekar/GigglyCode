// TODO: Add Meta Data to all of the Record.
#include "compiler.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <llvm/ADT/APInt.h>
#include <llvm/ADT/STLExtras.h>
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

using namespace compiler;
using llConstInt = llvm::ConstantInt;
using Json = AST::Json;

Compiler::Compiler(const Str& source, const std::filesystem::path& file_path, const std::filesystem::path& ir_gc_map, const std::filesystem::path& buildDir, const std::filesystem::path& relativePath)
    : llvm_context(), llvm_ir_builder(llvm_context), source(source), file_path(std::move(file_path)), ir_gc_map(std::move(ir_gc_map)), buildDir(std::move(buildDir)),
      relativePath(std::move(relativePath)) {

    // Convert file path to Str
    Str path_str = file_path.string();

    // Extract subStr after 'src/' or from the beginning if 'src' not found
    this->fc_st_name_prefix = extractPrefix(path_str);

    // Replace path delimiters with ".." for prefix
    replaceDelimiters(this->fc_st_name_prefix);

    // Initialize LLVM module with the modified prefix
    _initializeLLVMModule(path_str);

    // Initialize the compilation environment with built-ins
    _initializeEnvironment();

    // Initialize built-in functions and types
    _initializeBuiltins();

    // Initialize JSON structure for IR-GC mapping
    _initializeIRGCMap();
}

Str Compiler::extractPrefix(const Str& path_str) {
    size_t pos = path_str.rfind("src");
    pos = (pos != Str::npos) ? pos + 4 : 0;
    return path_str.substr(pos);
}

void Compiler::replaceDelimiters(Str& prefix) {
    const vector<char> delimiters = {'/', '\\'};
    for (const char delimiter : delimiters) {
        size_t delimiter_pos = 0;
        while ((delimiter_pos = prefix.find(delimiter, delimiter_pos)) != Str::npos) {
            prefix.replace(delimiter_pos, 1, "..");
            delimiter_pos += 2; // Move past the replaced characters
        }
    }
    prefix += ".."; // Append additional delimiter if needed
}

void Compiler::_initializeLLVMModule(const Str& path_str) {
    this->llvm_module = std::make_unique<llvm::Module>(this->fc_st_name_prefix, this->llvm_context);
    this->llvm_module->setSourceFileName(path_str);
}

void Compiler::_initializeEnvironment() {
    env = std::make_shared<Enviornment>(std::make_shared<Enviornment>(nullptr, StrRecordMap(), "builtins"), StrRecordMap());
}

void Compiler::_initializeIRGCMap() {
    this->ir_gc_map_json = Json{{"functions", Json::object()}, {"structs", Json::object()}, {"GSinstance", Json::array()}, {"GFinstance", Json::array()}};
}

void Compiler::addBuiltinType(const Str& name, llvm::Type* type) {
    auto record = std::make_shared<RecordStructType>(name, type);
    this->env->parent->addRecord(record);
}

void Compiler::addBuiltinFunction(const Str& name, llvm::FunctionType* funcType, const vector<std::tuple<Str, StructTypePtr, bool>>& params, StructTypePtr returnType) {
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, this->llvm_module.get());
    this->env->parent->addRecord(std::make_shared<RecordFunction>(name, func, funcType, params, returnType, true));
}

void Compiler::_initializeBuiltins() {
    // Types
    this->ll_pointer = llvm::PointerType::get(this->llvm_context, 0);

    addBuiltinType("int", llvm::Type::getInt64Ty(this->llvm_context));
    this->gc_int = env->parent->getStruct("int");
    this->ll_int = this->gc_int->stand_alone_type;

    addBuiltinType("int32", llvm::Type::getInt32Ty(this->llvm_context));
    this->gc_int32 = env->parent->getStruct("int32");
    this->ll_int32 = this->gc_int32->stand_alone_type;

    addBuiltinType("uint", llvm::Type::getInt64Ty(this->llvm_context));
    this->gc_uint = env->parent->getStruct("uint");
    this->ll_uint = this->gc_uint->stand_alone_type;

    addBuiltinType("uint32", llvm::Type::getInt32Ty(this->llvm_context));
    this->gc_uint32 = env->parent->getStruct("uint32");
    this->ll_uint32 = this->gc_uint32->stand_alone_type;

    addBuiltinType("float", llvm::Type::getDoubleTy(this->llvm_context));
    this->gc_float = env->parent->getStruct("float");
    this->ll_float = this->gc_float->stand_alone_type;

    addBuiltinType("float32", llvm::Type::getFloatTy(this->llvm_context));
    this->gc_float32 = env->parent->getStruct("float32");
    this->ll_float32 = this->gc_float32->stand_alone_type;

    addBuiltinType("char", llvm::Type::getInt8Ty(this->llvm_context));
    this->gc_char = env->parent->getStruct("char");
    this->ll_char = this->gc_char->stand_alone_type;

    addBuiltinType("str", this->ll_pointer);
    this->gc_str = env->parent->getStruct("str");
    this->ll_str = this->gc_str->stand_alone_type;

    addBuiltinType("void", llvm::Type::getVoidTy(this->llvm_context));
    this->gc_void = env->parent->getStruct("void");
    this->ll_void = this->gc_void->stand_alone_type;

    addBuiltinType("bool", llvm::Type::getInt1Ty(this->llvm_context));
    this->gc_bool = env->parent->getStruct("bool");
    this->ll_bool = this->gc_bool->stand_alone_type;

    addBuiltinType("array", this->ll_pointer);
    this->gc_array = env->parent->getStruct("array");
    this->ll_array = this->ll_pointer;

    this->ll_shared_ptr = llvm::StructType::create(this->llvm_context, {/*Value: */ this->ll_pointer, /*RC: */ this->ll_pointer}, "shared_ptr");
    auto _Any = std::make_shared<RecordModule>("Any");
    env->parent->addRecord(_Any);
    // Initializing Built-in Types
    addBuiltinFunction("puts", llvm::FunctionType::get(this->ll_void, this->ll_str, false), {{"Str", this->gc_str, false}}, this->gc_void);
    addBuiltinFunction("printf", llvm::FunctionType::get(this->ll_int, {this->ll_str}, true), {{"format", this->gc_str, false}}, this->gc_int);
    addBuiltinFunction("printf", llvm::FunctionType::get(this->ll_int, {this->ll_str}, true), {{"format", this->gc_str, false}}, this->gc_int);
    addBuiltinFunction("scanf", llvm::FunctionType::get(this->ll_int, {this->ll_str}, true), {{"format", this->gc_str, false}}, this->gc_int);
    addBuiltinFunction("malloc", llvm::FunctionType::get(this->ll_pointer, {this->ll_int}, true), {{"bits", this->gc_int, false}}, this->gc_void);
    addBuiltinFunction("free", llvm::FunctionType::get(this->ll_void, {this->ll_pointer}, true), {}, this->gc_void);
}

void Compiler::_incrementRC(llvm::Value* value) {
    // Pseudocode:
    // RC_ptr = load RC pointer from shared pointer structure
    // if RC_ptr is not null:
    //     increment RC value
    //     store incremented RC value
    // continue

    // RC_ptr = load RC pointer from shared pointer structure
    llvm::Value* RCPtr = this->llvm_ir_builder.CreateLoad(this->ll_pointer, this->llvm_ir_builder.CreateStructGEP(this->ll_shared_ptr, value, 1));
    // if RC_ptr is not null:
    llvm::Value* isNull = this->llvm_ir_builder.CreateIsNotNull(RCPtr);
    //     branch to increment or continuation based on the null check
    llvm::Function* func = this->llvm_ir_builder.GetInsertBlock()->getParent();
    llBB* increment = llBB::Create(this->llvm_context, "increment", func);
    llBB* conti = llBB::Create(this->llvm_context, "conti", func);
    this->llvm_ir_builder.CreateCondBr(isNull, increment, conti);
    //     RC_val = load current RC value
    this->llvm_ir_builder.SetInsertPoint(increment);
    auto RC_val = this->llvm_ir_builder.CreateLoad(this->ll_int, RCPtr);
    //     increment RC value
    auto incremented = this->llvm_ir_builder.CreateAdd(RC_val, llConstInt::get(this->ll_int, 1));
    //     store incremented RC value
    this->llvm_ir_builder.CreateStore(incremented, RCPtr);
    // continue
    this->llvm_ir_builder.CreateBr(conti);
    this->llvm_ir_builder.SetInsertPoint(conti);
}

void Compiler::_decrementRC(StructTypePtr type, llvm::Value* value) {
    // Pseudocode:
    // RC_ptr = load RC pointer from shared pointer structure
    // RC_value = load current RC value
    // if RC_ptr is not null:
    //     decrement RC value
    //     store decremented RC value
    //     if decremented RC value is zero:
    //         free memory
    // continue

    // RC_ptr = load RC pointer from shared pointer structure
    llvm::Value* RCPtr = this->llvm_ir_builder.CreateLoad(this->ll_pointer, this->llvm_ir_builder.CreateStructGEP(this->ll_shared_ptr, value, 1));
    // RC_value = load current RC value
    auto RC_value = this->llvm_ir_builder.CreateLoad(this->ll_int, RCPtr);
    // RC_ptr is not null
    auto is_null = this->llvm_ir_builder.CreateIsNotNull(RCPtr);


    auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();
    // if RC_ptr is not null:
    llBB* decrement = llBB::Create(this->llvm_context, "decrement", func);
    llBB* FreeBB = llBB::Create(this->llvm_context, "free", func);
    llBB* conti = llBB::Create(this->llvm_context, "conti", func);
    this->llvm_ir_builder.CreateCondBr(is_null, decrement, conti);
    this->llvm_ir_builder.SetInsertPoint(decrement);
    //     decrement RC value
    auto decremented = this->llvm_ir_builder.CreateSub(RC_value, llConstInt::get(this->ll_int, 1));
    //     store decremented RC value
    this->llvm_ir_builder.CreateStore(decremented, RCPtr);
    // decremented RC value is zero
    auto is_zero = this->llvm_ir_builder.CreateICmpEQ(decremented, llConstInt::get(this->ll_int, 0));
    //     if decremented RC value is zero:
    this->llvm_ir_builder.CreateCondBr(is_zero, FreeBB, conti);
    this->llvm_ir_builder.SetInsertPoint(FreeBB);
    //         free memory
    auto struct_val_pointer = llvm_ir_builder.CreateLoad(this->ll_pointer, this->llvm_ir_builder.CreateStructGEP(this->ll_shared_ptr, value, 0));
    if (type->gc_struct_clear)
        this->llvm_ir_builder.CreateCall(type->gc_struct_clear->function, {struct_val_pointer});
    this->llvm_ir_builder.CreateCall(this->env->getFunction("free", {this->env->getStruct("int")})->function, {struct_val_pointer});
    this->llvm_ir_builder.CreateCall(env->getFunction("free", {this->env->getStruct("int")})->function, {RCPtr});
    // continue
    this->llvm_ir_builder.CreateBr(conti);
    this->llvm_ir_builder.SetInsertPoint(conti);
}

void Compiler::compile(shared_ptr<AST::Node> node) {
    // This Only Compiles Statements & expressions are compiled in `_resolveValue
    switch (node->type()) {
        case AST::NodeType::Program:
            this->_visitProgram(node->castToProgram());
            break;
        case AST::NodeType::ExpressionStatement:
            this->_visitExpressionStatement(node->castToExpressionStatement());
            break;
        case AST::NodeType::InfixedExpression:
            this->_visitInfixExpression(node->castToInfixExpression());
            break;
        case AST::NodeType::IndexExpression:
            this->_visitIndexExpression(node->castToIndexExpression());
            break;
        case AST::NodeType::VariableDeclarationStatement:
            this->_visitVariableDeclarationStatement(node->castToVariableDeclarationStatement());
            break;
        case AST::NodeType::VariableAssignmentStatement:
            this->_visitVariableAssignmentStatement(node->castToVariableAssignmentStatement());
            break;
        case AST::NodeType::IfElseStatement:
            this->_visitIfElseStatement(node->castToIfElseStatement());
            break;
        case AST::NodeType::FunctionStatement:
            this->_visitFunctionDeclarationStatement(node->castToFunctionStatement());
            break;
        case AST::NodeType::CallExpression:
            this->_visitCallExpression(node->castToCallExpression());
            break;
        case AST::NodeType::ReturnStatement:
            this->_visitReturnStatement(node->castToReturnStatement());
            break;
        case AST::NodeType::RaiseStatement:
            this->_visitRaiseStatement(node->castToRaiseStatement());
            break;
        case AST::NodeType::TryCatchStatement:
            this->_visitTryCatchStatement(node->castToTryCatchStatement());
            break;
        case AST::NodeType::BlockStatement:
            this->_visitBlockStatement(node->castToBlockStatement());
            break;
        case AST::NodeType::WhileStatement:
            this->_visitWhileStatement(node->castToWhileStatement());
            break;
        case AST::NodeType::ForStatement:
            this->_visitForStatement(node->castToForStatement());
            break;
        case AST::NodeType::BreakStatement:
            this->_visitBreakStatement(node->castToBreakStatement());
            break;
        case AST::NodeType::ContinueStatement:
            this->_visitContinueStatement(node->castToContinueStatement());
            break;
        case AST::NodeType::StructStatement:
            this->_visitStructStatement(node->castToStructStatement());
            break;
        case AST::NodeType::ImportStatement:
            this->_visitImportStatement(node->castToImportStatement());
            break;
        default:
            errors::CompletionError("Unknown node type",
                                    this->source,
                                    node->meta_data.st_line_no,
                                    node->meta_data.end_line_no,
                                    "Unknown node type: " + AST::nodeTypeToString(node->type()))
                .raise();
    }
}

void Compiler::_visitBreakStatement(shared_ptr<AST::BreakStatement> node) {
    if (this->env->loop_conti_block.empty()) {
        errors::NodeOutside("Break outside loop", this->source, *node, errors::outsideNodeType::Break, "Break statement outside the Loop", "Remove the Break statement, it is not necessary").raise();
        exit(1);
    }
    if (node->loopIdx >= this->env->loop_ifbreak_block.size()) {
        errors::CompletionError("WrongLoopIdx", this->source, node->meta_data.st_line_no, node->meta_data.end_line_no, "Loop Index is out of range", "Remember: LoopIdx start with `0`").raise();
    }
    // break 0; == break; & .size() return 1 if it holds 1 iten not when it holds 2 thats why - 1
    if (this->env->loop_ifbreak_block.at(this->env->loop_ifbreak_block.size() - node->loopIdx - 1))
        // jumps to `ifbreak` block if it exists
        this->llvm_ir_builder.CreateBr(this->env->loop_ifbreak_block.at(this->env->loop_ifbreak_block.size() - node->loopIdx - 1));
    else this->llvm_ir_builder.CreateBr(this->env->loop_conti_block.at(this->env->loop_conti_block.size() - node->loopIdx - 1));
    throw DoneBr();
}

void Compiler::_visitContinueStatement(shared_ptr<AST::ContinueStatement> node) {
    if (this->env->loop_condition_block.empty()) {
        errors::NodeOutside("Continue outside loop",
                            this->source,
                            *node,
                            errors::outsideNodeType::Continue,
                            "Continue statement outside the Loop",
                            "Remove the Continue statement, it is not necessary")
            .raise();
        exit(1);
    }
    if (node->loopIdx >= this->env->loop_ifbreak_block.size()) {
        errors::CompletionError("WrongLoopIdx", this->source, node->meta_data.st_line_no, node->meta_data.end_line_no, "Loop Index is out of range", "Remember: LoopIdx start with `0`").raise();
    }
    // continue 0; == continue; & .size() return 1 if it holds 1 iten not when it holds 2 thats why - 1
    this->llvm_ir_builder.CreateBr(this->env->loop_condition_block.at(this->env->loop_condition_block.size() - node->loopIdx - 1));
    throw DoneBr();
}

void Compiler::_visitProgram(shared_ptr<AST::Program> program) {
    for (const auto& stmt : program->statements) { this->compile(stmt); }
}

void Compiler::_visitExpressionStatement(shared_ptr<AST::ExpressionStatement> expression_statement) {
    this->compile(expression_statement->expr);
}

void Compiler::_visitBlockStatement(shared_ptr<AST::BlockStatement> block_statement) {
    for (const auto& stmt : block_statement->statements) { this->compile(stmt); }
}

void Compiler::_checkAndConvertCallType(FunctionPtr func_record, shared_ptr<AST::CallExpression> func_call, vector<llvm::Value*>& args, const vector<StructTypePtr>& params_types) {
    vector<unsigned short> mismatches;
    for (const auto& [idx, pt, pst] : llvm::enumerate(func_record->arguments, params_types)) {
        auto expected_type = std::get<1>(pt);
        if (_checkType(expected_type, pst)) {
            // Type Is Same So No need to do any thing
        } else if (this->canConvertType(pst, expected_type)) {
            args[idx] = this->convertType({args[idx], nullptr, pst}, expected_type).value;
        } else {
            mismatches.push_back(idx);
        }
    }
    if (!mismatches.empty()) { errors::NoOverload(this->source, {mismatches}, func_call, "Cannot call the function with wrong type").raise(); }
}

Compiler::ResolvedValue Compiler::_CallGfunc(
    const vector<GenericFunctionPtr>& gfuncs, const shared_ptr<AST::CallExpression> func_call, const Str& name, vector<llvm::Value*>& args, const vector<StructTypePtr>& params_types) {
    // Attempt to find and call a matching function overload
    for (const auto& gfunc : gfuncs) {
        if (gfunc->env->isFunction(name, params_types, false, true)) {
            // Match found: Retrieve function record
            auto func_record = gfunc->env->getFunction(name, params_types, false, true);

            // Validate and convert argument types as necessary
            this->_checkAndConvertCallType(func_record, func_call, args, params_types);

            // Create LLVM call instruction
            auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, args, name + "_result");

            // Return the resolved value
            return {returnValue, nullptr, func_record->return_type, resolveType::StructInst, false};
        }
    }

    // No exact match found; prepare to handle generic functions
    auto prev_env = this->env;                 // Save current environment
    vector<vector<unsigned short>> mismatches; // Track parameter mismatches

    for (const auto& gfunc : gfuncs) {
        // Create a new environment for each generic function
        this->env = std::make_shared<Enviornment>(prev_env, StrRecordMap{}, name);
        vector<unsigned short> mismatch_indices;

        // Iterate over parameters to check type compatibility
        for (const auto& [idx, gparam] : llvm::enumerate(gfunc->func->parameters)) {
            const auto& pparam = params_types[idx];

            // Resolve the type of the generic parameter
            auto gparam_resolved = this->_resolveValue(gparam->value_type->name);
            auto gparam_type = gparam_resolved.type;
            auto gparam_variant = std::get<StructTypePtr>(gparam_resolved.variant);

            // Handle identifier literals by creating a new struct record
            if (gparam->value_type->name->type() == AST::NodeType::IdentifierLiteral) {
                auto struct_record = std::make_shared<RecordStructType>(*pparam);
                struct_record->name = gparam->value_type->name->castToIdentifierLiteral()->value;
                this->env->addRecord(struct_record);
            }
            // Check for exact type match or convertible types
            else if (gparam_type == resolveType::StructType && _checkType(gparam_variant, pparam)) {
                // Exact type match; no action needed
            } else if (gparam_type == resolveType::StructType && this->canConvertType(gparam_variant, pparam)) {
                // Convertible type; no action needed
            } else {
                // Type mismatch; record the index
                mismatch_indices.push_back(idx);
            }
        }

        // If there are mismatches, record and continue to next generic function
        if (!mismatch_indices.empty()) {
            mismatches.push_back(mismatch_indices);
            continue;
        }

        // All parameters match; proceed to create and compile the function
        auto body = gfunc->func->body;
        auto params = gfunc->func->parameters;

        vector<Str> param_names;
        vector<llvm::Type*> llvm_param_types;
        vector<StructTypePtr> param_struct_types;
        vector<bool> param_references;

        // Prepare LLVM parameter types and names
        for (size_t i = 0; i < params.size(); ++i) {
            auto param = params[i];
            auto param_type = params_types[i];
            param_names.push_back(param->name->castToIdentifierLiteral()->value);
            param_struct_types.push_back(param_type);
            param_references.push_back(param->value_type->refrence);
            llvm_param_types.push_back(param_type->struct_type || name == "array" ? this->ll_shared_ptr : param_type->stand_alone_type);
        }

        // Determine the LLVM return type
        auto return_type = this->_parseType(gfunc->func->return_type);
        auto llvm_return_type = return_type->struct_type || return_type->name == "array" ? this->ll_shared_ptr : return_type->stand_alone_type;

        // Create LLVM function type and function
        auto func_type = llvm::FunctionType::get(llvm_return_type, llvm_param_types, false);
        auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, this->fc_st_name_prefix != "main.gc.." ? this->fc_st_name_prefix + name : name, this->llvm_module.get());

        // Update IR-GC mapping
        this->ir_gc_map_json["functions"][name] = func->getName().str();

        // Assign names to LLVM function arguments
        for (const auto& [idx, arg] : llvm::enumerate(func->args())) { arg.setName(param_names[idx]); }

        // Create a record for the new function
        auto func_record = std::make_shared<RecordFunction>(name, func, func_type, std::vector<std::tuple<Str, StructTypePtr, bool>>(), return_type, gfunc->func->extra_info);
        func_record->env = this->env;

        if (body) {
            // Create entry basic block for the function
            auto bb = llBB::Create(this->llvm_context, "entry", func);
            this->function_entry_block.push_back(bb);
            this->llvm_ir_builder.SetInsertPoint(bb);
            this->env->current_function = func_record.get();

            // Allocate and store function arguments
            for (const auto& [idx, arg] : llvm::enumerate(func->args())) {
                auto param_type = param_struct_types[idx];

                // Create alloca for the argument if necessary
                llvm::Value* alloca = &arg;
                if (!arg.getType()->isPointerTy() || _checkType(param_type, this->env->getStruct("array"))) {
                    alloca = this->llvm_ir_builder.CreateAlloca(arg.getType(), nullptr, arg.getName() + "_og");
                    this->llvm_ir_builder.CreateStore(&arg, alloca);
                }

                // Load argument value if it's a reference
                llvm::Value* arg_value = param_references[idx] ? this->llvm_ir_builder.CreateLoad(param_type->stand_alone_type, &arg, "loaded_" + arg.getName()) : alloca;

                // Create and add a record for the argument variable
                auto record = std::make_shared<RecordVariable>(Str(arg.getName()), arg_value, alloca, param_type);
                func_record->arguments.emplace_back(arg.getName().str(), param_type, param_references[idx]);
                this->env->addRecord(record);
            }

            // Set metadata for the function record
            func_record->set_meta_data(gfunc->func->meta_data.st_line_no, gfunc->func->meta_data.st_col_no, gfunc->func->meta_data.end_line_no, gfunc->func->meta_data.end_col_no);

            // Add the function record to the environment
            this->env->addRecord(func_record);

            try {
                // Compile the function body
                this->compile(body);
            } catch (DoneRet) {
                // Handle return statement properly
            } catch (DoneBr) {
                // Handle branch statements properly
            }

            // Add the function record to the generic function's environment
            gfunc->env->addRecord(func_record);
            this->function_entry_block.pop_back();

            // Restore insert point if there are outer blocks
            if (!this->function_entry_block.empty()) { this->llvm_ir_builder.SetInsertPoint(this->function_entry_block.back()); }
        } else {
            // If no body, record the function arguments
            for (const auto& [idx, arg] : llvm::enumerate(func->args())) { func_record->arguments.emplace_back(arg.getName().str(), param_struct_types[idx], param_references[idx]); }

            // Add the function record to the generic function's environment
            gfunc->env->addRecord(func_record);
        }

        // Validate and convert call types before making the call
        this->_checkAndConvertCallType(func_record, func_call, args, params_types);

        // Handle identifier literals by updating struct names
        for (size_t i = 0; i < gfunc->func->parameters.size(); ++i) {
            const auto& gparam = gfunc->func->parameters[i];
            const auto& pparam = params_types[i];

            if (gparam->value_type->name->type() == AST::NodeType::IdentifierLiteral) {
                auto name_literal = gparam->value_type->name->castToIdentifierLiteral()->value;
                if (this->env->isStruct(name_literal)) {
                    auto struct_record = this->env->getStruct(name_literal);
                    struct_record->name = pparam->name;
                }
            }
        }

        // Create LLVM call instruction to the newly created function
        auto returnValue = this->llvm_ir_builder.CreateCall(func, args, name + "_result");

        // Restore the previous environment
        this->env = prev_env;

        // Return the resolved value
        return {returnValue, nullptr, func_record->return_type, resolveType::StructInst, false};
    }

    // Handle cases where no overload matches
    if (mismatches.empty()) {
        // No function exists with the given name and parameter types
        errors::NoOverload(this->source, mismatches, func_call, "Function does not exist.", "Check the function name or define the function.").raise();
    } else {
        // Argument types do not match any overload
        errors::NoOverload(this->source, mismatches, func_call, "Argument types do not match any overload.", "Check the argument types or define an appropriate overload.").raise();
    }
};

void Compiler::_createFunctionRecord(ASTFunctionStatementPtr function_declaration_statement, StructTypePtr struct_, std::shared_ptr<RecordModule> module, const Json& ir_gc_map_json) {
    // Extract the function name from the AST
    auto name = function_declaration_statement->name->castToIdentifierLiteral()->value;

    // Handle generic functions separately
    if (!function_declaration_statement->generic.empty()) {
        auto gsr = std::make_shared<RecordGenericFunction>(name, function_declaration_statement, this->env);

        if (module) {
            // If within a module, add to the module's record map
            module->record_map.push_back({name, gsr});
        } else if (struct_) {
            // Generics are not supported for struct members; raise an error
            errors::CompletionError("GenericInMethod", this->source, gsr->meta_data.st_line_no, gsr->meta_data.end_line_no, "Generics do not support struct members").raise();
        } else {
            // Otherwise, add to the global environment
            this->env->addRecord(gsr);
        }
        return; // Early exit for generic functions
    }

    // Parse function parameters
    auto params = function_declaration_statement->parameters;
    vector<Str> param_names;
    vector<llvm::Type*> param_types;
    vector<std::tuple<Str, StructTypePtr, bool>> arguments;

    for (const auto& param : params) {
        // Extract parameter name
        auto param_name = param->name->castToIdentifierLiteral()->value;
        param_names.push_back(param_name);

        // Parse parameter type
        StructTypePtr param_type = this->_parseType(param->value_type);
        llvm::Type* llvm_param_type = param_type->stand_alone_type
            ? param_type->stand_alone_type
            : (param->value_type->refrence ? llvm::PointerType::getUnqual(param_type->stand_alone_type) : param_type->struct_type->getPointerTo());
        param_types.push_back(llvm_param_type);

        // Store argument details for later use
        arguments.emplace_back(param_name, param_type, param->value_type->refrence);
    }

    // Determine the return type of the function
    StructTypePtr return_type = function_declaration_statement->return_type ? this->_parseType(function_declaration_statement->return_type) : this->env->getStruct("void");
    llvm::Type* llvm_return_type = return_type->struct_type ? this->ll_shared_ptr : return_type->stand_alone_type;

    // Create the LLVM function type
    auto func_type = llvm::FunctionType::get(llvm_return_type, param_types, false);

    // Prefix the function name if necessary
    Str prefixed_name = (this->fc_st_name_prefix != "main.gc..") ? this->fc_st_name_prefix + name : name;

    // Create the LLVM function and add it to the module
    auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, prefixed_name, this->llvm_module.get());

    // Assign names to the function arguments for readability
    size_t idx = 0;
    for (auto& arg : func->args()) { arg.setName(param_names[idx++]); }

    // Create a RecordFunction to keep track of the function's metadata and environment
    auto func_record = std::make_shared<RecordFunction>(name, func, func_type, arguments, return_type, function_declaration_statement->extra_info);

    // Add the function record to the appropriate scope (struct, module, or global environment)
    if (struct_) {
        struct_->methods.emplace_back(name, func_record);
        this->ir_gc_map_json["structs"][struct_->name]["methods"][name] = func->getName().str();
    } else if (module) {
        module->record_map.emplace_back(name, func_record);
    } else {
        this->env->addRecord(func_record);
        this->ir_gc_map_json["functions"][name] = func->getName().str();
    }

    // If the function has a body, proceed to compile it
    if (auto body = function_declaration_statement->body) {
        // Create the entry basic block for the function
        auto bb = llvm::BasicBlock::Create(this->llvm_context, "entry", func);
        this->function_entry_block.push_back(bb);
        this->llvm_ir_builder.SetInsertPoint(bb);

        // Save the current environment and create a new one for the function
        auto prev_env = this->env;
        this->env = std::make_shared<Enviornment>(prev_env, StrRecordMap{}, name);
        func_record->env = this->env;
        this->env->current_function = func_record.get();

        // Initialize function arguments in the new environment
        size_t arg_idx = 0;
        for (auto& arg : func->args()) {
            const auto& argument = arguments[arg_idx++];
            Str arg_name = std::get<0>(argument);
            StructTypePtr param_type_record = std::get<1>(argument);
            bool is_reference = std::get<2>(argument);

            llvm::Value* alloca = &arg;
            // Allocate space for the argument if it's not a pointer type or if it's an array
            if (!arg.getType()->isPointerTy() || _checkType(param_type_record, this->env->getStruct("array"))) {
                alloca = this->llvm_ir_builder.CreateAlloca(arg.getType(), nullptr, arg.getName());
                this->llvm_ir_builder.CreateStore(&arg, alloca);
            }

            // Load the argument value if it's passed by reference
            llvm::Value* arg_value = &arg;
            if (is_reference) { arg_value = this->llvm_ir_builder.CreateLoad(param_type_record->stand_alone_type ? param_type_record->stand_alone_type : param_type_record->struct_type, &arg); }

            // Create a variable record for the argument and add it to the environment
            auto record = std::make_shared<RecordVariable>(Str(arg.getName()), arg_value, alloca, param_type_record);
            this->env->addRecord(record);
        }

        // Set metadata for the function (e.g., source code location)
        func_record->set_meta_data(function_declaration_statement->meta_data.st_line_no,
                                   function_declaration_statement->meta_data.st_col_no,
                                   function_declaration_statement->meta_data.end_line_no,
                                   function_declaration_statement->meta_data.end_col_no);

        // Compile the function body within a try-catch to handle early returns or branches
        try {
            this->compile(body);

            // Ensure the function ends properly based on its return type
            if (return_type->name != "void") {
                this->llvm_ir_builder.CreateUnreachable();
            } else {
                this->llvm_ir_builder.CreateRetVoid();
            }
        } catch (DoneRet) {
            // Handle explicit return statements
        } catch (DoneBr) {
            // Handle branch statements
        }

        // Restore the previous environment after compiling the function
        this->env = prev_env;
        this->function_entry_block.pop_back();

        // Reset the insert point to the previous basic block if any
        if (!this->function_entry_block.empty()) { this->llvm_ir_builder.SetInsertPoint(this->function_entry_block.back()); }
    }
}

Compiler::ResolvedValue Compiler::_callStruct(StructTypePtr struct_record, shared_ptr<AST::CallExpression> call_expression, vector<StructTypePtr> params_types, LLVMValueVector args) {
    auto name = struct_record->name;
    auto struct_type = struct_record->struct_type;

    // Allocate space for the shared pointer
    llvm::Value* alloca = this->llvm_ir_builder.CreateAlloca(this->ll_shared_ptr, nullptr, name);

    // Lambda to handle initialization for new struct instances
    auto initializeNewStruct = [&](llvm::Value* allocaPtr) {
        // Get pointer to the value field
        auto value_gep = this->llvm_ir_builder.CreateGEP(this->ll_shared_ptr, allocaPtr, llConstInt::get(this->ll_int, 0));

        // Calculate size of the struct
        llvm::Value* gep = this->llvm_ir_builder.CreateGEP(struct_type, llvm::ConstantPointerNull::get(struct_type->getPointerTo()), llConstInt::get(llvm::Type::getInt64Ty(this->llvm_context), 1));
        llvm::Value* size = this->llvm_ir_builder.CreatePtrToInt(gep, llvm::Type::getInt64Ty(this->llvm_context));

        // Allocate memory for the struct
        auto struct_alloca = this->llvm_ir_builder.CreateCall(this->env->getFunction("malloc", {this->env->getStruct("int")})->function, {size});
        this->llvm_ir_builder.CreateStore(struct_alloca, value_gep);

        // Allocate and initialize reference count
        auto RC_alloca =
            this->llvm_ir_builder.CreateCall(this->env->getFunction("malloc", {this->env->getStruct("int")})->function, {llConstInt::get(this->ll_int, this->ll_int->getScalarSizeInBits())});
        this->llvm_ir_builder.CreateStore(llConstInt::get(this->ll_int, 1), RC_alloca);
        auto RC_gep = this->llvm_ir_builder.CreateGEP(this->ll_shared_ptr, allocaPtr, llConstInt::get(this->ll_int, 1));
        this->llvm_ir_builder.CreateStore(RC_alloca, RC_gep);
    };

    // Lambda to handle initialization for existing struct instances
    auto initializeExistingStruct = [&](llvm::Value* allocaPtr) {
        // Allocate space for the struct
        auto struct_alloca = this->llvm_ir_builder.CreateAlloca(struct_type, nullptr, name);

        // Get pointer to the value field and store the struct
        auto value_gep = this->llvm_ir_builder.CreateGEP(this->ll_shared_ptr, allocaPtr, llConstInt::get(this->ll_int, 0));
        this->llvm_ir_builder.CreateStore(struct_alloca, value_gep);

        // Initialize reference count to null
        auto RC_gep = this->llvm_ir_builder.CreateGEP(this->ll_shared_ptr, allocaPtr, llConstInt::get(this->ll_int, 1));
        this->llvm_ir_builder.CreateStore(llvm::ConstantPointerNull::get(this->ll_pointer), RC_gep);
    };

    // Initialize based on whether it's a new struct instance
    if (call_expression->_new) {
        initializeNewStruct(alloca);
    } else {
        initializeExistingStruct(alloca);
    }

    // Prepare parameters and arguments for the __init__ method
    params_types.insert(params_types.begin(), struct_record);
    args.insert(args.begin(), this->llvm_ir_builder.CreateLoad(this->ll_shared_ptr, alloca));

    // Retrieve the __init__ method
    auto func = struct_record->get_method("__init__", params_types);

    if (func) {
        // Validate and convert argument types
        this->_checkAndConvertCallType(func, call_expression, args, params_types);
        // Call the __init__ method
        this->llvm_ir_builder.CreateCall(func->function, args);
    } else {
        // Raise an error if __init__ is not defined
        errors::NoOverload(this->source,
                           {},
                           call_expression,
                           "Initialization method does not exist for struct " + struct_record->name + ".",
                           "Check the initialization method name or define the method.")
            .raise();
        exit(1);
    }

    // Return the resolved value indicating a struct instance
    return {alloca, alloca, struct_record, resolveType::StructInst, true};
}

void Compiler::_createClearMethod(StructTypePtr struct_record) {
    // Retrieve the struct's name
    auto struct_name = struct_record->name;

    // Define the return type and parameter types for the clear function
    llvm::Type* return_type = llvm::Type::getVoidTy(this->llvm_context);
    std::vector<llvm::Type*> param_types = {this->ll_pointer};

    // Create the function type: void clear(pointer)
    auto clear_func_type = llvm::FunctionType::get(return_type, param_types, false);

    // Generate the clear function with external linkage
    std::string func_name = struct_name + "__clear__";
    auto clear_func = llvm::Function::Create(clear_func_type, llvm::Function::ExternalLinkage, func_name, this->llvm_module.get());

    // Create the entry basic block for the clear function
    auto entry_bb = llvm::BasicBlock::Create(this->llvm_context, "entry", clear_func);
    this->llvm_ir_builder.SetInsertPoint(entry_bb);

    // Access the first argument of the clear function (the struct pointer)
    llvm::Value* struct_ptr = clear_func->arg_begin();

    // Iterate over each field in the struct
    const auto& fields = struct_record->getFields();
    for (const auto& field_name : fields) {
        auto field_type = struct_record->sub_types[field_name];

        // Check if the field is a struct or an array
        bool is_struct_or_array = field_type->struct_type || field_type->name == "array";
        if (!is_struct_or_array) continue;

        // Calculate the pointer to the field within the struct
        llvm::Value* gep = this->llvm_ir_builder.CreateStructGEP(struct_record->struct_type, struct_ptr, 1, field_name.c_str());

        // Load the field's value
        llvm::Value* field = this->llvm_ir_builder.CreateLoad(this->ll_shared_ptr, gep, field_name.c_str());

        // Decrement the reference count of the field
        this->_decrementRC(field_type, field);
    }

    // Insert the return void instruction
    this->llvm_ir_builder.CreateRetVoid();

    // Associate the clear function with the struct's clear method in the environment
    struct_record->gc_struct_clear = std::make_shared<RecordFunction>("__clear__", clear_func, clear_func_type, std::vector<std::tuple<Str, StructTypePtr, bool>>(), this->env->getStruct("void"));
}

Compiler::ResolvedValue Compiler::_CallGstruct(
    const vector<GenericStructTypePtr>& gstructs, const shared_ptr<AST::CallExpression> func_call, const Str& name, vector<llvm::Value*>& args, const vector<StructTypePtr>& params_types) {
    auto prev_env = this->env; // Save the current environment

    for (const auto& gstruct : gstructs) {
        // Skip if there aren't enough parameters for generics
        if (params_types.size() < gstruct->structAST->generics.size()) { continue; }

        // Set up a new environment based on the generic struct's environment
        this->env = std::make_shared<Enviornment>(gstruct->env);
        vector<StructTypePtr> generics;
        vector<Str> generic_names;
        vector<llvm::Value*> remaining_args(args);
        vector<StructTypePtr> remaining_params_types(params_types);

        // Map provided parameter types to the struct's generics
        auto generic_iter = llvm::zip(params_types, gstruct->structAST->generics);
        for (const auto& [pt, generic] : generic_iter) {
            auto pt_copy = std::make_shared<RecordStructType>(*pt);
            generic_names.push_back(pt_copy->name);
            pt_copy->name = generic->name->castToIdentifierLiteral()->value;
            this->env->addRecord(pt_copy);
            generics.push_back(pt_copy);
            remaining_args.erase(remaining_args.begin());
            remaining_params_types.erase(remaining_params_types.begin());
        }

        Str struct_name = gstruct->structAST->name->castToIdentifierLiteral()->value;
        auto struct_record = std::make_shared<RecordStructType>(struct_name);

        // Check if the struct with the given generics already exists
        if (!gstruct->env->isStruct(struct_name, false, generics)) {
            // Initialize struct fields
            vector<llvm::Type*> field_types;
            auto fields = gstruct->structAST->fields;
            this->env->addRecord(struct_record);

            for (const auto& field : fields) {
                if (field->type() == AST::NodeType::VariableDeclarationStatement) {
                    // Handle variable declarations within the struct
                    auto field_decl = field->castToVariableDeclarationStatement();
                    Str field_name = field_decl->name->castToIdentifierLiteral()->value;
                    auto field_type = this->_parseType(field_decl->value_type);

                    // Determine the LLVM type based on whether it's a standalone type or an array
                    llvm::Type* llvm_field_type = (field_type->stand_alone_type || field_type->name == "array") ? this->ll_shared_ptr : field_type->stand_alone_type;
                    field_types.push_back(llvm_field_type);
                    struct_record->addSubType(field_name, field_type);

                    // Check if the field type is a generic type
                    if (field_decl->value_type->type() == AST::NodeType::IdentifierLiteral) {
                        auto field_value = field_decl->value_type->name->castToIdentifierLiteral()->value;
                        bool is_generic = false;

                        for (const auto& generic : gstruct->structAST->generics) {
                            if (field_value == generic->name->castToIdentifierLiteral()->value) {
                                is_generic = true;
                                break;
                            }
                        }

                        if (is_generic) { struct_record->generic_sub_types.push_back(field_type); }
                    }

                    // Create and set the LLVM struct type
                    auto struct_type = llvm::StructType::create(this->llvm_context, field_types, this->fc_st_name_prefix + struct_name);
                    struct_type->setBody(field_types);
                    struct_record->struct_type = struct_type;
                } else {
                    // Handle function declarations within the struct
                    auto func_dec = field->castToFunctionStatement();

                    // Ensure that struct methods do not have generic functions
                    if (!func_dec->generic.empty()) {
                        errors::CompletionError("Doesn't Support",
                                                this->source,
                                                func_dec->meta_data.st_line_no,
                                                func_dec->meta_data.end_line_no,
                                                "Struct Methods do not support Generic Functions",
                                                "Set the Generic on the struct")
                            .raise();
                        exit(1);
                    }

                    // Visit and process the function declaration
                    this->_visitFunctionDeclarationStatement(func_dec, struct_record);
                }
            }

            // Create a clear method for the struct
            this->_createClearMethod(struct_record);
        } else {
            // Retrieve the existing struct record with the specified generics
            struct_record = gstruct->env->getStruct(struct_name, false, generics);
        }

        // Add the struct record to the generic struct's environment and restore the previous environment
        gstruct->env->addRecord(struct_record);
        this->env = prev_env;

        // Perform the struct call and return the resolved value
        return _callStruct(struct_record, func_call, params_types, args);
    }

    // If no matching struct overload is found, raise an error and exit
    errors::NoOverload(this->source, {}, func_call, "Struct overload does not exist.", "Check the argument types or define an appropriate overload, first pass types & then init function params.")
        .raise();
    exit(1);
};

void Compiler::_createStructRecord(ASTStructStatementPtr struct_statement, std::shared_ptr<RecordModule> module, const Json& ir_gc_map_json) {
    // Extract the struct name
    Str struct_name = struct_statement->name->castToIdentifierLiteral()->value;

    // Initialize JSON entries for the struct
    this->ir_gc_map_json["structs"][struct_name] = Json::object();
    this->ir_gc_map_json["structs"][struct_name]["methods"] = Json::object();

    // Create a new record for the struct
    auto struct_record = std::make_shared<RecordStructType>(struct_name);

    // Handle generic structs early and return
    if (!struct_statement->generics.empty()) {
        auto gsr = std::make_shared<RecordGenericStructType>(struct_name, struct_statement, this->env);
        this->env->addRecord(gsr);
        if (module) { module->record_map.emplace_back(struct_name, gsr); }
        return;
    }

    // Prepare to parse struct fields
    vector<llvm::Type*> field_types;
    auto fields = struct_statement->fields;

    // Save and update the environment for struct scope
    auto prev_env = this->env;
    this->env = std::make_shared<Enviornment>(prev_env);
    this->env->addRecord(struct_record);

    // Iterate over each field in the struct
    for (auto field : fields) {
        if (field->type() == AST::NodeType::VariableDeclarationStatement) {
            // Process variable declaration fields
            this->_processFieldDeclaration(field, struct_statement, struct_record, field_types);
        } else {
            // Process function declarations within the struct
            this->_processFieldFunction(field, struct_record, ir_gc_map_json);
        }
    }

    // Restore the previous environment
    this->env = prev_env;

    // Add the struct record to the module or environment
    if (module) {
        module->record_map.emplace_back(struct_name, struct_record);
    } else {
        this->env->addRecord(struct_record);
    }

    // Create a clear method for the struct
    this->_createClearMethod(struct_record);
}

void Compiler::_processFieldDeclaration(AST::NodePtr field, ASTStructStatementPtr struct_statement, std::shared_ptr<RecordStructType> struct_record, vector<llvm::Type*>& field_types) {
    // Extract field name
    Str field_name = field->castToVariableDeclarationStatement()->name->castToIdentifierLiteral()->value;

    // Parse the field type
    auto field_type = this->_parseType(field->castToVariableDeclarationStatement()->value_type);

    // Determine the LLVM type to use
    llvm::Type* llvm_field_type = (field_type->struct_type || field_type->name == "array") ? this->ll_shared_ptr : field_type->stand_alone_type;

    // Add the LLVM type to the field types vector
    field_types.push_back(llvm_field_type);

    // Add the subtype to the struct record
    struct_record->addSubType(field_name, field_type);

    // Handle generic subtypes if applicable
    this->_handleGenericSubType(field, struct_statement, struct_record, field_type);
}

void Compiler::_handleGenericSubType(AST::NodePtr field, ASTStructStatementPtr struct_statement, std::shared_ptr<RecordStructType> struct_record, StructTypePtr field_type) {
    auto value_type = field->castToVariableDeclarationStatement()->value_type;

    if (value_type->type() == AST::NodeType::IdentifierLiteral) {
        Str field_value = value_type->castToIdentifierLiteral()->value;
        bool is_generic = false;

        // Check if the field type is a generic parameter
        for (const auto& generic : struct_statement->generics) {
            if (field_value == generic->name->castToIdentifierLiteral()->value) {
                is_generic = true;
                break;
            }
        }

        // Add to generic subtypes if applicable
        if (is_generic) { struct_record->generic_sub_types.push_back(field_type); }
    }
}

void Compiler::_processFieldFunction(AST::NodePtr field, std::shared_ptr<RecordStructType> struct_record, const Json& ir_gc_map_json) {
    auto func_dec = field->castToFunctionStatement();

    // Ensure the function is not generic
    if (!func_dec->generic.empty()) {
        errors::CompletionError("Doesn't Support",
                                this->source,
                                func_dec->meta_data.st_line_no,
                                func_dec->meta_data.end_line_no,
                                "Struct Methods do not support Generic Functions",
                                "Set the Generic on the struct")
            .raise();
        exit(1);
    }

    // Create the function record within the struct
    this->_createFunctionRecord(func_dec, struct_record, nullptr, ir_gc_map_json);
}

void Compiler::_visitStructStatement(ASTStructStatementPtr struct_statement) {
    this->_createStructRecord(struct_statement, nullptr, this->ir_gc_map_json);
}

Compiler::ResolvedValue Compiler::convertType(const ResolvedValue& from, StructTypePtr to) {
    auto [fromloadedval, fromalloca, _fromtype, struct_type, inscope] = from;
    auto fromtype = std::get<StructTypePtr>(_fromtype);
    // If Types are Same
    if (_checkType(fromtype, to)) { return {fromloadedval, fromalloca, fromtype, resolveType::StructInst, inscope}; }

    if (fromtype->name == "int32" && to->name == "int") {
        auto int_type = this->llvm_ir_builder.CreateSExt(fromloadedval, this->env->getStruct("int")->stand_alone_type, "int32_to_int");
        return {int_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "int" && to->name == "int32") {
        auto int32_type = this->llvm_ir_builder.CreateTrunc(fromloadedval, this->env->getStruct("int32")->stand_alone_type, "int_to_int32");
        return {int32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "uint32" && to->name == "uint") {
        auto uint_type = this->llvm_ir_builder.CreateZExt(fromloadedval, this->env->getStruct("uint")->stand_alone_type, "uint32_to_uint");
        return {uint_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "uint" && to->name == "uint32") {
        auto uint32_type = this->llvm_ir_builder.CreateTrunc(fromloadedval, this->env->getStruct("uint32")->stand_alone_type, "uint_to_uint32");
        return {uint32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "float32" && to->name == "float") {
        auto float_type = this->llvm_ir_builder.CreateFPExt(fromloadedval, this->env->getStruct("float")->stand_alone_type, "float32_to_float");
        return {float_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "float" && to->name == "float32") {
        auto float32_type = this->llvm_ir_builder.CreateFPTrunc(fromloadedval, this->env->getStruct("float32")->stand_alone_type, "float_to_float32");
        return {float32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "int" && to->name == "float") {
        auto float_type = this->llvm_ir_builder.CreateSIToFP(fromloadedval, this->env->getStruct("float")->stand_alone_type, "int_to_float");
        return {float_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "int32" && to->name == "float32") {
        auto float32_type = this->llvm_ir_builder.CreateSIToFP(fromloadedval, this->env->getStruct("float32")->stand_alone_type, "int32_to_float32");
        return {float32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "uint" && to->name == "float") {
        auto float_type = this->llvm_ir_builder.CreateUIToFP(fromloadedval, this->env->getStruct("float")->stand_alone_type, "uint_to_float");
        return {float_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "uint32" && to->name == "float32") {
        auto float32_type = this->llvm_ir_builder.CreateUIToFP(fromloadedval, this->env->getStruct("float32")->stand_alone_type, "uint32_to_float32");
        return {float32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "float" && to->name == "int") {
        auto int_type = this->llvm_ir_builder.CreateFPToSI(fromloadedval, this->env->getStruct("int")->stand_alone_type, "float_to_int");
        return {int_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "float32" && to->name == "int32") {
        auto int32_type = this->llvm_ir_builder.CreateFPToSI(fromloadedval, this->env->getStruct("int32")->stand_alone_type, "float32_to_int32");
        return {int32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "float" && to->name == "uint") {
        auto uint_type = this->llvm_ir_builder.CreateFPToUI(fromloadedval, this->env->getStruct("uint")->stand_alone_type, "float_to_uint");
        return {uint_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "float32" && to->name == "uint32") {
        auto uint32_type = this->llvm_ir_builder.CreateFPToUI(fromloadedval, this->env->getStruct("uint32")->stand_alone_type, "float32_to_uint32");
        return {uint32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "bool" && to->name == "int") {
        auto int_type = this->llvm_ir_builder.CreateZExt(fromloadedval, this->env->getStruct("int")->stand_alone_type, "bool_to_int");
        return {int_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "bool" && to->name == "uint") {
        auto uint_type = this->llvm_ir_builder.CreateZExt(fromloadedval, this->env->getStruct("uint")->stand_alone_type, "bool_to_uint");
        return {uint_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "bool" && to->name == "float") {
        auto float_type = this->llvm_ir_builder.CreateUIToFP(fromloadedval, this->env->getStruct("float")->stand_alone_type, "bool_to_float");
        return {float_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "bool" && to->name == "float32") {
        auto float32_type = this->llvm_ir_builder.CreateUIToFP(fromloadedval, this->env->getStruct("float32")->stand_alone_type, "bool_to_float32");
        return {float32_type, fromalloca, to, resolveType::StructInst};
    } else if ((fromtype->name == "int" || fromtype->name == "float" || fromtype->name == "str") && to->name == "bool") {
        auto bool_type = this->llvm_ir_builder.CreateICmpNE(fromloadedval, llvm::Constant::getNullValue(fromloadedval->getType()), "to_bool");
        return {bool_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->struct_type) { // If struct & it is mark as auto cast
        if (fromtype->is_method("", {fromtype}, {{"autocast", true}}, to)) {
            auto method = fromtype->get_method("", {fromtype}, {{"autocast", true}}, to);
            auto returnValue = this->llvm_ir_builder.CreateCall(method->function, {fromalloca}, fromtype->name + "_to_" + to->name);
            return {returnValue, nullptr, method->return_type, resolveType::StructInst, false};
        }
    }

    shared_ptr<RecordModule> x = nullptr;
    return {nullptr, nullptr, x, resolveType::Module};
};

bool Compiler::canConvertType(StructTypePtr from, StructTypePtr to) {
    const vector<std::pair<Str, Str>> convertibleTypes = {{"int32", "int"},  {"int", "int32"},      {"uint32", "uint"}, {"uint", "uint32"},    {"float32", "float"}, {"float", "float32"},
                                                          {"int", "float"},  {"int32", "float32"},  {"uint", "float"},  {"uint32", "float32"}, {"float", "int"},     {"float32", "int32"},
                                                          {"float", "uint"}, {"float32", "uint32"}, {"bool", "int"},    {"bool", "uint"},      {"bool", "float"},    {"bool", "float32"},
                                                          {"int", "bool"},   {"float", "bool"},     {"str", "bool"}};

    for (const auto& [fromType, toType] : convertibleTypes) {
        if (from->name == fromType && to->name == toType) { return true; }
    }
    if (from->struct_type && from->is_method("", {from}, {{"autocast", true}}, to, true)) { return true; }
    return false;
};

bool Compiler::conversionPrecidence(StructTypePtr from, StructTypePtr to) {
    const vector<std::pair<Str, Str>> precedencePairs = {{"int32", "int"},
                                                         {"uint32", "int"},
                                                         {"uint", "int"},
                                                         {"uint32", "uint"},
                                                         {"float32", "float"},
                                                         {"int32", "float"},
                                                         {"uint32", "float"},
                                                         {"uint", "float"},
                                                         {"int", "float"},
                                                         {"int32", "float32"},
                                                         {"uint32", "float32"},
                                                         {"uint", "float32"},
                                                         {"int", "float32"}};

    for (const auto& pair : precedencePairs) {
        if (from->name == pair.first && to->name == pair.second) { return true; }
    }

    return false;
};


Compiler::ResolvedValue Compiler::_memberAccess(shared_ptr<AST::InfixExpression> infixed_expression) {
    auto left = infixed_expression->left;
    auto right = infixed_expression->right;
    auto [left_value, left_alloca, _left_type, ltt, leftInscope] = this->_resolveValue(left);
    if (right->type() == AST::NodeType::IdentifierLiteral) {
        if (ltt == resolveType::Module) {
            auto module = std::get<shared_ptr<RecordModule>>(_left_type);
            auto name = right->castToIdentifierLiteral()->value;
            if (module->is_module(name)) {
                return Compiler::ResolvedValue(nullptr, nullptr, module->get_module(name), resolveType::Module);
            } else if (module->is_struct(name)) {
                return Compiler::ResolvedValue(nullptr, nullptr, module->get_struct(name), resolveType::StructType);
            } else if (module->isGenericStruct(name)) {
                return Compiler::ResolvedValue(nullptr, nullptr, module->getGenericStruct(name), resolveType::GStructType);
            } else {
                errors::DosentContain(this->source,
                                      right->castToIdentifierLiteral(),
                                      left,
                                      "no member `" + name + "` not found in module " + module->name,
                                      "Check the Member Name in the module is correct")
                    .raise();
                exit(1);
            }
        } else if (ltt == resolveType::StructInst) {
            auto left_type = std::get<StructTypePtr>(_left_type);
            if (left_type->sub_types.contains(right->castToIdentifierLiteral()->value)) {
                unsigned short idx = 0;
                for (auto field : left_type->getFields()) {
                    if (field == right->castToIdentifierLiteral()->value) { break; }
                    idx++;
                }
                auto type = left_type->sub_types[right->castToIdentifierLiteral()->value];
                llvm::Value* gep = this->llvm_ir_builder.CreateStructGEP(left_type->struct_type,
                                                                         this->llvm_ir_builder.CreateLoad(this->ll_pointer, this->llvm_ir_builder.CreateStructGEP(this->ll_shared_ptr, left_alloca, 0)),
                                                                         idx,
                                                                         "accesed" + right->castToIdentifierLiteral()->value + "_from_" + left_type->name);
                if (type->struct_type || type->name == "array") {
                    this->_incrementRC(gep);
                    return {gep, gep, type, resolveType::StructInst, false};
                }
                llvm::Value* loaded = this->llvm_ir_builder.CreateLoad(type->stand_alone_type, gep, "loded" + right->castToIdentifierLiteral()->value);
                return {loaded, gep, type, resolveType::StructInst, false};
            } else {
                errors::DosentContain(this->source,
                                      right->castToIdentifierLiteral(),
                                      left,
                                      "no member `" + right->castToIdentifierLiteral()->value + "` not found in struct " + left_type->name,
                                      "Check the Member Name in the struct is correct")
                    .raise();
                exit(1);
            }
        }
    } else if (right->type() == AST::NodeType::CallExpression) {
        auto call_expression = right->castToCallExpression();
        auto name = call_expression->name->castToIdentifierLiteral()->value;
        auto params = call_expression->arguments;
        vector<llvm::Value*> args;
        vector<llvm::Value*> arg_allocas;
        vector<StructTypePtr> params_types;
        for (auto arg : params) {
            auto [value, val_alloca, param_type, ptt, paramInscope] = this->_resolveValue(arg);
            if (ptt == resolveType::Module) {
                errors::WrongType(this->source, arg, {}, "Cant pass Module to the Function").raise();
                exit(1);
            } else if (ptt == resolveType::StructType || ptt == resolveType::GStructType) {
                errors::WrongType(this->source, arg, {}, "Cant pass type to the Function").raise();
                exit(1);
            }
            if (std::get<StructTypePtr>(param_type)->struct_type || std::get<StructTypePtr>(param_type)->name == "array") {
                this->_incrementRC(value);
            }
            params_types.push_back(std::get<StructTypePtr>(param_type));
            arg_allocas.push_back(val_alloca);
            args.push_back(value);
        }
        if (ltt == resolveType::Module) {
            auto left_type = std::get<shared_ptr<RecordModule>>(_left_type);
            if (left_type->isGenericFunc(name) ? left_type->isFunction(name, params_types, true) : left_type->isFunction(name, params_types)) {
                auto func = left_type->getFunction(name, params_types);
                unsigned short idx = 0;
                for (auto [arg_alloca, param_type, argument] : llvm::zip(arg_allocas, params_types, func->arguments)) {
                    if (param_type->stand_alone_type && std::get<2>(argument)) { args[idx] = arg_alloca; }
                    idx++;
                }
                auto returnValue = this->llvm_ir_builder.CreateCall(func->function, args, name + "_result");
                return {returnValue, nullptr, func->return_type, resolveType::StructInst, false};
            } else if (left_type->isGenericFunc(name)) {
                auto gfuncs = left_type->get_GenericFunc(name);
                return this->_CallGfunc(gfuncs, call_expression, name, args, params_types);
            } else if (left_type->isGenericStruct(name)) {
                auto gstruct = left_type->getGenericStruct(name);
                return this->_CallGstruct(gstruct, call_expression, name, args, params_types);
            } else if (left_type->is_struct(name)) {
                return this->_callStruct(left_type->get_struct(name), call_expression, params_types, args);
            } else {
                errors::DosentContain(this->source,
                                      call_expression->name->castToIdentifierLiteral(),
                                      left,
                                      "Struct Or Function " + name + " overload Dose Not Exit.",
                                      "Check the Name is Correct or the params are correct")
                    .raise();
                exit(1);
            }
        } else if (ltt == resolveType::StructInst) {
            auto left_type = std::get<StructTypePtr>(_left_type);
            params_types.insert(params_types.begin(), left_type);
            args.insert(args.begin(), left_alloca);
            arg_allocas.insert(arg_allocas.begin(), left_alloca);
            auto name = right->castToCallExpression()->name->castToIdentifierLiteral()->value;
            if (left_type->is_method(name, params_types)) {
                auto method = left_type->get_method(name, params_types);
                unsigned short idx = 0;
                for (auto [arg_alloca, param_type, argument] : llvm::zip(arg_allocas, params_types, method->arguments)) {
                    if (param_type->stand_alone_type && std::get<2>(argument)) { args[idx] = arg_alloca; }
                    idx++;
                }
                auto returnValue = this->llvm_ir_builder.CreateCall(method->function, args, name + "_reuturn_value");
                return {returnValue, nullptr, method->return_type, resolveType::StructInst, false};
            } else {
                errors::NoOverload(this->source, {}, call_expression, "method does not exist for struct " + left_type->name + ".", "Check the initialization method name or define the method.")
                    .raise();
                exit(1);
            }
        }
    }
    errors::CompletionError("Wrong Member Access",
                            this->source,
                            right->meta_data.st_line_no,
                            right->meta_data.end_line_no,
                            "Member access should be identifier of method not " + AST::nodeTypeToString(right->type()))
        .raise();
    exit(1);
};

Compiler::ResolvedValue Compiler::_StructInfixCall(const Str& op_method,
                                                   const Str& op,
                                                   StructTypePtr left_type,
                                                   StructTypePtr right_type,
                                                   shared_ptr<AST::Expression> left,
                                                   shared_ptr<AST::Expression> right,
                                                   llvm::Value* left_value,
                                                   llvm::Value* right_value) {
    vector<StructTypePtr> params_type1{left_type, right_type};
    vector<StructTypePtr> params_type2{right_type, left_type};
    if (left_type->is_method(op_method, params_type1)) {
        auto func_record = left_type->get_method(op_method, params_type1);
        auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {left_value, right_value});
        return {returnValue, nullptr, func_record->return_type, resolveType::StructInst, false};
    } else if (right_type->is_method(op_method, params_type2)) {
        auto func_record = right_type->get_method(op_method, params_type2);
        auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, {right_value, left_value});
        return {returnValue, nullptr, func_record->return_type, resolveType::StructInst, false};
    } else {
        errors::WrongInfix(this->source, left, right, op, "Cant " + op + " 2 structs", "Add the `" + op_method + "` method in structs in either one of the struct").raise();
        exit(1);
    }
};


Compiler::ResolvedValue Compiler::_visitInfixExpression(shared_ptr<AST::InfixExpression> infixed_expression) {
    auto op = infixed_expression->op;
    auto left = infixed_expression->left;
    auto right = infixed_expression->right;
    auto [left_value, left_alloca, _left_type, ltt, leftinscope] = this->_resolveValue(left);
    if (op == token::TokenType::Dot) { return this->_memberAccess(infixed_expression); }
    auto [right_value, right_alloca, _right_type, rtt, rightinscope] = this->_resolveValue(right);
    if (ltt != resolveType::StructInst || rtt != resolveType::StructInst) {
        errors::WrongInfix(this->source, left, right, token::tokenTypeString(op), "Cant " + token::tokenTypeString(op) + " 2 types or modules").raise();
        exit(1);
    }
    // Handle type conversion
    auto left_val = left_value;
    auto right_val = right_value;
    auto left_type = std::get<StructTypePtr>(_left_type);
    auto right_type = std::get<StructTypePtr>(_right_type);
    if (!_checkType(left_type, right_type) && (this->canConvertType(left_type, right_type) || this->canConvertType(right_type, left_type)) && left_type->stand_alone_type &&
        right_type->stand_alone_type) {
        if (this->conversionPrecidence(left_type, right_type)) {
            auto x = this->convertType({left_val, left_alloca, left_type}, right_type);
            right_val = x.value;
            right_alloca = x.alloca;
            right_type = std::get<StructTypePtr>(x.variant);
        } else {
            auto x = this->convertType({right_val, right_alloca, right_type}, left_type);
            left_val = x.value;
            left_alloca = x.alloca;
            left_type = std::get<StructTypePtr>(x.variant);
        }
    }
    vector<StructTypePtr> params_type1{left_type, right_type};
    vector<StructTypePtr> params_type2{right_type, left_type};
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
                return {inst, nullptr, this->env->getStruct("int"), resolveType::StructInst};
            }
            case (token::TokenType::Dash): {
                auto inst = this->llvm_ir_builder.CreateSub(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("int"), resolveType::StructInst};
            }
            case (token::TokenType::Asterisk): {
                auto inst = this->llvm_ir_builder.CreateMul(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("int"), resolveType::StructInst};
            }
            case (token::TokenType::ForwardSlash): {
                llvm::Value* inst;
                if (left_type->name == "int" || left_type->name == "int32") inst = this->llvm_ir_builder.CreateSDiv(left_val_converted, right_val_converted);
                else inst = this->llvm_ir_builder.CreateUDiv(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("int"), resolveType::StructInst};
            }
            case (token::TokenType::Percent): {
                llvm::Value* inst;
                if (left_type->name == "int" || left_type->name == "int32") inst = this->llvm_ir_builder.CreateSRem(left_val_converted, right_val_converted);
                else inst = this->llvm_ir_builder.CreateURem(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("int"), resolveType::StructInst};
            }
            case (token::TokenType::EqualEqual): {
                auto inst = this->llvm_ir_builder.CreateICmpEQ(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
            }
            case (token::TokenType::NotEquals): {
                auto inst = this->llvm_ir_builder.CreateICmpNE(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
            }
            case (token::TokenType::LessThan): {
                llvm::Value* inst;
                if (left_type->name == "int" || left_type->name == "int32") inst = this->llvm_ir_builder.CreateICmpSLT(left_val_converted, right_val_converted);
                else inst = this->llvm_ir_builder.CreateICmpULT(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
            }
            case (token::TokenType::GreaterThan): {
                llvm::Value* inst;
                if (left_type->name == "int" || left_type->name == "int32") inst = this->llvm_ir_builder.CreateICmpSGT(left_val_converted, right_val_converted);
                else inst = this->llvm_ir_builder.CreateICmpUGT(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
            }
            case (token::TokenType::LessThanOrEqual): {
                llvm::Value* inst;
                if (left_type->name == "int" || left_type->name == "int32") inst = this->llvm_ir_builder.CreateICmpSLE(left_val_converted, right_val_converted);
                else inst = this->llvm_ir_builder.CreateICmpULE(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
            }
            case (token::TokenType::GreaterThanOrEqual): {
                llvm::Value* inst;
                if (left_type->name == "int" || left_type->name == "int32") inst = this->llvm_ir_builder.CreateICmpSGE(left_val_converted, right_val_converted);
                else inst = this->llvm_ir_builder.CreateICmpUGE(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
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
                return {inst, nullptr, this->env->getStruct("float"), resolveType::StructInst};
            }
            case (token::TokenType::Dash): {
                auto inst = this->llvm_ir_builder.CreateFSub(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("float"), resolveType::StructInst};
            }
            case (token::TokenType::Asterisk): {
                auto inst = this->llvm_ir_builder.CreateFMul(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("float"), resolveType::StructInst};
            }
            case (token::TokenType::ForwardSlash): {
                auto inst = this->llvm_ir_builder.CreateFDiv(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("float"), resolveType::StructInst};
            }
            case (token::TokenType::EqualEqual): {
                auto inst = this->llvm_ir_builder.CreateFCmpOEQ(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
            }
            case (token::TokenType::NotEquals): {
                auto inst = this->llvm_ir_builder.CreateFCmpONE(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
            }
            case (token::TokenType::LessThan): {
                auto inst = this->llvm_ir_builder.CreateFCmpOLT(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
            }
            case (token::TokenType::GreaterThan): {
                auto inst = this->llvm_ir_builder.CreateFCmpOGT(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
            }
            case (token::TokenType::LessThanOrEqual): {
                auto inst = this->llvm_ir_builder.CreateFCmpOLE(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
            }
            case (token::TokenType::GreaterThanOrEqual): {
                auto inst = this->llvm_ir_builder.CreateFCmpOGE(left_val_converted, right_val_converted);
                return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
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
        errors::WrongType(this->source, infixed_expression, {this->env->getStruct("int"), this->env->getStruct("float")}, "Unknown Type", "Check the types of the operands.").raise();
        exit(1);
    }
};

Compiler::ResolvedValue Compiler::_resolveAndValidateLeftOperand(const std::shared_ptr<AST::IndexExpression>& index_expression) {
    auto [left, left_alloca, _left_generic, ltt, inscope] = this->_resolveValue(index_expression->left);

    if (ltt != resolveType::StructInst) {
        errors::Cantindex(this->source, index_expression, false, "Cannot index Module or type", "Ensure the left-hand side is an array or a valid indexable type.").raise();
        exit(1);
    }

    auto left_generic = std::get<StructTypePtr>(_left_generic);
    return {left, left_alloca, left_generic, ltt, inscope};
}

Compiler::ResolvedValue Compiler::_resolveAndValidateIndexOperand(const std::shared_ptr<AST::IndexExpression>& index_expression) {
    auto [index, __, _index_generic, itt, inscope] = this->_resolveValue(index_expression->index);

    if (itt != resolveType::StructInst) {
        errors::Cantindex(this->source, index_expression, true, "Index must be an integer, not a Module or type", "Ensure the index is an integer.").raise();
        exit(1);
    }

    auto index_generic = std::get<StructTypePtr>(_index_generic);
    return {index, __, index_generic, itt, inscope};
}

Compiler::ResolvedValue
Compiler::_handleArrayIndexing(llvm::Value* left, StructTypePtr left_generic, llvm::Value* index, StructTypePtr index_generic, const std::shared_ptr<AST::IndexExpression>& index_expression) {
    // Validate that the index is of type 'int'
    if (!_checkType(index_generic, this->env->getStruct("int"))) {
        errors::Cantindex(this->source, index_expression, true, "Index must be an integer not `" + index_generic->name + "`", "Ensure the index is an integer.").raise();
        exit(1);
    }

    // Load the array pointer
    auto array = this->llvm_ir_builder.CreateLoad(this->ll_pointer, this->llvm_ir_builder.CreateGEP(this->ll_shared_ptr, left, llConstInt::get(llvm::Type::getInt64Ty(this->llvm_context), 0)));

    // Get the element type
    auto element_type = left_generic->generic_sub_types[0];
    auto element_ptr_type = (element_type->struct_type || element_type->name == "array") ? this->ll_shared_ptr : element_type->stand_alone_type;

    // Calculate the element pointer
    auto element = this->llvm_ir_builder.CreateGEP(element_ptr_type, array, index, "element");
    if (element_type->struct_type || element_type->name == "array") {
        this->_incrementRC(element);
    }
    // Load the element value
    llvm::Value* load = (element_type->struct_type || element_type->name == "array") ? this->llvm_ir_builder.CreateLoad(this->ll_shared_ptr, element)
                                                                                     : this->llvm_ir_builder.CreateLoad(element_type->stand_alone_type, element);

    return {load, element, element_type, resolveType::StructInst, false};
}

Compiler::ResolvedValue
Compiler::_handleStructIndexing(llvm::Value* left_alloca, llvm::Value* index, StructTypePtr index_generic, StructTypePtr left_generic, const std::shared_ptr<AST::IndexExpression>& index_expression) {
    if (left_generic->is_method("__index__", {left_generic, index_generic})) {
        auto idx_method = left_generic->get_method("__index__", {left_generic, index_generic});
        auto returnValue = this->llvm_ir_builder.CreateCall(idx_method->function, {left_alloca, index});
        return {returnValue, nullptr, idx_method->return_type, resolveType::StructInst, false};
    }

    _raiseNoIndexMethodError(left_generic, index_expression);
}

[[noreturn]] void Compiler::_raiseNoIndexMethodError(StructTypePtr left_generic, const std::shared_ptr<AST::IndexExpression>& index_expression) {
    errors::NoOverload(this->source, {}, index_expression, "__index__ method does not exist for struct " + left_generic->name + ".", "Define the __index__ method.").raise();
    exit(1);
}

Compiler::ResolvedValue Compiler::_visitIndexExpression(const std::shared_ptr<AST::IndexExpression> index_expression) {
    // Resolve and validate the left operand
    auto [left, left_alloca, _left_generic, ltt, _] = this->_resolveAndValidateLeftOperand(index_expression);
    auto left_generic = std::get<StructTypePtr>(_left_generic);
    // Resolve and validate the index operand
    auto [index, __, _index_generic, itt, _] = this->_resolveAndValidateIndexOperand(index_expression);
    auto index_generic = std::get<StructTypePtr>(_index_generic);

    // Handle array indexing if applicable
    if (_checkType(left_generic, this->env->getStruct("array"))) { return _handleArrayIndexing(left, left_generic, index, index_generic, index_expression); }

    // Handle struct indexing via __index__ method
    return _handleStructIndexing(left_alloca, index, index_generic, left_generic, index_expression);
}


// Refactored _visitVariableDeclarationStatement
void Compiler::_visitVariableDeclarationStatement(const std::shared_ptr<AST::VariableDeclarationStatement>& variable_declaration_statement) {
    auto var_name = variable_declaration_statement->name->castToIdentifierLiteral();

    // Check if the variable is already declared
    if (this->env->isVariable(var_name->value)) { errors::DuplicateVariableError(this->source, var_name->value, "Variable is already declared").raise(); }

    auto var_value = variable_declaration_statement->value;
    auto var_type = this->_parseType(variable_declaration_statement->value_type);

    if (var_value == nullptr) {
        llvm::Value* alloca = var_type->struct_type ? this->llvm_ir_builder.CreateAlloca(this->ll_shared_ptr) : this->llvm_ir_builder.CreateAlloca(var_type->stand_alone_type);

        if (var_type->struct_type) {
            auto RC_gep = this->llvm_ir_builder.CreateStructGEP(this->ll_shared_ptr, alloca, 1);
            this->llvm_ir_builder.CreateStore(llvm::ConstantPointerNull::get(this->ll_pointer), RC_gep);
        }

        auto var = std::make_shared<RecordVariable>(var_name->value, nullptr, alloca, var_type);
        this->env->addRecord(var);
        return;
    }

    auto [var_value_resolved, var_value_alloca, _var_generic, vartt, valinscope] = this->_resolveValue(var_value);

    if (vartt != resolveType::StructInst) { errors::WrongType(this->source, var_value, {var_type}, "Cannot assign module or type to variable").raise(); }

    auto var_generic = std::get<StructTypePtr>(_var_generic);

    if (!_checkType(var_generic, var_type)) {
        if (this->canConvertType(var_generic, var_type)) {
            auto converted = this->convertType({var_value_resolved, var_value_alloca, var_generic}, var_type);
            var_value_resolved = converted.value;
            var_value_alloca = converted.alloca;
            var_generic = std::get<StructTypePtr>(converted.variant);
        } else {
            errors::WrongType(this->source, var_value, {var_type}, "Cannot assign mismatched type").raise();
        }
    }

    if (var_type->struct_type || var_type->name == "array") {
        if (valinscope)
            this->_incrementRC(var_value_alloca);
        auto var = std::make_shared<RecordVariable>(var_name->value, var_value_resolved, var_value_resolved, var_generic);
        var->variable_type = var_generic;
        this->env->addRecord(var);
    } else {
        auto alloca = this->llvm_ir_builder.CreateAlloca(var_type->stand_alone_type);
        this->llvm_ir_builder.CreateStore(var_value_resolved, alloca, variable_declaration_statement->is_volatile);
        auto var = std::make_shared<RecordVariable>(var_name->value, var_value_resolved, alloca, var_generic);
        this->env->addRecord(var);
    }
}

// Refactored _visitVariableAssignmentStatement
void Compiler::_visitVariableAssignmentStatement(const std::shared_ptr<AST::VariableAssignmentStatement>& variable_assignment_statement) {
    auto var_value = variable_assignment_statement->value;
    auto [value, value_alloca, _assignmentType, vtt, assignmentinscope] = this->_resolveValue(var_value);
    auto assignmentType = std::get<StructTypePtr>(_assignmentType);

    if (vtt != resolveType::StructInst) { errors::WrongType(this->source, var_value, {assignmentType}, "Cannot assign module or type to variable").raise(); }

    auto [_, alloca, _var_type, att, inscope] = this->_resolveValue(variable_assignment_statement->name);

    if (att != resolveType::StructInst) { errors::WrongType(this->source, variable_assignment_statement->name, {std::get<StructTypePtr>(_var_type)}, "Cannot assign to ltype").raise(); }

    auto var_type = std::get<StructTypePtr>(_var_type);

    if (!_checkType(var_type, assignmentType)) {
        if (this->canConvertType(assignmentType, var_type)) {
            auto converted = this->convertType({value, value_alloca, assignmentType}, var_type);
            value = converted.value;
            value_alloca = converted.alloca;
            assignmentType = std::get<StructTypePtr>(converted.variant);
        } else {
            errors::WrongType(this->source, var_value, {var_type}, "Cannot assign mismatched type").raise();
        }
    }

    if (assignmentType->struct_type) {
        if (assignmentinscope)
            this->_incrementRC(value_alloca);
        this->_decrementRC(assignmentType, alloca);
        auto load = this->llvm_ir_builder.CreateLoad(var_type->struct_type, value);
        this->llvm_ir_builder.CreateStore(load, alloca);
    } else {
        this->llvm_ir_builder.CreateStore(value, alloca);
    }
}

compiler::Compiler::ResolvedValue Compiler::_resolveIntegerLiteral(const std::shared_ptr<AST::IntegerLiteral>& integer_literal) {
    auto value = llvm::ConstantInt::get(this->llvm_context, llvm::APInt(64, integer_literal->value));
    auto alloca = this->llvm_ir_builder.CreateAlloca(this->env->getStruct("int")->stand_alone_type);
    this->llvm_ir_builder.CreateStore(value, alloca);
    return {value, alloca, this->env->getStruct("int"), resolveType::StructInst};
}

compiler::Compiler::ResolvedValue Compiler::_resolveFloatLiteral(const std::shared_ptr<AST::FloatLiteral>& float_literal) {
    auto value = llvm::ConstantFP::get(this->llvm_context, llvm::APFloat(float_literal->value));
    auto alloca = this->llvm_ir_builder.CreateAlloca(this->env->getStruct("float")->stand_alone_type);
    this->llvm_ir_builder.CreateStore(value, alloca);
    return {value, alloca, this->env->getStruct("float"), resolveType::StructInst};
}

compiler::Compiler::ResolvedValue Compiler::_resolveStringLiteral(const std::shared_ptr<AST::StringLiteral>& string_literal) {
    auto value = this->llvm_ir_builder.CreateGlobalStringPtr(string_literal->value);
    return {value, value, this->env->getStruct("str"), resolveType::StructInst};
}

compiler::Compiler::ResolvedValue Compiler::_resolveIdentifierLiteral(const std::shared_ptr<AST::IdentifierLiteral>& identifier_literal) {
    if (this->env->isVariable(identifier_literal->value)) {
        auto variable = this->env->getVariable(identifier_literal->value);
        auto currentStructType = variable->variable_type;
        currentStructType->meta_data = identifier_literal->meta_data;
        if (currentStructType->stand_alone_type) {
            auto loadInst = this->llvm_ir_builder.CreateLoad(currentStructType->stand_alone_type, variable->allocainst);
            return {loadInst, variable->allocainst, currentStructType, resolveType::StructInst, true};
        } else {
            return {variable->allocainst, variable->allocainst, currentStructType, resolveType::StructInst, true};
        }
    } else if (this->env->isModule(identifier_literal->value)) {
        return {nullptr, nullptr, this->env->getModule(identifier_literal->value), resolveType::Module};
    } else if (this->env->isStruct(identifier_literal->value)) {
        return {nullptr, nullptr, this->env->getStruct(identifier_literal->value), resolveType::StructType};
    } else if (this->env->isGenericStruct(identifier_literal->value)) {
        return {nullptr, nullptr, this->env->getGenericStruct(identifier_literal->value), resolveType::GStructType};
    }
    errors::NotDefined(this->source, identifier_literal, "Variable or function or struct `" + identifier_literal->value + "` not defined", "Recheck the Name").raise();
    std::exit(1);
}

compiler::Compiler::ResolvedValue Compiler::_resolveInfixExpression(const std::shared_ptr<AST::InfixExpression>& infix_expression) {
    return this->_visitInfixExpression(infix_expression);
}

compiler::Compiler::ResolvedValue Compiler::_resolveIndexExpression(const std::shared_ptr<AST::IndexExpression>& index_expression) {
    return this->_visitIndexExpression(index_expression);
}

compiler::Compiler::ResolvedValue Compiler::_resolveCallExpression(const std::shared_ptr<AST::CallExpression>& call_expression) {
    return this->_visitCallExpression(call_expression);
}

compiler::Compiler::ResolvedValue Compiler::_resolveBooleanLiteral(const std::shared_ptr<AST::BooleanLiteral>& boolean_literal) {
    auto value = boolean_literal->value ? llvm::ConstantInt::getTrue(this->llvm_context) : llvm::ConstantInt::getFalse(this->llvm_context);
    auto alloca = this->llvm_ir_builder.CreateAlloca(this->env->getStruct("bool")->stand_alone_type);
    this->llvm_ir_builder.CreateStore(value, alloca);
    return {value, nullptr, this->env->getStruct("bool"), resolveType::StructInst, false};
}

compiler::Compiler::ResolvedValue Compiler::_resolveArrayLiteral(const std::shared_ptr<AST::ArrayLiteral>& array_literal) {
    return this->_visitArrayLiteral(array_literal);
}

compiler::Compiler::ResolvedValue Compiler::_resolveValue(shared_ptr<AST::Node> node) {
    switch (node->type()) {
        case AST::NodeType::IntegerLiteral:
            return this->_resolveIntegerLiteral(node->castToIntegerLiteral());
        case AST::NodeType::FloatLiteral:
            return this->_resolveFloatLiteral(node->castToFloatLiteral());
        case AST::NodeType::StringLiteral:
            return this->_resolveStringLiteral(node->castToStringLiteral());
        case AST::NodeType::IdentifierLiteral:
            return this->_resolveIdentifierLiteral(node->castToIdentifierLiteral());
        case AST::NodeType::InfixedExpression:
            return this->_resolveInfixExpression(node->castToInfixExpression());
        case AST::NodeType::IndexExpression:
            return this->_resolveIndexExpression(node->castToIndexExpression());
        case AST::NodeType::CallExpression:
            return this->_resolveCallExpression(node->castToCallExpression());
        case AST::NodeType::BooleanLiteral:
            return this->_resolveBooleanLiteral(node->castToBooleanLiteral());
        case AST::NodeType::ArrayLiteral:
            return this->_resolveArrayLiteral(node->castToArrayLiteral());
        default:
            errors::UnknownNodeTypeError("Unknown node type", this->source, node->meta_data.st_line_no, node->meta_data.st_col_no, node->meta_data.end_line_no, node->meta_data.end_col_no, "Unknown node type: " + AST::nodeTypeToString(node->type()))
                .raise();
            std::exit(1);
    }
}

Compiler::ResolvedValue Compiler::_visitArrayLiteral(const std::shared_ptr<AST::ArrayLiteral>& array_literal) {
    std::vector<llvm::Value*> values;
    StructTypePtr struct_type = nullptr;
    StructTypePtr first_generic = nullptr;

    for (const auto& element : array_literal->elements) {
        // Validate the array element's type
        this->_validateArrayElement(element, first_generic);

        // Resolve the value of the element
        auto [value, value_alloca, _generic, vtt, element_inscope] = this->_resolveValue(element);
        StructTypePtr generic = std::get<StructTypePtr>(_generic);

        // Initialize struct_type and first_generic if not set
        if (!struct_type) {
            struct_type = generic;
            first_generic = generic;
        }
        // Handle type conversion if necessary
        ResolvedValue resolved_value = {value, value_alloca, _generic, vtt};
        this->_handleTypeConversion(element, resolved_value, first_generic);
        generic = std::get<StructTypePtr>(resolved_value.variant);
        if (element_inscope && (generic->struct_type || generic->name == "array")) {
            this->_incrementRC(value);
        }

        // Collect the validated and possibly converted value
        values.push_back(resolved_value.value);
    }

    // Create the LLVM array type
    auto array_type = llvm::ArrayType::get(struct_type->struct_type ? this->ll_shared_ptr : struct_type->stand_alone_type, values.size());
    llvm::Value* array;

    // Allocate memory for the array
    if (array_literal->_new) {
        llvm::Value* gep = this->llvm_ir_builder.CreateGEP(array_type, llvm::ConstantPointerNull::get(this->ll_pointer), llConstInt::get(this->ll_int, 1));
        llvm::Value* size = this->llvm_ir_builder.CreatePtrToInt(gep, this->ll_int);
        array = this->llvm_ir_builder.CreateCall(this->env->getFunction("malloc", {this->env->getStruct("int")})->function, {size});
    } else {
        array = this->llvm_ir_builder.CreateAlloca(array_type);
    }

    // Store each element in the LLVM array
    for (size_t i = 0; i < values.size(); ++i) {
        auto element_ptr = this->llvm_ir_builder.CreateGEP(array_type, array, {this->llvm_ir_builder.getInt64(0), this->llvm_ir_builder.getInt64(i)});
        // if (first_generic->struct_type || first_generic->name == "array") { this->_incrementRC(values[i]); }
        this->llvm_ir_builder.CreateStore(values[i], element_ptr);
    }

    // Create the array struct and manage reference counting
    auto array_struct = std::make_shared<RecordStructType>(*this->env->getStruct("array"));
    array_struct->generic_sub_types.push_back(first_generic);
    auto shared_array = this->llvm_ir_builder.CreateAlloca(this->ll_shared_ptr);
    this->llvm_ir_builder.CreateStore(array, this->llvm_ir_builder.CreateStructGEP(this->ll_shared_ptr, shared_array, 0));

    if (array_literal->_new) {
        auto RC_alloca = this->llvm_ir_builder.CreateCall(this->env->getFunction("malloc", {this->env->getStruct("int")})->function,
                                                          {llConstInt::get(llvm::Type::getInt64Ty(this->llvm_context), this->ll_int->getScalarSizeInBits() / 8)});
        this->llvm_ir_builder.CreateStore(llConstInt::get(this->ll_int, 1), RC_alloca);
        this->llvm_ir_builder.CreateStore(RC_alloca, this->llvm_ir_builder.CreateStructGEP(this->ll_shared_ptr, shared_array, 1));
    } else {
        this->llvm_ir_builder.CreateStore(llvm::ConstantPointerNull::get(this->ll_pointer), this->llvm_ir_builder.CreateStructGEP(this->ll_shared_ptr, shared_array, 1));
    }

    return {shared_array, shared_array, array_struct, resolveType::StructInst, false};
}

// Implementation of _validateArrayElement
void Compiler::_validateArrayElement(const std::shared_ptr<AST::Node>& element, const StructTypePtr& first_generic) {
    auto [value, value_alloca, _generic, vtt, _] = this->_resolveValue(element);
    if (vtt != resolveType::StructInst) { errors::ArrayTypeError(this->source, element, first_generic, "Cannot add Module or type in Array").raise(); }
}

// Implementation of _handleTypeConversion
void Compiler::_handleTypeConversion(AST::ExpressionPtr element, ResolvedValue& resolved_value, const StructTypePtr& first_generic) {
    StructTypePtr generic = std::get<StructTypePtr>(resolved_value.variant);

    if (!_checkType(first_generic, generic)) {
        if (this->canConvertType(generic, first_generic)) {
            ResolvedValue converted = this->convertType({resolved_value.value, resolved_value.alloca, generic}, first_generic);
            resolved_value.value = converted.value;
            resolved_value.alloca = converted.alloca;
            resolved_value.variant = converted.variant;
        } else {
            errors::ArrayTypeError(this->source, element, first_generic, "Array with multiple types or generics").raise();
        }
    }
}

// Definition of _handleVoidReturnStatement
void Compiler::_handleVoidReturnStatement(const std::shared_ptr<AST::ReturnStatement>& return_statement) {
    this->llvm_ir_builder.CreateRetVoid();
    _cleanupReferenceCounts();
    throw DoneRet(); // Indicates to stop parsing further statements in the current block
}

// Definition of _handleValueReturnStatement
void Compiler::_handleValueReturnStatement(const std::shared_ptr<AST::ReturnStatement>& return_statement) {
    auto value = return_statement->value;
    auto [return_value, return_alloca, _return_type, _, return_in_scope] = _resolveAndValidateReturnValue(value);
    auto return_type = std::get<StructTypePtr>(_return_type);
    if (this->env->current_function == nullptr) {
        errors::NodeOutside("Return outside function", this->source, *return_statement, errors::outsideNodeType::Retuen, "Return statement outside of a function").raise();
        exit(1);
    }

    _checkAndConvertReturnType(value, return_type);

    if (return_type->struct_type && return_in_scope) { this->_incrementRC(return_value); }

    _cleanupReferenceCounts();

    _createReturnInstruction(return_value, return_alloca, return_type);

    throw DoneRet(); // Indicates to stop parsing further statements in the current block
}

// Definition of _resolveAndValidateReturnValue
Compiler::ResolvedValue Compiler::_resolveAndValidateReturnValue(const std::shared_ptr<AST::Expression>& value) {
    auto resolved_value = this->_resolveValue(value);

    if (resolved_value.type != resolveType::StructInst) {
        if (resolved_value.type == resolveType::StructType && std::get<StructTypePtr>(resolved_value.variant)->name == "void") {
            this->llvm_ir_builder.CreateRetVoid();
            throw DoneRet();
        } else {
            errors::WrongType(this->source, value, {this->env->current_function->return_type}, "Cannot return module or type from function").raise();
            exit(1);
        }
    }

    StructTypePtr return_type = std::get<StructTypePtr>(resolved_value.variant);
    return {resolved_value.value, resolved_value.alloca, return_type, resolved_value.type, resolved_value.inscope};
}

// Definition of _checkAndConvertReturnType
void Compiler::_checkAndConvertReturnType(AST::ExpressionPtr value, StructTypePtr& return_type) {
    llvm::Value* return_value = nullptr;
    llvm::Value* return_alloca = nullptr;

    if (!_checkType(this->env->current_function->return_type, return_type)) {
        if (this->canConvertType(return_type, this->env->current_function->return_type)) {
            auto [converted_value, converted_alloca, converted_type, _, _] = this->convertType({return_value, return_alloca, return_type}, this->env->current_function->return_type);
            return_value = converted_value;
            return_alloca = converted_alloca;
            if (!return_value) {
                errors::ReturnTypeMismatchError(this->source, this->env->current_function->return_type, value, "Return Type mismatch").raise();
                exit(1);
            }
            return_type = std::get<StructTypePtr>(converted_type);
        } else {
            errors::ReturnTypeMismatchError(this->source, this->env->current_function->return_type, value, "Return Type mismatch").raise();
            exit(1);
        }
    }
}

// Definition of _cleanupReferenceCounts
void Compiler::_cleanupReferenceCounts() {
    for (auto var : this->env->current_function->env->getCurrentVars()) {
        // if (var->variable_type->struct_type || var->variable_type->name == "array") { this->_decrementRC(var->variable_type, var->value); }
    }
}

// Definition of _createReturnInstruction
void Compiler::_createReturnInstruction(llvm::Value* return_value, llvm::Value* return_alloca, StructTypePtr return_type) {
    auto function_return_type = this->env->current_function->function->getReturnType();

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
}

// Refactored _visitReturnStatement
void Compiler::_visitReturnStatement(shared_ptr<AST::ReturnStatement> return_statement) {
    if (!return_statement->value && this->env->current_function->return_type->name == "void") {
        _handleVoidReturnStatement(return_statement);
    } else {
        _handleValueReturnStatement(return_statement);
    }
}

StructTypePtr Compiler::_parseType(shared_ptr<AST::Type> type) {
    vector<StructTypePtr> generics;
    for (auto gen : type->generics) { generics.push_back(this->_parseType(gen)); }

    auto [_, __, _struct, stt, _] = this->_resolveValue(type->name);
    if (stt != resolveType::StructType) {
        if (stt == resolveType::GStructType) {
            auto gstructs = std::get<vector<GenericStructTypePtr>>(_struct);
            auto prev_env = this->env;

            for (auto gstruct : gstructs) {
                if (generics.size() != gstruct->structAST->generics.size()) { continue; }

                this->env = std::make_shared<Enviornment>(gstruct->env);
                Str struct_name = gstruct->structAST->name->castToIdentifierLiteral()->value;
                auto struct_record = std::make_shared<RecordStructType>(struct_name);

                if (!gstruct->env->isStruct(struct_name, false, generics)) {
                    for (auto [generic, rg] : llvm::zip(generics, gstruct->structAST->generics)) {
                        auto generic_copy = std::make_shared<RecordStructType>(*generic);
                        generic_copy->name = rg->name->castToIdentifierLiteral()->value;
                        this->env->addRecord(generic_copy);
                    }

                    vector<llvm::Type*> field_types;
                    auto fields = gstruct->structAST->fields;
                    this->env->addRecord(struct_record);

                    for (auto field : fields) {
                        if (field->type() == AST::NodeType::VariableDeclarationStatement) {
                            _handleFieldDeclaration(gstruct, field, struct_record, field_types, struct_name);
                        } else if (field->type() == AST::NodeType::FunctionStatement) {
                            auto func_dec = field->castToFunctionStatement();
                            if (func_dec->generic.size() != 0) { continue; }
                            this->_visitFunctionDeclarationStatement(func_dec, struct_record);
                        }
                    }
                } else {
                    struct_record = gstruct->env->getStruct(struct_name, false, generics);
                }

                gstruct->env->addRecord(struct_record);
                this->env = prev_env;
                return struct_record;
            }

            errors::NoOverload(this->source, {}, type->name, "No GStruct Viable").raise();
        }

        errors::WrongType(this->source, type->name, {}, "module is not a Type").raise();
    }

    auto struct_ = std::get<StructTypePtr>(_struct);
    if (struct_->name == "array") {
        struct_ = std::make_shared<RecordStructType>(*struct_);
        struct_->generic_sub_types.push_back(generics[0]);
    }
    return struct_;
}

void Compiler::_handleFieldDeclaration(GenericStructTypePtr gstruct, AST::NodePtr field, std::shared_ptr<RecordStructType> struct_record, vector<llvm::Type*>& field_types, const Str& struct_name) {
    auto field_decl = field->castToVariableDeclarationStatement();
    Str field_name = field_decl->name->castToIdentifierLiteral()->value;
    auto field_type = this->_parseType(field_decl->value_type);
    struct_record->addSubType(field_name, field_type);

    field_types.push_back(field_type->stand_alone_type ? field_type->stand_alone_type : field_type->struct_type);

    if (field_decl->value_type->type() == AST::NodeType::IdentifierLiteral) {
        auto field_value = field_decl->value_type->name->castToIdentifierLiteral()->value;
        bool is_generic = false;

        for (const auto& generic : gstruct->structAST->generics) {
            if (field_value == generic->name->castToIdentifierLiteral()->value) {
                is_generic = true;
                break;
            }
        }

        if (is_generic) { struct_record->generic_sub_types.push_back(field_type); }
    }

    auto struct_type = llvm::StructType::create(this->llvm_context, field_types, this->fc_st_name_prefix + struct_name);
    struct_type->setBody(field_types);
    struct_record->struct_type = struct_type;
}

void Compiler::_visitFunctionDeclarationStatement(ASTFunctionStatementPtr function_declaration_statement, StructTypePtr struct_) {
    this->_createFunctionRecord(function_declaration_statement, struct_, nullptr, this->ir_gc_map_json);
}

Compiler::ResolvedValue Compiler::_visitCallExpression(shared_ptr<AST::CallExpression> call_expression) {
    auto name = call_expression->name->castToIdentifierLiteral()->value;
    auto param = call_expression->arguments;
    vector<llvm::Value*> args;
    vector<llvm::Value*> arg_allocas;
    vector<StructTypePtr> params_types;

    for (auto arg : param) {
        auto [value, alloca, _param_type, ptt, _] = this->_resolveValue(arg);
        auto param_type = std::get<StructTypePtr>(_param_type);
        if (ptt == resolveType::Module) {
            errors::WrongType(this->source, arg, {}, "Cant pass Module to the Function").raise();
            exit(1);
        } else if (ptt == resolveType::StructType || ptt == resolveType::GStructType) {
            errors::WrongType(this->source, arg, {}, "Cant pass type to the Function").raise();
            exit(1);
        }
        params_types.push_back(param_type);
        args.push_back(param_type->struct_type || param_type->name == "array" ? this->llvm_ir_builder.CreateLoad(this->ll_shared_ptr, alloca) : value);
        arg_allocas.push_back(alloca);
        if ((param_type->struct_type || param_type->name == "array") && _)
            this->_incrementRC(value);
    }

    if (this->env->isGenericFunc(name) ? this->env->isFunction(name, params_types, false, true) : this->env->isFunction(name, params_types)) {
        auto func = this->env->getFunction(name, params_types, false, true);
        unsigned short idx = 0;
        for (auto [arg_alloca, param_type, argument] : llvm::zip(arg_allocas, params_types, func->arguments)) {
            if (param_type->stand_alone_type && std::get<2>(argument)) { args[idx] = arg_alloca; }
            idx++;
        }
        this->_checkAndConvertCallType(func, call_expression, args, params_types);
        auto returnValue = this->llvm_ir_builder.CreateCall(func->function, args);
        return {returnValue, nullptr, func->return_type, resolveType::StructInst, false};
    } else if (this->env->isGenericFunc(name)) {
        auto gfuncs = this->env->getGenericFunc(name);
        return this->_CallGfunc(gfuncs, call_expression, name, args, params_types);
    } else if (this->env->isGenericStruct(name)) {
        auto gstruct = this->env->getGenericStruct(name);
        return this->_CallGstruct(gstruct, call_expression, name, args, params_types);
    } else if (this->env->isStruct(name)) {
        auto struct_record = this->env->getStruct(name);
        return _callStruct(struct_record, call_expression, params_types, args);
    }

    errors::CompletionError("Function not defined", this->source, call_expression->meta_data.st_line_no, call_expression->meta_data.end_line_no, "Function `" + name + "` not defined").raise();
    exit(1);
};

void Compiler::_visitIfElseStatement(shared_ptr<AST::IfElseStatement> if_statement) {
    // Resolve the condition expression
    auto [condition_val, _, condition_type_ptr, condition_resolve_type, _] = this->_resolveValue(if_statement->condition);

    // Ensure the condition is of type bool
    if (condition_resolve_type != resolveType::StructInst || !_checkType(std::get<StructTypePtr>(condition_type_ptr), this->env->getStruct("bool"))) {
        errors::WrongType(this->source, if_statement->condition, {this->env->getStruct("bool")}, "If-else condition must be of type bool.").raise();
        exit(1);
    }

    // Save the current environment and create a new one for the if-else block
    auto previous_env = this->env;
    this->env = make_shared<Enviornment>(this->env);

    // Get the current function from the LLVM IR builder
    auto current_function = this->llvm_ir_builder.GetInsertBlock()->getParent();

    // Create basic blocks for 'then', 'else' (if exists), and 'continue'
    llBB* then_block = llBB::Create(this->llvm_context, "then", current_function);
    llBB* continue_block = llBB::Create(this->llvm_context, "cont", current_function);
    llBB* else_block = if_statement->alternative ? llBB::Create(this->llvm_context, "else", current_function) : nullptr;

    // Create a conditional branch based on the condition value
    if (else_block) {
        this->llvm_ir_builder.CreateCondBr(condition_val, then_block, else_block);
    } else {
        this->llvm_ir_builder.CreateCondBr(condition_val, then_block, continue_block);
    }

    // Compile the 'then' branch
    this->llvm_ir_builder.SetInsertPoint(then_block);
    try {
        this->compile(if_statement->consequence);
        this->llvm_ir_builder.CreateBr(continue_block);
    } catch (DoneRet) {
        // Handle return within the 'then' branch
    } catch (DoneBr) {
        // Handle branch within the 'then' branch
    }

    // Compile the 'else' branch if it exists
    if (else_block) {
        // Restore the previous environment before compiling the 'else' branch
        this->env = make_shared<Enviornment>(previous_env);
        this->llvm_ir_builder.SetInsertPoint(else_block);
        try {
            this->compile(if_statement->alternative);
            this->llvm_ir_builder.CreateBr(continue_block);
        } catch (DoneRet) {
            // Handle return within the 'else' branch
        } catch (DoneBr) {
            // Handle branch within the 'else' branch
        }
    }

    // Restore the original environment and set the insert point to the 'continue' block
    this->env = previous_env;
    this->llvm_ir_builder.SetInsertPoint(continue_block);
}

void Compiler::_visitWhileStatement(shared_ptr<AST::WhileStatement> while_statement) {
    // Extract condition and body from the while statement
    auto condition = while_statement->condition;
    auto body = while_statement->body;

    // Get the current function from the LLVM IR builder
    auto current_function = this->llvm_ir_builder.GetInsertBlock()->getParent();

    // Create basic blocks for condition, body, and continuation
    llBB* conditionBlock = llBB::Create(this->llvm_context, "cond", current_function);
    llBB* bodyBlock = llBB::Create(this->llvm_context, "body", current_function);
    llBB* continueBlock = llBB::Create(this->llvm_context, "cont", current_function);

    // Branch to the condition block
    this->llvm_ir_builder.CreateBr(conditionBlock);
    this->llvm_ir_builder.SetInsertPoint(conditionBlock);

    // Resolve the condition value
    auto [condition_val, condition_alloca, condition_type_variant, condition_resolve_type, _] = this->_resolveValue(condition);

    // Ensure the condition is of type StructInst
    if (condition_resolve_type != resolveType::StructInst) {
        errors::WrongType(this->source, condition, {this->env->getStruct("bool")}, "While loop condition cannot be a module or type.").raise();
        exit(1);
    }

    // Retrieve the actual condition type
    auto conditionType = std::get<StructTypePtr>(condition_type_variant);

    // Check if the condition type is bool, possibly converting if necessary
    if (!_checkType(conditionType, this->env->getStruct("bool"))) {
        if (this->canConvertType(conditionType, this->env->getStruct("bool"))) {
            std::cout << "Converting condition to bool." << std::endl;
            auto converted = this->convertType({condition_val, condition_alloca, conditionType}, this->env->getStruct("bool"));
            condition_val = converted.value;
        } else {
            errors::WrongType(this->source, condition, {this->env->getStruct("bool")}, "While loop condition must be of type bool.").raise();
            exit(1);
        }
    }

    // Initialize break and continue blocks if provided
    llBB* ifBreakBlock = nullptr;
    llBB* notBreakBlock = nullptr;

    if (while_statement->ifbreak) { ifBreakBlock = llBB::Create(this->llvm_context, "ifbreak", current_function); }

    if (while_statement->notbreak) {
        notBreakBlock = llBB::Create(this->llvm_context, "notbreak", current_function);
        this->llvm_ir_builder.CreateCondBr(condition_val, bodyBlock, notBreakBlock);
    } else {
        this->llvm_ir_builder.CreateCondBr(condition_val, bodyBlock, continueBlock);
    }

    // Enter the loop environment
    this->env->enterLoop(continueBlock, bodyBlock, conditionBlock, ifBreakBlock, notBreakBlock);

    // Set the insertion point to the body block
    this->llvm_ir_builder.SetInsertPoint(bodyBlock);

    // Save the current environment and create a new one for the loop body
    auto previous_env = this->env;
    this->env = make_shared<Enviornment>(this->env);

    try {
        // Compile the loop body
        this->compile(body);
        // After body execution, branch back to the condition block
        this->llvm_ir_builder.CreateBr(conditionBlock);
    } catch (DoneRet) {
        // Handle return statements within the loop body
    } catch (DoneBr) {
        // Handle break statements within the loop body
    }

    // Handle the ifbreak block if it exists
    if (ifBreakBlock) {
        // Restore the previous environment
        this->env = make_shared<Enviornment>(previous_env);
        this->llvm_ir_builder.SetInsertPoint(ifBreakBlock);

        try {
            // Compile the ifbreak statement
            this->compile(while_statement->ifbreak);
            // After ifbreak, branch to the continuation block
            this->llvm_ir_builder.CreateBr(continueBlock);
        } catch (DoneRet) {
            // Handle return statements within the ifbreak block
        } catch (DoneBr) {
            // Handle break statements within the ifbreak block
        }
    }

    // Handle the notbreak block if it exists
    if (notBreakBlock) {
        // Restore the previous environment
        this->env = make_shared<Enviornment>(previous_env);
        this->llvm_ir_builder.SetInsertPoint(notBreakBlock);

        try {
            // Compile the notbreak statement
            this->compile(while_statement->notbreak);
            // After notbreak, branch to the continuation block
            this->llvm_ir_builder.CreateBr(continueBlock);
        } catch (DoneRet) {
            // Handle return statements within the notbreak block
        } catch (DoneBr) {
            // Handle break statements within the notbreak block
        }
    }

    // Set the insertion point to the continuation block and restore the environment
    this->llvm_ir_builder.SetInsertPoint(continueBlock);
    this->env = previous_env;

    // Exit the loop environment
    this->env->exitLoop();
}

void Compiler::_visitForStatement(shared_ptr<AST::ForStatement> for_statement) {
    // Resolve the iterable object from the 'from' expression
    auto [iterable_value, iterable_alloca, _iterable_type, resolve_type, _] = this->_resolveValue(for_statement->from);

    // Ensure the resolved type is a struct instance
    if (resolve_type != resolveType::StructInst) {
        errors::WrongType(this->source, for_statement->from, {}, "Cannot loop over a module or type").raise();
        exit(1);
    }

    // Get the actual struct type of the iterable
    auto iterable_type = std::get<StructTypePtr>(_iterable_type);

    // Check if the iterable has an __iter__ method
    if (iterable_type->is_method("__iter__", {iterable_type})) {
        // Retrieve the __iter__ method and its return type (iterator type)
        auto iter_method = iterable_type->get_method("__iter__", {iterable_type});
        auto iterator_type = iter_method->return_type;

        // Ensure the iterator has both __next__ and __done__ methods
        bool has_next = iterable_type->is_method("__next__", {iterable_type, iterator_type});
        bool has_done = iterable_type->is_method("__done__", {iterable_type, iterator_type}, {}, this->env->getStruct("bool"));
        if (!(has_next && has_done)) {
            std::cerr << "__next__ or __done__ method not defined" << std::endl;
            exit(1);
        }

        // Get the current LLVM function
        auto current_function = this->llvm_ir_builder.GetInsertBlock()->getParent();

        // Create basic blocks for condition, body, and continuation
        llBB* condition_block = llBB::Create(this->llvm_context, "cond", current_function);
        llBB* body_block = llBB::Create(this->llvm_context, "body", current_function);
        llBB* continue_block = llBB::Create(this->llvm_context, "cont", current_function);

        // Enter the loop environment
        this->env->enterLoop(continue_block, body_block, condition_block, nullptr, nullptr);

        // Retrieve the __next__ and __done__ methods
        auto next_method = iterable_type->get_method("__next__", {iterable_type, iterator_type});
        auto done_method = iterable_type->get_method("__done__", {iterable_type, iterator_type});

        // Allocate space for the iterator
        llvm::Value* iterator_alloca = this->llvm_ir_builder.CreateAlloca(iterator_type->stand_alone_type);
        if (iterator_type->stand_alone_type) {
            // Store the result of __iter__ call into the allocated space
            llvm::Value* iter_result = this->llvm_ir_builder.CreateCall(iter_method->function, {iterable_alloca});
            this->llvm_ir_builder.CreateStore(iter_result, iterator_alloca);
        } else {
            // Directly assign the result of __iter__ call
            iterator_alloca = this->llvm_ir_builder.CreateCall(iter_method->function, {iterable_alloca});
        }

        // Initialize break and continue blocks if provided
        llBB* if_break_block = nullptr;
        llBB* not_break_block = nullptr;

        if (for_statement->ifbreak) { if_break_block = llBB::Create(this->llvm_context, "ifbreak", current_function); }

        // Branch to the condition block
        this->llvm_ir_builder.CreateBr(condition_block);
        this->llvm_ir_builder.SetInsertPoint(condition_block);

        // Handle the 'notbreak' condition if present
        if (for_statement->notbreak) {
            not_break_block = llBB::Create(this->llvm_context, "notbreak", current_function);
            // Call __done__ to determine if the loop should continue
            llvm::Value* done_call = this->llvm_ir_builder.CreateCall(done_method->function, {iterable_alloca, iterator_alloca});
            this->llvm_ir_builder.CreateCondBr(done_call, not_break_block, body_block);
        } else {
            // Call __done__ to determine if the loop should continue
            llvm::Value* done_call = this->llvm_ir_builder.CreateCall(done_method->function, {iterable_alloca, iterator_alloca});
            this->llvm_ir_builder.CreateCondBr(done_call, continue_block, body_block);
        }

        // Set insertion point to the body block
        this->llvm_ir_builder.SetInsertPoint(body_block);

        // Save the current environment and create a new one for the loop body
        auto previous_env = this->env;
        this->env = std::make_shared<Enviornment>(this->env);

        // Allocate space for the loop variable
        llvm::Value* loop_var_alloca;
        if (next_method->return_type->stand_alone_type) {
            loop_var_alloca = this->llvm_ir_builder.CreateAlloca(next_method->return_type->stand_alone_type);
        } else {
            loop_var_alloca = this->llvm_ir_builder.CreateAlloca(next_method->return_type->struct_type);
        }

        // Call __next__ and store the result
        if (next_method->return_type->struct_type) {
            llvm::Value* next_call = this->llvm_ir_builder.CreateCall(next_method->function, {iterable_alloca, iterator_alloca});
            llvm::Value* loaded_value = this->llvm_ir_builder.CreateLoad(next_method->return_type->struct_type, next_call);
            this->llvm_ir_builder.CreateStore(loaded_value, loop_var_alloca);
        } else {
            llvm::Value* next_call = this->llvm_ir_builder.CreateCall(next_method->function, {iterable_alloca, iterator_alloca});
            this->llvm_ir_builder.CreateStore(next_call, loop_var_alloca);
        }

        // Create and add the loop variable to the environment
        VariablePtr loop_var;
        if (next_method->return_type->struct_type) {
            loop_var = std::make_shared<RecordVariable>(for_statement->get->value, loop_var_alloca, loop_var_alloca, next_method->return_type);
        } else {
            llvm::Value* loaded_value = this->llvm_ir_builder.CreateLoad(next_method->return_type->stand_alone_type, loop_var_alloca);
            loop_var = std::make_shared<RecordVariable>(for_statement->get->value, loaded_value, loop_var_alloca, next_method->return_type);
        }
        this->env->addRecord(loop_var);

        // Compile the body of the loop
        try {
            this->compile(for_statement->body);
            // After body execution, branch back to the condition block
            this->llvm_ir_builder.CreateBr(condition_block);
        } catch (DoneRet) {
            // Handle return statements within the loop body
        } catch (DoneBr) {
            // Handle break statements within the loop body
        }

        // Handle the ifbreak block if it exists
        if (if_break_block) {
            // Restore the previous environment
            this->env = std::make_shared<Enviornment>(previous_env);
            this->llvm_ir_builder.SetInsertPoint(if_break_block);
            try {
                // Compile the ifbreak statement
                this->compile(for_statement->ifbreak);
                // After ifbreak, branch to the continuation block
                this->llvm_ir_builder.CreateBr(continue_block);
            } catch (DoneRet) {
                // Handle return statements within the ifbreak block
            } catch (DoneBr) {
                // Handle break statements within the ifbreak block
            }
        }

        // Handle the notbreak block if it exists
        if (not_break_block) {
            // Restore the previous environment
            this->env = std::make_shared<Enviornment>(previous_env);
            this->llvm_ir_builder.SetInsertPoint(not_break_block);
            try {
                // Compile the notbreak statement
                this->compile(for_statement->notbreak);
                // After notbreak, branch to the continuation block
                this->llvm_ir_builder.CreateBr(continue_block);
            } catch (DoneRet) {
                // Handle return statements within the notbreak block
            } catch (DoneBr) {
                // Handle break statements within the notbreak block
            }
        }

        // Restore the original environment and set the insertion point to the continuation block
        this->env = previous_env;
        this->llvm_ir_builder.SetInsertPoint(continue_block);

        // Exit the loop environment
        this->env->exitLoop();
    }
}

void Compiler::_visitTryCatchStatement(shared_ptr<AST::TryCatchStatement> tc_statement) {
    std::cerr << "TODO: Add Suport to Handel Exception" << std::endl;
    exit(1);
}

void Compiler::_visitRaiseStatement(shared_ptr<AST::RaiseStatement> raise_statement) {
    std::cerr << "TODO: Add Suport to raise Exception" << std::endl;
    exit(1);
}

const Str readFileToString(const Str& filePath); // Defined in main.cpp

void Compiler::_visitImportStatement(shared_ptr<AST::ImportStatement> import_statement, shared_ptr<RecordModule> module) {
    // Extract the relative path from the import statement
    Str relative_path = import_statement->relativePath;

    // Derive the module name by taking the substring after the last '/' and replacing '.' with '_'
    auto module_name = relative_path.substr(relative_path.find_last_of('/') + 1);
    std::replace(module_name.begin(), module_name.end(), '.', '_');

    // Determine the path for the imported source file
    std::filesystem::path gc_source_path = this->file_path.parent_path() / (relative_path + ".gc");
    if (relative_path.starts_with("std/")) { gc_source_path = GC_STD_DIR / (relative_path.substr(4) + ".gc"); }

    // Determine the path for the IR-GC map file
    std::filesystem::path ir_gc_map_path = this->ir_gc_map.parent_path() / (relative_path + ".gc.Json");
    if (relative_path.starts_with("std/")) { ir_gc_map_path = GC_STD_IRGCMAP / (relative_path.substr(4) + ".gc.Json"); }

    // Load the IR-GC map JSON
    Json ir_gc_map_json;
    std::ifstream ir_gc_map_file(ir_gc_map_path);
    if (!ir_gc_map_file.is_open()) {
        std::cerr << "Failed to open IR-GC map file: " << ir_gc_map_path << std::endl;
        throw NotCompiledError(gc_source_path.string());
    }
    ir_gc_map_file >> ir_gc_map_json;
    ir_gc_map_file.close();

    // Verify if the IR-GC map is up-to-date
    if (!ir_gc_map_json["uptodate"]) {
        std::cerr << "IR-GC map is not up-to-date for: " << gc_source_path << std::endl;
        throw NotCompiledError(gc_source_path.string());
    }

    // Read the source code from the file
    Str gc_source = readFileToString(gc_source_path.string());
    Str previous_source = this->source;
    this->source = gc_source;

    // Parse the source code into an AST
    Lexer lexer(gc_source);
    parser::Parser parser(std::make_shared<Lexer>(lexer));
    auto program = parser.parseProgram();

    // Save the current environment and create a new one for the imported module
    auto previous_env = this->env;
    this->env = std::make_shared<Enviornment>(previous_env, StrRecordMap{}, module_name);

    // Create a new module record if not importing into an existing module
    std::shared_ptr<RecordModule> import_module = module;
    if (!module) {
        import_module = std::make_shared<RecordModule>(module_name);
        this->env->addRecord(import_module);
    }

    // Iterate over each statement in the imported program
    for (auto& stmt : program->statements) {
        switch (stmt->type()) {
            case AST::NodeType::FunctionStatement:
                this->_createFunctionRecord(stmt->castToFunctionStatement(), nullptr, import_module, ir_gc_map_json);
                break;

            case AST::NodeType::StructStatement:
                this->_createStructRecord(stmt->castToStructStatement(), import_module, ir_gc_map_json);
                break;

            case AST::NodeType::ImportStatement:
                this->_visitImportStatement(stmt->castToImportStatement(), import_module);
                break;

            default:
                std::cerr << "Unknown statement type during import. Skipping." << std::endl;
                break;
        }
    }

    // Restore the previous state
    this->file_path = this->file_path.parent_path(); // Adjust path as necessary
    this->env = previous_env;
    this->source = previous_source;
}

void Compiler::_importFunctionDeclarationStatement(ASTFunctionStatementPtr function_declaration_statement, std::shared_ptr<RecordModule> module, Json& ir_gc_map_json) {
    // Utilize the helper function with the module and ir_gc_map_json
    this->_createFunctionRecord(function_declaration_statement, nullptr, module, ir_gc_map_json);
}

void Compiler::_importStructStatement(ASTStructStatementPtr struct_statement, std::shared_ptr<RecordModule> module, Json& ir_gc_map_json) {
    // Utilize the helper function with the module and ir_gc_map_json
    this->_createStructRecord(struct_statement, module, ir_gc_map_json);
}
