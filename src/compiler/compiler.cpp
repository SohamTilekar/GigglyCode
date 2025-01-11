// TODO: Add Meta Data to all of the Record.
// TODO: Allocate on Heap only if `call_expression->_new`
#include "compiler.hpp"
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>
#include <thread>

#include "../errors/errors.hpp"
#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include "enviornment/enviornment.hpp"

using namespace compiler;
using llConstInt = llvm::ConstantInt;

Compiler::Compiler(
    const Str& source, const std::filesystem::path& file_path, compilationState::RecordFile* file_record, const std::filesystem::path& buildDir, const std::filesystem::path& relativePath)
    : llvm_context(), llvm_ir_builder(llvm_context), source(source), file_path(std::move(file_path)), file_record(file_record), buildDir(std::move(buildDir)), relativePath(std::move(relativePath)) {

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
    auto builtins = new Enviornment(nullptr, StrRecordMap(), "builtins");
    env = new Enviornment(builtins, StrRecordMap());
    this->env->childes.push_back(builtins);
    this->file_record->env = this->env;
}

void Compiler::addBuiltinType(const Str& name, llvm::Type* type) {
    auto record = new RecordStructType(name, type);
    this->env->parent->addRecord(record);
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

    addBuiltinType("raw_array", this->ll_pointer);
    this->gc_raw_array = env->parent->getStruct("raw_array");
    this->ll_raw_array = this->ll_pointer;

    addBuiltinType("nullptr", this->ll_pointer);

    auto _Any = new RecordModule("Any");
    env->parent->addRecord(_Any);
    // Initializing Built-in Types
    this->_initilizeCSTDLib();
}

void Compiler::compile(AST::Node* node) {
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
        case AST::NodeType::SwitchCaseStatement:
            this->_visitSwitchCaseStatement(node->castToSwitchCaseStatement());
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
            errors::raiseCompletionError(this->file_path,
                                         this->source,
                                         node->meta_data.st_line_no,
                                         node->meta_data.st_col_no,
                                         node->meta_data.end_line_no,
                                         node->meta_data.end_col_no,
                                         "Unknown node type: " + AST::nodeTypeToString(node->type()));
    }
}

void Compiler::_visitBreakStatement(AST::BreakStatement* node) {
    if (this->env->loop_conti_block.empty()) {
        errors::raiseNodeOutsideError(this->file_path, this->source, node, errors::OutsideNodeType::Break, "Break statement outside the Loop", "Remove the Break statement, it is not necessary");
    }
    if (node->loopIdx >= this->env->loop_ifbreak_block.size()) {
        errors::raiseCompletionError(this->file_path,
                                     this->source,
                                     node->meta_data.st_line_no,
                                     node->extra_info.int_map["idx_stcol_no"],
                                     node->meta_data.end_line_no,
                                     node->extra_info.int_map["idx_endcol_no"],
                                     "Loop index " + std::to_string(node->loopIdx) + " is out of range. Maximum allowed index is " + std::to_string(this->env->loop_ifbreak_block.size() - 1) + ".",
                                     "Ensure that the loop index is within the valid range. Remember: LoopIdx starts with `0`.");
    }
    // break 0; == break; & .size() return 1 if it holds 1 iten not when it holds 2 thats why - 1
    if (this->env->loop_ifbreak_block.at(this->env->loop_ifbreak_block.size() - node->loopIdx - 1))
        // jumps to `ifbreak` block if it exists
        this->llvm_ir_builder.CreateBr(this->env->loop_ifbreak_block.at(this->env->loop_ifbreak_block.size() - node->loopIdx - 1));
    else this->llvm_ir_builder.CreateBr(this->env->loop_conti_block.at(this->env->loop_conti_block.size() - node->loopIdx - 1));
    throw DoneBr();
}

void Compiler::_visitContinueStatement(AST::ContinueStatement* node) {
    if (this->env->loop_condition_block.empty()) {
        errors::raiseNodeOutsideError(this->file_path,
                                      this->source,
                                      node,
                                      errors::OutsideNodeType::Continue,
                                      "Continue statement outside the Loop",
                                      "Remove the Continue statement, it is not necessary");
    }
    if (node->loopIdx >= this->env->loop_ifbreak_block.size()) {
        errors::raiseCompletionError(this->file_path,
                                     this->source,
                                     node->meta_data.st_line_no,
                                     node->extra_info.int_map["idx_stcol_no"],
                                     node->meta_data.end_line_no,
                                     node->extra_info.int_map["idx_endcol_no"],
                                     "Loop Index is out of range",
                                     "Remember: LoopIdx start with `0`");
    }
    // continue 0; == continue; & .size() return 1 if it holds 1 iten not when it holds 2 thats why - 1
    this->llvm_ir_builder.CreateBr(this->env->loop_condition_block.at(this->env->loop_condition_block.size() - node->loopIdx - 1));
    throw DoneBr();
}

void Compiler::_visitProgram(AST::Program* program) {
    for (const auto& stmt : program->statements) { this->compile(stmt); }
}

void Compiler::_visitExpressionStatement(AST::ExpressionStatement* expression_statement) {
    this->compile(expression_statement->expr);
}

void Compiler::_visitBlockStatement(AST::BlockStatement* block_statement) {
    for (const auto& stmt : block_statement->statements) { this->compile(stmt); }
}

void Compiler::_checkAndConvertCallType(std::vector<RecordFunction*> func_records, AST::CallExpression* func_call, vector<llvm::Value*>& args, const vector<RecordStructType*>& params_types) {
    for (auto func_record : func_records)
        if (func_record->is_var_arg) return;
    vector<vector<unsigned short>> mismatches;
    for (auto func_record : func_records) {
        vector<unsigned short> mismatch_indices;
        size_t limit = std::min(func_record->arguments.size(), params_types.size());
        for (size_t idx = 0; idx < limit; ++idx) {
            auto pt = func_record->arguments[idx];
            auto pst = params_types[idx];
            auto expected_type = std::get<1>(pt);
            if (_checkType(expected_type, pst)) {
                // Type Is Same So No need to do any thing
            } else if (this->canConvertType(pst, expected_type)) {
                args[idx] = this->convertType({args[idx], nullptr, pst}, expected_type).value;
            } else {
                mismatch_indices.push_back(idx);
            }
        }
        if (func_record->arguments.size() != params_types.size()) {
            size_t max_size = std::max(func_record->arguments.size(), params_types.size());
            for (size_t idx = limit; idx < max_size; ++idx) { mismatch_indices.push_back(idx); }
        }
        if (!mismatch_indices.empty()) {
            mismatches.push_back(mismatch_indices);
        } else return;
    }
    if (!mismatches.empty()) errors::raiseNoOverloadError(this->file_path, this->source, mismatches, func_call, "Cannot call the function with wrong type");
}

Compiler::ResolvedValue
Compiler::_CallGfunc(const vector<RecordGenericFunction*>& gfuncs, AST::CallExpression* func_call, const Str& name, vector<llvm::Value*>& args, const vector<RecordStructType*>& params_types) {
    // Attempt to find and call a matching function overload
    for (const auto& gfunc : gfuncs) {
        if (gfunc->env->isFunction(name, params_types, false, true)) {
            // Match found: Retrieve function record
            auto func_record = gfunc->env->getFunction(name, params_types, false, true);

            // Validate and convert argument types as necessary
            this->_checkAndConvertCallType({func_record}, func_call, args, params_types);

            // Create LLVM call instruction
            auto returnValue = this->llvm_ir_builder.CreateCall(func_record->function, args, name + "_result");

            // Return the resolved value
            return {returnValue, nullptr, func_record->return_type, resolveType::StructInst};
        }
    }

    // No exact match found; prepare to handle generic functions
    auto prev_env = this->env;                 // Save current environment
    vector<vector<unsigned short>> mismatches; // Track parameter mismatches

    for (const auto& gfunc : gfuncs) {
        // Create a new environment for each generic function
        this->env = new Enviornment(prev_env, {}, name);
        prev_env->childes.push_back(this->env);
        vector<unsigned short> mismatch_indices;

        // Iterate over parameters to check type compatibility
        for (const auto& [idx, gparam] : llvm::enumerate(gfunc->func->parameters)) {
            const auto& pparam = params_types[idx];

            // Handle identifier literals by creating a new struct record
            if (gparam->value_type->name->type() == AST::NodeType::IdentifierLiteral) {
                auto struct_record = new RecordStructType(*pparam);
                struct_record->name = gparam->value_type->name->castToIdentifierLiteral()->value;
                this->env->addRecord(struct_record);
                continue;
            }

            // Resolve the type of the generic parameter
            auto gparam_resolved = this->_resolveValue(gparam->value_type->name);
            auto gparam_type = gparam_resolved.type;
            auto gparam_variant = std::get<RecordStructType*>(gparam_resolved.variant);

            // Check for exact type match or convertible types
            if (gparam_type == resolveType::StructType && _checkType(gparam_variant, pparam)) {
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
        vector<RecordStructType*> param_struct_types;
        vector<bool> param_references;

        // Prepare LLVM parameter types and names
        for (size_t i = 0; i < params.size(); ++i) {
            auto param = params[i];
            auto param_type = params_types[i];
            param_names.push_back(param->name->castToIdentifierLiteral()->value);
            param_struct_types.push_back(param_type);
            param_references.push_back(param->value_type->refrence);
            llvm_param_types.push_back(param_type->struct_type || name == "raw_array" ? this->ll_pointer : param_type->stand_alone_type);
        }

        // Determine the LLVM return type
        auto return_type = this->_parseType(gfunc->func->return_type);
        auto llvm_return_type = return_type->struct_type || return_type->name == "raw_array" ? this->ll_pointer : return_type->stand_alone_type;

        // Create LLVM function type and function
        auto func_type = llvm::FunctionType::get(llvm_return_type, llvm_param_types, false);
        auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, this->fc_st_name_prefix != "main.gc.." ? this->fc_st_name_prefix + name : name, this->llvm_module.get());

        // Assign names to LLVM function arguments
        for (const auto& [idx, arg] : llvm::enumerate(func->args())) { arg.setName(param_names[idx]); }

        // Create a record for the new function
        auto func_record = new RecordFunction(name, func, func_type, {}, return_type, gfunc->func->extra_info);

        if (body) {
            // Create entry basic block for the function
            auto bb = llBB::Create(this->llvm_context, "entry", func);
            this->function_entry_block.push_back(bb);
            this->llvm_ir_builder.SetInsertPoint(bb);
            this->env->current_function = func_record;

            // Allocate and store function arguments
            for (const auto& [idx, arg] : llvm::enumerate(func->args())) {
                auto param_type = param_struct_types[idx];

                // Create alloca for the argument if necessary
                llvm::Value* alloca = &arg;
                if (!arg.getType()->isPointerTy() || _checkType(param_type, this->env->getStruct("raw_array"))) {
                    alloca = this->llvm_ir_builder.CreateAlloca(arg.getType(), nullptr, arg.getName() + "_og");
                    this->llvm_ir_builder.CreateStore(&arg, alloca);
                }

                // Load argument value if it's a reference
                llvm::Value* arg_value = param_references[idx] ? this->llvm_ir_builder.CreateLoad(param_type->stand_alone_type, &arg, "loaded_" + arg.getName()) : alloca;
                // Create and add a record for the argument variable
                auto record = new RecordVariable(Str(arg.getName()), arg_value, alloca, param_type);
                func_record->arguments.emplace_back(arg.getName().str(), param_type, param_references[idx]);
                this->env->addRecord(record);
            }

            // Set metadata for the function record
            func_record->set_meta_data(gfunc->func->meta_data.st_line_no, gfunc->func->meta_data.st_col_no, gfunc->func->meta_data.end_line_no, gfunc->func->meta_data.end_col_no);

            // Add the function record to the environment
            this->env->addRecord(new RecordFunction(*func_record));

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
        return {returnValue, nullptr, func_record->return_type, resolveType::StructInst};
    }

    // Handle cases where no overload matches
    if (mismatches.empty()) {
        errors::raiseNotDefinedError(this->file_path, this->source, func_call->name, "Function dose not Exist.", "Check the function name or define the function.");
    } else {
        errors::raiseNoOverloadError(this->file_path, this->source, mismatches, func_call, "Argument types do not match any overload.", "Check the argument types or define an appropriate overload.");
    }
};

void Compiler::_createFunctionRecord(AST::FunctionStatement* function_declaration_statement, RecordStructType* struct_, RecordModule* module, compilationState::RecordFile* local_file_record) {
    // Extract the function name from the AST
    auto name = function_declaration_statement->name->castToIdentifierLiteral()->value;

    // Handle generic functions separately
    if (!function_declaration_statement->generic.empty()) {
        auto gsr = new RecordGenericFunction(name, function_declaration_statement, this->env);

        if (module) {
            // If within a module, add to the module's record map
            module->record_map.push_back({name, gsr});
        } else if (struct_) {
            // Generics are not supported for struct members; raise an error
            errors::raiseCompletionError("GenericInMethod",
                                         this->source,
                                         gsr->meta_data.st_line_no,
                                         gsr->meta_data.st_col_no,
                                         gsr->meta_data.end_line_no,
                                         gsr->meta_data.end_col_no,
                                         "Generics do not support struct members");
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
    vector<std::tuple<Str, RecordStructType*, bool>> arguments;
    vector<RecordStructType*> args;

    for (const auto& param : params) {
        // Extract parameter name
        auto param_name = param->name->castToIdentifierLiteral()->value;
        param_names.push_back(param_name);

        // Parse parameter type
        RecordStructType* param_type = this->_parseType(param->value_type);
        llvm::Type* llvm_param_type = param_type->struct_type || param_type->name == "raw_array"
            ? this->ll_pointer
            : (param->value_type->refrence ? llvm::PointerType::getUnqual(param_type->stand_alone_type) : param_type->stand_alone_type);
        param_types.push_back(llvm_param_type);

        // Store argument details for later use
        args.push_back(param_type);
        arguments.emplace_back(param_name, param_type, param->value_type->refrence);
    }

    // Determine the return type of the function
    RecordStructType* return_type = function_declaration_statement->return_type ? this->_parseType(function_declaration_statement->return_type) : this->env->getStruct("void");
    llvm::Type* llvm_return_type = return_type->struct_type || return_type->name == "raw_array" ? this->ll_pointer : return_type->stand_alone_type;

    // Create the LLVM function type
    auto func_type = llvm::FunctionType::get(llvm_return_type, param_types, false);

    // Prefix the function name if necessary
    Str func_name;
    if (module) func_name = local_file_record->env->getFunction(name, args)->ll_name;
    else if (struct_ && local_file_record) func_name = local_file_record->env->getStruct(struct_->name)->get_method(name, args)->function->getName().str();
    else func_name = this->fc_st_name_prefix != "main.gc.." ? this->fc_st_name_prefix + name : name;

    // Create the LLVM function and add it to the module
    auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, func_name, this->llvm_module.get());
    // Assign names to the function arguments for readability
    size_t idx = 0;
    for (auto& arg : func->args()) { arg.setName(param_names[idx++]); }

    // Create a RecordFunction to keep track of the function's metadata and environment
    auto func_record = new RecordFunction(name, func, func_type, arguments, return_type, function_declaration_statement->extra_info);
    func_record->ll_name = func->getName().str();

    // Add the function record to the appropriate scope (struct, module, or global environment)
    if (struct_) {
        struct_->methods.emplace_back(name, func_record);
    } else if (module) {
        module->record_map.emplace_back(name, func_record);
        return;
    } else if (struct_ && local_file_record) {
        struct_->addMethod(name, func_record);
        return;
    } else {
        this->env->addRecord(func_record);
    }

    // If the function has a body, proceed to compile it
    if (auto body = function_declaration_statement->body) {
        // Create the entry basic block for the function
        auto bb = llvm::BasicBlock::Create(this->llvm_context, "entry", func);
        this->function_entry_block.push_back(bb);
        this->llvm_ir_builder.SetInsertPoint(bb);
        // if (this->fc_st_name_prefix == "main.gc.." && name == "main") { func->setGC("statepoint-example"); }

        // Save the current environment and create a new one for the function
        auto prev_env = this->env;
        auto new_env = Enviornment(prev_env);
        this->env = &new_env;
        this->env->current_function = func_record;

        // Initialize function arguments in the new environment
        size_t arg_idx = 0;
        for (auto& arg : func->args()) {
            const auto& argument = arguments[arg_idx++];
            Str arg_name = std::get<0>(argument);
            RecordStructType* param_type_record = std::get<1>(argument);
            bool is_reference = std::get<2>(argument);

            llvm::Value* alloca = &arg;
            // Allocate space for the argument if it's not a pointer type or if it's an raw_array
            if (!std::get<2>(argument)) {
                alloca = this->llvm_ir_builder.CreateAlloca(arg.getType(), nullptr, arg.getName());
                this->llvm_ir_builder.CreateStore(&arg, alloca);
            }

            // Create a variable record for the argument and add it to the environment
            auto record = new RecordVariable(Str(arg.getName()), nullptr, alloca, param_type_record);
            this->env->addRecord(record);
        }

        // Set metadata for the function (e.g., source code location)
        func_record->set_meta_data(function_declaration_statement->meta_data.st_line_no,
                                   function_declaration_statement->meta_data.st_col_no,
                                   function_declaration_statement->meta_data.end_line_no,
                                   function_declaration_statement->meta_data.end_col_no);

        // Compile the function body within a try-catch to handle early returns or branches
        this->env->addRecord(new RecordFunction(*func_record));
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

void Compiler::_visitFunctionDeclarationStatement(AST::FunctionStatement* function_declaration_statement, RecordStructType* struct_) {
    this->_createFunctionRecord(function_declaration_statement, struct_, nullptr);
}

Compiler::ResolvedValue Compiler::_visitCallExpression(AST::CallExpression* call_expression) {
    auto name = call_expression->name->castToIdentifierLiteral()->value;
    auto param = call_expression->arguments;
    if (name == "raw_array") {
        if (param.size() != 2) {
            errors::raiseCompletionError(this->file_path,
                                     this->source,
                                     call_expression->meta_data.st_line_no,
                                     call_expression->meta_data.st_col_no,
                                     call_expression->meta_data.end_line_no,
                                     call_expression->meta_data.end_col_no,
                                     "raw_array constructor requires exactly 2 arguments: type and size",
                                     "Pass both a type argument and size argument");
        }

        // First param is the type
        auto [type_val, type_alloca, _type_param_type, type_resolve_type] = this->_resolveValue(param[0]);
        if (type_resolve_type != resolveType::StructType) {
            errors::raiseWrongTypeError(this->file_path, this->source, param[0], nullptr, {}, "First argument must be a type");
        }
        auto raw_array_type = std::get<RecordStructType*>(_type_param_type);

        // Second param is the size
        auto [size_val, size_alloca, _size_param_type, size_resolve_type] = this->_resolveValue(param[1]);
        auto size_type = std::get<RecordStructType*>(_size_param_type);
        if (!_checkType(size_type, this->env->getStruct("int"))) {
            errors::raiseWrongTypeError(this->file_path, this->source, param[1], size_type, {this->env->getStruct("int")}, "Second argument must be an integer size");
        }

        llvm::Value* raw_array;
        if (call_expression->_new) {
            // Create array type and calculate allocation size
            llvm::Type* element_type = raw_array_type->struct_type ? raw_array_type->struct_type : raw_array_type->stand_alone_type;
            auto array_type = llvm::ArrayType::get(element_type, 0);
            llvm::Value* element_size = this->llvm_ir_builder.CreateGEP(element_type, llvm::ConstantPointerNull::get(element_type->getPointerTo()), llConstInt::get(this->ll_int, 1));
            llvm::Value* alloc_size = this->llvm_ir_builder.CreateMul(this->llvm_ir_builder.CreatePtrToInt(element_size, this->ll_int), size_val);

            // Allocate memory for array
            auto malloc_fn = this->env->getFunction("malloc", {this->env->getStruct("int")});
            raw_array = this->llvm_ir_builder.CreateCall(malloc_fn->function, {alloc_size});
        } else {
            // Create the LLVM raw_array type
            raw_array = this->llvm_ir_builder.CreateAlloca(
                raw_array_type->struct_type ? raw_array_type->struct_type : raw_array_type->stand_alone_type,
                size_val ? size_val : this->llvm_ir_builder.CreateLoad(this->ll_int, size_alloca));
        }

        // Create array struct type
        auto array_struct = new RecordStructType(*this->env->getStruct("raw_array"));
        array_struct->generic_sub_types.push_back(raw_array_type);

        return {raw_array, raw_array, array_struct, resolveType::StructInst};
    }
    vector<llvm::Value*> args;
    vector<llvm::Value*> arg_allocas;
    vector<RecordStructType*> params_types;

    for (auto arg : param) {
        auto [value, alloca, _param_type, ptt] = this->_resolveValue(arg);
        auto param_type = std::get<RecordStructType*>(_param_type);
        if (ptt == resolveType::Module) {
            errors::raiseWrongTypeError(this->file_path, this->source, arg, nullptr, {}, "Cant pass Module to the Function");
        } else if ((ptt == resolveType::StructType && param_type->name != "nullptr") && !this->env->isGenericStruct(name)) {
            errors::raiseWrongTypeError(this->file_path, this->source, arg, nullptr, {}, "Cant pass type to the Function");
        }
        params_types.push_back(param_type);
        if (!value && !alloca) {
            args.push_back(nullptr);
            arg_allocas.push_back(nullptr);
            continue;
        }
        args.push_back(value ? value : this->llvm_ir_builder.CreateLoad(param_type->struct_type || param_type->name == "raw_array" ? this->ll_pointer : param_type->stand_alone_type, alloca));
        arg_allocas.push_back(alloca);
    }

    if (this->env->isGenericFunc(name) ? this->env->isFunction(name, params_types, false, true) : this->env->isFunction(name, params_types)) {
        auto func = this->env->isGenericFunc(name) ? this->env->getFunction(name, params_types, false, true) : this->env->getFunction(name, params_types);
        unsigned short idx = 0;
        for (auto [arg_alloca, param_type, argument] : llvm::zip(arg_allocas, params_types, func->arguments)) {
            if (param_type->stand_alone_type && std::get<2>(argument)) { args[idx] = arg_alloca; }
            idx++;
        }
        this->_checkAndConvertCallType({func}, call_expression, args, params_types);
        auto returnValue = this->llvm_ir_builder.CreateCall(func->function, args);
        return {returnValue, nullptr, func->return_type, resolveType::StructInst};
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
    std::vector<RecordFunction*> func_records;
    for (auto func : this->env->getFunc(name)) { func_records.push_back(func); }
    this->_checkAndConvertCallType(func_records, call_expression, args, params_types);
    errors::raiseCompletionError(this->file_path,
                                 this->source,
                                 call_expression->meta_data.st_line_no,
                                 call_expression->meta_data.st_col_no,
                                 call_expression->meta_data.end_line_no,
                                 call_expression->meta_data.end_col_no,
                                 "Function `" + name + "` not defined");
};

Compiler::ResolvedValue Compiler::_callStruct(RecordStructType* struct_record, AST::CallExpression* call_expression, vector<RecordStructType*> params_types, LLVMValueVector args) {
    auto name = struct_record->name;
    auto struct_type = struct_record->struct_type;

    llvm::Value* alloca;

    // Lambda to handle initialization for new struct instances
    auto initializeNewStruct = [&](llvm::Value* allocaPtr) {
        // Calculate size of the struct
        llvm::Value* gep = this->llvm_ir_builder.CreateGEP(struct_type, llvm::ConstantPointerNull::get(struct_type->getPointerTo()), llConstInt::get(llvm::Type::getInt64Ty(this->llvm_context), 1));
        llvm::Value* size = this->llvm_ir_builder.CreatePtrToInt(gep, llvm::Type::getInt64Ty(this->llvm_context));

        alloca = this->llvm_ir_builder.CreateCall(this->env->getFunction("malloc", {this->env->getStruct("int")})->function, {size});
    };

    // Initialize based on whether it's a new struct instance
    if (call_expression->_new) {
        initializeNewStruct(alloca);
    } else {
        alloca = this->llvm_ir_builder.CreateAlloca(struct_type, nullptr, name);
    }
    // Prepare parameters and arguments for the __init__ method
    params_types.insert(params_types.begin(), struct_record);
    args.insert(args.begin(), alloca);

    // Retrieve the __init__ method
    auto func = struct_record->get_method("__init__", params_types);

    if (func) {
        // Validate and convert argument types
        this->_checkAndConvertCallType({func}, call_expression, args, params_types);
        // Call the __init__ method
        this->llvm_ir_builder.CreateCall(func->function, args);
    } else {
        std::vector<RecordFunction*> methods = {};
        for (auto [_, method] : struct_record->methods) {
            if (method->name == "__init__") { methods.push_back(method); }
        }
        _checkAndConvertCallType(methods, call_expression, args, params_types);
        // Raise an error if __init__ is not defined
        errors::raiseNoOverloadError(this->file_path,
                                     this->source,
                                     {},
                                     call_expression,
                                     "Initialization method does not exist for struct " + struct_record->name + ".",
                                     "Check the initialization method name or define the method.");
    }

    // Return the resolved value indicating a struct instance
    return {alloca, alloca, struct_record, resolveType::StructInst};
}

Compiler::ResolvedValue
Compiler::_handleStructIndexing(llvm::Value* left_alloca, llvm::Value* index, RecordStructType* index_generic, RecordStructType* left_generic, AST::IndexExpression* index_expression) {
    if (left_generic->is_method("__index__", {left_generic, index_generic})) {
        auto idx_method = left_generic->get_method("__index__", {left_generic, index_generic});
        auto returnValue = this->llvm_ir_builder.CreateCall(idx_method->function, {left_alloca, index});
        return {returnValue, nullptr, idx_method->return_type, resolveType::StructInst};
    }

    _raiseNoIndexMethodError(left_generic, index_expression);
}

Compiler::ResolvedValue
Compiler::_CallGstruct(const vector<RecordGenericStructType*>& gstructs, AST::CallExpression* func_call, const Str& name, vector<llvm::Value*>& args, const vector<RecordStructType*>& params_types) {
    auto prev_env = this->env; // Save the current environment

    for (const auto& gstruct : gstructs) {
        // Skip if there aren't enough parameters for generics
        if (params_types.size() < gstruct->structAST->generics.size()) { continue; }

        // Set up a new environment based on the generic struct's environment
        this->env = new Enviornment(gstruct->env);
        prev_env->childes.push_back(this->env);
        vector<RecordStructType*> generics;
        vector<Str> generic_names;
        vector<llvm::Value*> remaining_args(args);
        vector<RecordStructType*> remaining_params_types(params_types);

        // Map provided parameter types to the struct's generics
        auto generic_iter = llvm::zip(params_types, gstruct->structAST->generics);
        for (const auto& [pt, generic] : generic_iter) {
            auto pt_copy = new RecordStructType(*pt);
            generic_names.push_back(pt_copy->name);
            pt_copy->name = generic->name->castToIdentifierLiteral()->value;
            this->env->addRecord(pt_copy);
            generics.push_back(pt_copy);
            remaining_args.erase(remaining_args.begin());
            remaining_params_types.erase(remaining_params_types.begin());
        }

        Str struct_name = gstruct->structAST->name->castToIdentifierLiteral()->value;
        auto struct_record = new RecordStructType(struct_name);

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

                    // Determine the LLVM type based on whether it's a standalone type or an raw_array
                    llvm::Type* llvm_field_type = (field_type->struct_type || field_type->name == "raw_array") ? this->ll_pointer : field_type->stand_alone_type;
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
                    // Process function declarations within the struct
                    this->_processFieldFunction(field, struct_record);
                }
            }

            // Add the struct record to the generic struct's environment and restore the previous environment
            gstruct->env->addRecord(new RecordStructType(*struct_record));
        } else {
            // Retrieve the existing struct record with the specified generics
            delete struct_record;
            struct_record = gstruct->env->getStruct(struct_name, false, generics);
        }

        // Perform the struct call and return the resolved value
        auto x = _callStruct(struct_record, func_call, remaining_params_types, remaining_args);
        this->env = prev_env;
        return x;
    }

    // If no matching struct overload is found, raise an error and exit
    errors::raiseNoOverloadError(this->file_path,
                                 this->source,
                                 {},
                                 func_call,
                                 "Struct overload does not exist.",
                                 "Check the argument types or define an appropriate overload, first pass types & then init function params.");
};

void Compiler::_handleFieldDeclaration(RecordGenericStructType* gstruct, AST::Node* field, RecordStructType* struct_record, vector<llvm::Type*>& field_types, const Str& struct_name) {
    auto field_decl = field->castToVariableDeclarationStatement();
    Str field_name = field_decl->name->castToIdentifierLiteral()->value;
    auto field_type = this->_parseType(field_decl->value_type);
    struct_record->addSubType(field_name, field_type);

    field_types.push_back(field_type->struct_type || field_type->name == "raw_array" ? this->ll_pointer : field_type->stand_alone_type);

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

void Compiler::_createStructRecord(AST::StructStatement* struct_statement, RecordModule* module, compilationState::RecordFile* local_file_record) {
    // Extract the struct name
    Str struct_name = struct_statement->name->castToIdentifierLiteral()->value;

    // Handle generic structs early and return
    if (!struct_statement->generics.empty()) {
        auto gsr = new RecordGenericStructType(struct_name, struct_statement, this->env);
        this->env->addRecord(gsr);
        if (module) { module->record_map.emplace_back(struct_name, gsr); }
        return;
    }

    // Create a new record for the struct
    auto struct_record = new RecordStructType(struct_name);

    // Prepare to parse struct fields
    vector<llvm::Type*> field_types;
    auto fields = struct_statement->fields;

    // Save and update the environment for struct scope
    auto prev_env = this->env;
    this->env = new Enviornment(prev_env);
    prev_env->childes.push_back(this->env);
    this->env->addRecord(struct_record); // Dont Copy struct type here because it's get modify down the road

    // Iterate over each field in the struct
    for (auto field : fields) {
        if (field->type() == AST::NodeType::VariableDeclarationStatement) {
            // Handle variable declarations within the struct
            auto field_decl = field->castToVariableDeclarationStatement();
            Str field_name = field_decl->name->castToIdentifierLiteral()->value;
            auto field_type = this->_parseType(field_decl->value_type);

            // Determine the LLVM type based on whether it's a standalone type or an raw_array
            llvm::Type* llvm_field_type = (field_type->struct_type || field_type->name == "raw_array") ? this->ll_pointer : field_type->stand_alone_type;
            field_types.push_back(llvm_field_type);
            struct_record->addSubType(field_name, field_type);

            // Create and set the LLVM struct type
            auto struct_type = llvm::StructType::create(this->llvm_context, field_types, this->fc_st_name_prefix + struct_name);
            struct_type->setBody(field_types);
            struct_record->struct_type = struct_type;
        } else {
            // Process function declarations within the struct
            this->_processFieldFunction(field, struct_record, local_file_record);
        }
    }

    // Restore the previous environment
    this->env = prev_env;

    // Add the struct record to the module or environment
    if (module) {
        module->record_map.emplace_back(struct_name, struct_record);
    } else {
        this->env->addRecord(new RecordStructType(*struct_record));
    }
}

void Compiler::_handleGenericSubType(AST::Node* field, AST::StructStatement* struct_statement, RecordStructType* struct_record, RecordStructType* field_type) {
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

void Compiler::_processFieldFunction(AST::Node* field, RecordStructType* struct_record, compilationState::RecordFile* local_file_record) {
    auto func_dec = field->castToFunctionStatement();

    // Ensure the function is not generic
    if (!func_dec->generic.empty()) {
        errors::raiseCompletionError(this->file_path,
                                     this->source,
                                     func_dec->name->meta_data.st_line_no,
                                     func_dec->name->meta_data.st_col_no,
                                     func_dec->name->meta_data.end_line_no,
                                     func_dec->name->meta_data.end_col_no,
                                     "Struct Methods do not support Generic Functions",
                                     "Set the Generic on the struct");
    }

    // Create the function record within the struct
    this->_createFunctionRecord(func_dec, struct_record, nullptr, local_file_record);
}

void Compiler::_visitStructStatement(AST::StructStatement* struct_statement) {
    this->_createStructRecord(struct_statement, nullptr);
}

Compiler::ResolvedValue Compiler::_memberAccess(AST::InfixExpression* infixed_expression) {
    auto left = infixed_expression->left;
    auto right = infixed_expression->right;
    auto [left_value, left_alloca, _left_type, ltt] = this->_resolveValue(left);
    if (right->type() == AST::NodeType::IdentifierLiteral) {
        if (ltt == resolveType::Module) {
            auto module = std::get<RecordModule*>(_left_type);
            auto name = right->castToIdentifierLiteral()->value;
            if (module->is_module(name)) {
                return Compiler::ResolvedValue(nullptr, nullptr, module->get_module(name), resolveType::Module);
            } else if (module->is_struct(name)) {
                return Compiler::ResolvedValue(nullptr, nullptr, module->get_struct(name), resolveType::StructType);
            } else if (module->isGenericStruct(name)) {
                return Compiler::ResolvedValue(nullptr, nullptr, module->getGenericStruct(name), resolveType::GStructType);
            } else {
                errors::raiseDoesntContainError(this->file_path,
                                                this->source,
                                                right->castToIdentifierLiteral(),
                                                left,
                                                "no member `" + name + "` not found in module " + module->name,
                                                "Check the Member Name in the module is correct");
            }
        } else if (ltt == resolveType::StructInst) {
            auto left_type = std::get<RecordStructType*>(_left_type);
            if (left_type->sub_types.contains(right->castToIdentifierLiteral()->value)) {
                unsigned short idx = 0;
                for (auto field : left_type->getFields()) {
                    if (field == right->castToIdentifierLiteral()->value) { break; }
                    idx++;
                }
                auto type = left_type->sub_types[right->castToIdentifierLiteral()->value];
                llvm::Value* gep = this->llvm_ir_builder.CreateStructGEP(left_type->struct_type, left_value, idx, "accesed" + right->castToIdentifierLiteral()->value + "_from_" + left_type->name);
                return {nullptr, gep, type, resolveType::StructInst};
            } else {
                errors::raiseDoesntContainError(this->file_path,
                                                this->source,
                                                right->castToIdentifierLiteral(),
                                                left,
                                                "no member `" + right->castToIdentifierLiteral()->value + "` not found in struct " + left_type->name,
                                                "Check the Member Name in the struct is correct");
            }
        }
    } else if (right->type() == AST::NodeType::CallExpression) {
        auto call_expression = right->castToCallExpression();
        auto name = call_expression->name->castToIdentifierLiteral()->value;
        auto params = call_expression->arguments;
        vector<llvm::Value*> args;
        vector<llvm::Value*> arg_allocas;
        vector<RecordStructType*> params_types;
        for (auto arg : params) {
            auto [value, val_alloca, param_type, ptt] = this->_resolveValue(arg);
            if (ptt == resolveType::Module) {
                errors::raiseWrongTypeError(this->file_path, this->source, arg, nullptr, {}, "Cant pass Module to the Function");
            } else if ((ptt == resolveType::StructType && std::get<RecordStructType*>(param_type)->name != "nullptr" &&
                        !(ltt == resolveType::Module && std::get<RecordModule*>(_left_type)->isGenericStruct(name))) ||
                       ptt == resolveType::GStructType) {
                errors::raiseWrongTypeError(this->file_path, this->source, arg, nullptr, {}, "Cant pass type to the Function");
            }
            params_types.push_back(std::get<RecordStructType*>(param_type));
            arg_allocas.push_back(val_alloca);
            args.push_back(value);
        }
        if (ltt == resolveType::Module) {
            auto left_type = std::get<RecordModule*>(_left_type);
            if (left_type->isGenericFunc(name) ? left_type->isFunction(name, params_types, true) : left_type->isFunction(name, params_types)) {
                auto func = left_type->getFunction(name, params_types);
                unsigned short idx = 0;
                _checkAndConvertCallType({func}, call_expression, args, params_types);
                for (auto [arg_alloca, param_type, argument] : llvm::zip(arg_allocas, params_types, func->arguments)) {
                    if (param_type->stand_alone_type && std::get<2>(argument)) { args[idx] = arg_alloca; }
                    idx++;
                }
                auto returnValue = this->llvm_ir_builder.CreateCall(func->function, args, name + "_result");
                return {returnValue, nullptr, func->return_type, resolveType::StructInst};
            } else if (left_type->isGenericFunc(name)) {
                auto gfuncs = left_type->get_GenericFunc(name);
                return this->_CallGfunc(gfuncs, call_expression, name, args, params_types);
            } else if (left_type->isGenericStruct(name)) {
                auto gstruct = left_type->getGenericStruct(name);
                return this->_CallGstruct(gstruct, call_expression, name, args, params_types);
            } else if (left_type->is_struct(name)) {
                return this->_callStruct(left_type->get_struct(name), call_expression, params_types, args);
            } else {
                errors::raiseDoesntContainError(this->file_path,
                                                this->source,
                                                call_expression->name->castToIdentifierLiteral(),
                                                left,
                                                "Struct Or Function " + name + " overload Dose Not Exit.",
                                                "Check the Name is Correct or the params are correct");
            }
        } else if (ltt == resolveType::StructInst) {
            auto left_type = std::get<RecordStructType*>(_left_type);
            params_types.insert(params_types.begin(), left_type);
            args.insert(args.begin(), left_value ? left_value : left_alloca);
            arg_allocas.insert(arg_allocas.begin(), left_value ? left_value : left_alloca);
            auto name = right->castToCallExpression()->name->castToIdentifierLiteral()->value;
            if (left_type->is_method(name, params_types)) {
                auto method = left_type->get_method(name, params_types);
                unsigned short idx = 0;
                _checkAndConvertCallType({method}, call_expression, args, params_types);
                for (auto [arg_alloca, param_type, argument] : llvm::zip(arg_allocas, params_types, method->arguments)) {
                    if (param_type->stand_alone_type && std::get<2>(argument)) { args[idx] = arg_alloca; }
                    idx++;
                }
                auto returnValue = this->llvm_ir_builder.CreateCall(method->function, args, method->return_type->stand_alone_type != this->ll_void ? name + "_reuturn_value" : "");
                return {returnValue, nullptr, method->return_type, resolveType::StructInst};
            } else {
                errors::raiseNoOverloadError(this->file_path,
                                             this->source,
                                             {},
                                             call_expression,
                                             "method does not exist for struct " + left_type->name + ".",
                                             "Check the initialization method name or define the method.");
            }
        }
    }
    errors::raiseCompletionError(this->file_path,
                                 this->source,
                                 right->meta_data.st_line_no,
                                 right->meta_data.st_col_no,
                                 right->meta_data.end_line_no,
                                 right->meta_data.end_col_no,
                                 "Member access should be identifier of method not " + AST::nodeTypeToString(right->type()));
};

Compiler::ResolvedValue Compiler::_StructInfixCall(const Str& op_method,
                                                   const Str& op,
                                                   RecordStructType* left_type,
                                                   RecordStructType* right_type,
                                                   AST::Expression* left,
                                                   AST::Expression* right,
                                                   llvm::Value* left_value,
                                                   llvm::Value* left_alloca,
                                                   llvm::Value* right_value,
                                                   llvm::Value* right_alloca) {
    vector<RecordStructType*> params_type1{left_type, right_type};
    vector<RecordStructType*> params_type2{right_type, left_type};
    if (left_type->is_method(op_method, params_type1)) {
        auto func_record = left_type->get_method(op_method, params_type1);
        auto returnValue = this->llvm_ir_builder.CreateCall(
            func_record->function,
            {left_value ? left_value : this->llvm_ir_builder.CreateLoad(left_type->struct_type ? left_type->struct_type : left_type->stand_alone_type, left_alloca),
             right_value ? right_value : this->llvm_ir_builder.CreateLoad(right_type->struct_type ? right_type->struct_type : right_type->stand_alone_type, right_alloca)});
        return {returnValue, nullptr, func_record->return_type, resolveType::StructInst};
    } else if (right_type->is_method(op_method, params_type2)) {
        auto func_record = right_type->get_method(op_method, params_type2);
        auto returnValue = this->llvm_ir_builder.CreateCall(
            func_record->function,
            {right_value ? right_value : this->llvm_ir_builder.CreateLoad(right_type->struct_type ? right_type->struct_type : right_type->stand_alone_type, right_alloca),
             left_value ? left_value : this->llvm_ir_builder.CreateLoad(left_type->struct_type ? left_type->struct_type : left_type->stand_alone_type, left_alloca)});
        return {returnValue, nullptr, func_record->return_type, resolveType::StructInst};
    } else {
        errors::raiseWrongInfixError(this->file_path, this->source, left, right, op, "Cant " + op + " 2 structs", "Add the `" + op_method + "` method in structs in either one of the struct");
    }
};

bool Compiler::conversionPrecidence(RecordStructType* from, RecordStructType* to) {
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

Compiler::ResolvedValue Compiler::convertType(const ResolvedValue& from, RecordStructType* to) {
    auto [fromloadedval, fromalloca, _fromtype, struct_type] = from;
    auto fromtype = std::get<RecordStructType*>(_fromtype);
    // If Types are Same
    if (_checkType(fromtype, to)) { return {fromloadedval, fromalloca, fromtype, resolveType::StructInst}; }

    if (fromtype->name == "int32" && to->name == "int") {
        auto int_type = this->llvm_ir_builder.CreateSExt(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                         this->env->getStruct("int")->stand_alone_type,
                                                         "int32_to_int");
        return {int_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "int" && to->name == "int32") {
        auto int32_type = this->llvm_ir_builder.CreateTrunc(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                            this->env->getStruct("int32")->stand_alone_type,
                                                            "int_to_int32");
        return {int32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "uint32" && to->name == "uint") {
        auto uint_type = this->llvm_ir_builder.CreateZExt(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                          this->env->getStruct("uint")->stand_alone_type,
                                                          "uint32_to_uint");
        return {uint_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "uint" && to->name == "uint32") {
        auto uint32_type = this->llvm_ir_builder.CreateTrunc(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                             this->env->getStruct("uint32")->stand_alone_type,
                                                             "uint_to_uint32");
        return {uint32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "float32" && to->name == "float") {
        auto float_type = this->llvm_ir_builder.CreateFPExt(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                            this->env->getStruct("float")->stand_alone_type,
                                                            "float32_to_float");
        return {float_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "float" && to->name == "float32") {
        auto float32_type = this->llvm_ir_builder.CreateFPTrunc(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                                this->env->getStruct("float32")->stand_alone_type,
                                                                "float_to_float32");
        return {float32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "int" && to->name == "float") {
        auto float_type = this->llvm_ir_builder.CreateSIToFP(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                             this->env->getStruct("float")->stand_alone_type,
                                                             "int_to_float");
        return {float_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "int32" && to->name == "float32") {
        auto float32_type = this->llvm_ir_builder.CreateSIToFP(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                               this->env->getStruct("float32")->stand_alone_type,
                                                               "int32_to_float32");
        return {float32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "uint" && to->name == "float") {
        auto float_type = this->llvm_ir_builder.CreateUIToFP(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                             this->env->getStruct("float")->stand_alone_type,
                                                             "uint_to_float");
        return {float_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "uint32" && to->name == "float32") {
        auto float32_type = this->llvm_ir_builder.CreateUIToFP(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                               this->env->getStruct("float32")->stand_alone_type,
                                                               "uint32_to_float32");
        return {float32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "float" && to->name == "int") {
        auto int_type = this->llvm_ir_builder.CreateFPToSI(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                           this->env->getStruct("int")->stand_alone_type,
                                                           "float_to_int");
        return {int_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "float32" && to->name == "int32") {
        auto int32_type = this->llvm_ir_builder.CreateFPToSI(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                             this->env->getStruct("int32")->stand_alone_type,
                                                             "float32_to_int32");
        return {int32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "float" && to->name == "uint") {
        auto uint_type = this->llvm_ir_builder.CreateFPToUI(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                            this->env->getStruct("uint")->stand_alone_type,
                                                            "float_to_uint");
        return {uint_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "float32" && to->name == "uint32") {
        auto uint32_type = this->llvm_ir_builder.CreateFPToUI(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                              this->env->getStruct("uint32")->stand_alone_type,
                                                              "float32_to_uint32");
        return {uint32_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "bool" && to->name == "int") {
        auto int_type = this->llvm_ir_builder.CreateZExt(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                         this->env->getStruct("int")->stand_alone_type,
                                                         "bool_to_int");
        return {int_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "bool" && to->name == "uint") {
        auto uint_type = this->llvm_ir_builder.CreateZExt(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                          this->env->getStruct("uint")->stand_alone_type,
                                                          "bool_to_uint");
        return {uint_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "bool" && to->name == "float") {
        auto float_type = this->llvm_ir_builder.CreateUIToFP(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                             this->env->getStruct("float")->stand_alone_type,
                                                             "bool_to_float");
        return {float_type, fromalloca, to, resolveType::StructInst};
    } else if (fromtype->name == "bool" && to->name == "float32") {
        auto float32_type = this->llvm_ir_builder.CreateUIToFP(fromloadedval ? fromloadedval : this->llvm_ir_builder.CreateLoad(fromtype->stand_alone_type, fromalloca),
                                                               this->env->getStruct("float32")->stand_alone_type,
                                                               "bool_to_float32");
        return {float32_type, fromalloca, to, resolveType::StructInst};
    } else if ((fromtype->name == "int" || fromtype->name == "float" || fromtype->name == "str") && to->name == "bool") {
        errors::raiseCompletionError(this->file_path,
                                     this->source,
                                     fromtype->meta_data.st_line_no,
                                     fromtype->meta_data.st_col_no,
                                     fromtype->meta_data.end_line_no,
                                     fromtype->meta_data.end_col_no,
                                     "Conversion from int, float, or str to bool is not yet implemented",
                                     "Fix the conversion logic for int, float, or str to bool.");
    } else if (fromtype->struct_type) { // If struct & it is mark as auto cast
        if (fromtype->is_method("", {fromtype}, AST::MoreData(std::unordered_map<std::string, bool>{{"autocast", true}}), to)) {
            auto method = fromtype->get_method("", {fromtype}, AST::MoreData(std::unordered_map<std::string, bool>{{"autocast", true}}), to);
            auto returnValue = this->llvm_ir_builder.CreateCall(method->function, {fromalloca}, fromtype->name + "_to_" + to->name);
            return {returnValue, nullptr, method->return_type, resolveType::StructInst};
        }
    }

    RecordModule* x = nullptr;
    return {nullptr, nullptr, x, resolveType::Module};
};

bool Compiler::canConvertType(RecordStructType* from, RecordStructType* to) {
    const vector<std::pair<Str, Str>> convertibleTypes = {{"int32", "int"},  {"int", "int32"},      {"uint32", "uint"}, {"uint", "uint32"},    {"float32", "float"}, {"float", "float32"},
                                                          {"int", "float"},  {"int32", "float32"},  {"uint", "float"},  {"uint32", "float32"}, {"float", "int"},     {"float32", "int32"},
                                                          {"float", "uint"}, {"float32", "uint32"}, {"bool", "int"},    {"bool", "uint"},      {"bool", "float"},    {"bool", "float32"},
                                                          {"int", "bool"},   {"float", "bool"},     {"str", "bool"}};

    for (const auto& [fromType, toType] : convertibleTypes) {
        if (from->name == fromType && to->name == toType) { return true; }
    }
    if (from->struct_type && from->is_method("", {from}, std::unordered_map<std::string, bool>{{"autocast", true}}, to, true)) { return true; }
    return false;
};

Compiler::ResolvedValue Compiler::_visitInfixExpression(AST::InfixExpression* infixed_expression) {
    auto op = infixed_expression->op;
    auto left = infixed_expression->left;
    auto right = infixed_expression->right;
    auto [left_value, left_alloca, _left_type, ltt] = this->_resolveValue(left);
    if (op == token::TokenType::Dot) { return this->_memberAccess(infixed_expression); }
    auto [right_value, right_alloca, _right_type, rtt] = this->_resolveValue(right);
    if (ltt != resolveType::StructInst && !(ltt == resolveType::StructType && std::get<RecordStructType*>(_left_type)->name == "nullptr") ||
        (rtt != resolveType::StructInst && !(rtt == resolveType::StructType && std::get<RecordStructType*>(_right_type)->name == "nullptr"))) {
        errors::raiseWrongInfixError(this->file_path, this->source, left, right, token::tokenTypeString(op), "Cant " + token::tokenTypeString(op) + " 2 types or modules");
    }
    // Handle type conversion
    auto left_val = left_value;
    auto right_val = right_value;
    auto left_type = std::get<RecordStructType*>(_left_type);
    auto right_type = std::get<RecordStructType*>(_right_type);
    if (!_checkType(left_type, right_type) && (this->canConvertType(left_type, right_type) || this->canConvertType(right_type, left_type)) && left_type->stand_alone_type &&
        right_type->stand_alone_type) {
        if (this->conversionPrecidence(left_type, right_type)) {
            auto x = this->convertType({left_val, left_alloca, left_type}, right_type);
            right_val = x.value;
            right_alloca = x.alloca;
            right_type = std::get<RecordStructType*>(x.variant);
        } else {
            auto x = this->convertType({right_val, right_alloca, right_type}, left_type);
            left_val = x.value;
            left_alloca = x.alloca;
            left_type = std::get<RecordStructType*>(x.variant);
        }
    }
    vector<RecordStructType*> params_type1{left_type, right_type};
    vector<RecordStructType*> params_type2{right_type, left_type};
    if (left_type->struct_type != nullptr || right_type->struct_type != nullptr) {
        switch (op) {
            case token::TokenType::Plus: {
                return this->_StructInfixCall("__add__", "add", left_type, right_type, left, right, left_value, left_alloca, right_value, right_alloca);
            }
            case token::TokenType::Dash: {
                return this->_StructInfixCall("__sub__", "substract", left_type, right_type, left, right, left_value, left_alloca, right_value, right_alloca);
            }
            case token::TokenType::Asterisk: {
                return this->_StructInfixCall("__mul__", "multiply", left_type, right_type, left, right, left_value, left_alloca, right_value, right_alloca);
            }
            case token::TokenType::ForwardSlash: {
                return this->_StructInfixCall("__div__", "divide", left_type, right_type, left, right, left_value, left_alloca, right_value, right_alloca);
            }
            case token::TokenType::Percent: {
                return this->_StructInfixCall("__mod__", "%", left_type, right_type, left, right, left_value, left_alloca, right_value, right_alloca);
            }
            case token::TokenType::EqualEqual: {
                if (left_type->name == "nullptr" && right_type->name == "nullptr") {
                    auto inst = llvm::ConstantInt::getTrue(this->llvm_context);
                    return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
                } else if (left_type->name == "nullptr") {
                    auto inst = this->llvm_ir_builder.CreateICmpEQ(right_value ? right_value : this->llvm_ir_builder.CreateLoad(this->ll_pointer, right_alloca),
                                                                   llvm::Constant::getNullValue(this->ll_pointer));
                    return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
                } else if (right_type->name == "nullptr") {
                    auto inst =
                        this->llvm_ir_builder.CreateICmpEQ(left_value ? left_value : this->llvm_ir_builder.CreateLoad(this->ll_pointer, left_alloca), llvm::Constant::getNullValue(this->ll_pointer));
                    return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
                }
                return this->_StructInfixCall("__eq__", "compare equals", left_type, right_type, left, right, left_value, left_alloca, right_value, right_alloca);
            }
            case token::TokenType::NotEquals: {
                if (left_type->name == "nullptr" && right_type->name == "nullptr") {
                    auto inst = llvm::ConstantInt::getFalse(this->llvm_context);
                    return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
                } else if (left_type->name == "nullptr") {
                    auto inst = this->llvm_ir_builder.CreateICmpNE(right_value ? right_value : this->llvm_ir_builder.CreateLoad(this->ll_pointer, right_alloca),
                                                                   llvm::Constant::getNullValue(this->ll_pointer));
                    return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
                } else if (right_type->name == "nullptr") {
                    auto inst =
                        this->llvm_ir_builder.CreateICmpNE(left_value ? left_value : this->llvm_ir_builder.CreateLoad(this->ll_pointer, left_alloca), llvm::Constant::getNullValue(this->ll_pointer));
                    return {inst, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
                }
                return this->_StructInfixCall("__neq__", "compare not equals", left_type, right_type, left, right, left_value, left_alloca, right_value, right_alloca);
            }
            case token::TokenType::LessThan: {
                return this->_StructInfixCall("__lt__", "compare less than", left_type, right_type, left, right, left_value, left_alloca, right_value, right_alloca);
            }
            case token::TokenType::GreaterThan: {
                return this->_StructInfixCall("__gt__", "compare greater than", left_type, right_type, left, right, left_value, left_alloca, right_value, right_alloca);
            }
            case token::TokenType::LessThanOrEqual: {
                return this->_StructInfixCall("__lte__", "compare less than equals", left_type, right_type, left, right, left_value, left_alloca, right_value, right_alloca);
            }
            case token::TokenType::GreaterThanOrEqual: {
                return this->_StructInfixCall("__gte__", "compare greater than equals", left_type, right_type, left, right, left_value, left_alloca, right_value, right_alloca);
            }
            case token::TokenType::AsteriskAsterisk: {
                return this->_StructInfixCall("__pow__", "**", left_type, right_type, left, right, left_value, left_alloca, right_value, right_alloca);
            }
            default: {
                errors::raiseWrongInfixError(this->file_path, this->source, left, right, token::tokenTypeString(op), "Cant operator: `" + token::tokenTypeString(op) + "` 2 structs", "");
            }
        }
    }
    llvm::Value* left_val_converted = left_val ? left_val : this->llvm_ir_builder.CreateLoad(left_type->stand_alone_type, left_alloca);
    llvm::Value* right_val_converted = right_val ? right_val : this->llvm_ir_builder.CreateLoad(right_type->stand_alone_type, right_alloca);
    auto common_type = left_type;
    if (left_type != right_type) {
        if (this->conversionPrecidence(left_type, right_type)) {
            auto lct = this->convertType({left_val, left_alloca, left_type}, right_type);
            left_val_converted = lct.value ? lct.value : this->llvm_ir_builder.CreateLoad(right_type->stand_alone_type, lct.alloca);
            common_type = right_type;
        } else if (this->conversionPrecidence(right_type, left_type)) {
            auto rct = this->convertType({right_val, right_alloca, right_type}, left_type);
            right_val_converted = rct.value ? rct.value : this->llvm_ir_builder.CreateLoad(left_type->stand_alone_type, rct.alloca);
        } else {
            auto rct = this->convertType({right_val, right_alloca, right_type}, left_type);
            right_val_converted = rct.value ? rct.value : this->llvm_ir_builder.CreateLoad(left_type->stand_alone_type, rct.alloca);
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
                errors::raiseWrongInfixError(this->file_path, this->source, left, right, token::tokenTypeString(op), "Power operator not supported for int");
            }
            default: {
                errors::raiseWrongInfixError(this->file_path, this->source, left, right, token::tokenTypeString(op), "Unknown operator: " + token::tokenTypeString(op));
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
                errors::raiseWrongInfixError(this->file_path, this->source, left, right, token::tokenTypeString(op), "Power operator not supported for float");
            }
            default: {
                errors::raiseWrongInfixError(this->file_path, this->source, left, right, token::tokenTypeString(op), "Unknown operator: " + token::tokenTypeString(op));
            }
        }
    } else {
        errors::raiseWrongTypeError(this->file_path,
                                    this->source,
                                    infixed_expression,
                                    common_type,
                                    {this->env->getStruct("int"), this->env->getStruct("float")},
                                    "Unknown Type",
                                    "Check the types of the operands.");
    }
};

Compiler::ResolvedValue Compiler::_resolveAndValidateLeftOperand(AST::IndexExpression* index_expression) {
    auto [left, left_alloca, _left_generic, ltt] = this->_resolveValue(index_expression->left);

    if (ltt != resolveType::StructInst) {
        errors::raiseCantIndexError(this->file_path, this->source, index_expression, false, "Cannot index Module or type", "Ensure the left-hand side is an raw_array or a valid indexable type.");
    }

    auto left_generic = std::get<RecordStructType*>(_left_generic);
    return {left, left_alloca, left_generic, ltt};
}

Compiler::ResolvedValue Compiler::_resolveAndValidateIndexOperand(AST::IndexExpression* index_expression) {
    auto [index, __, _index_generic, itt] = this->_resolveValue(index_expression->index);

    if (itt != resolveType::StructInst) {
        errors::raiseCantIndexError(this->file_path, this->source, index_expression, true, "Index must be an integer, not a Module or type", "Ensure the index is an integer.");
    }

    auto index_generic = std::get<RecordStructType*>(_index_generic);
    return {index, __, index_generic, itt};
}

Compiler::ResolvedValue Compiler::_handleraw_arrayIndexing(llvm::Value* left, RecordStructType* left_generic, llvm::Value* index, RecordStructType* index_generic, AST::IndexExpression* index_expression) {
    // Validate that the index is of type 'int'
    if (!_checkType(index_generic, this->env->getStruct("int"))) {
        errors::raiseCantIndexError(this->file_path, this->source, index_expression, true, "Index must be an integer not `" + index_generic->name + "`", "Ensure the index is an integer.");
    }

    // Get the element type
    auto element_type = left_generic->generic_sub_types[0];
    auto element_ptr_type = (element_type->struct_type || element_type->name == "raw_array") ? this->ll_pointer : element_type->stand_alone_type;

    // Calculate the element pointer
    auto element = this->llvm_ir_builder.CreateGEP(element_ptr_type, left, index, "element");
    // Load the element value
    llvm::Value* load = (element_type->struct_type || element_type->name == "raw_array") ? element : this->llvm_ir_builder.CreateLoad(element_type->stand_alone_type, element);

    return {load, element, element_type, resolveType::StructInst};
}

[[noreturn]] void Compiler::_raiseNoIndexMethodError(RecordStructType* left_generic, AST::IndexExpression* index_expression) {
    errors::raiseNoOverloadError(this->file_path, this->source, {}, index_expression, "__index__ method does not exist for struct " + left_generic->name + ".", "Define the __index__ method.");
}

Compiler::ResolvedValue Compiler::_visitIndexExpression(AST::IndexExpression* index_expression) {
    // Resolve and validate the left operand
    auto [left, left_alloca, _left_generic, ltt] = this->_resolveAndValidateLeftOperand(index_expression);
    auto left_generic = std::get<RecordStructType*>(_left_generic);
    // Resolve and validate the index operand
    auto [index, __, _index_generic, itt] = this->_resolveAndValidateIndexOperand(index_expression);
    auto index_generic = std::get<RecordStructType*>(_index_generic);

    // Handle raw_array indexing if applicable
    if (_checkType(left_generic, this->env->getStruct("raw_array"))) { return _handleraw_arrayIndexing(left, left_generic, index, index_generic, index_expression); }

    // Handle struct indexing via __index__ method
    return _handleStructIndexing(left_alloca, index, index_generic, left_generic, index_expression);
}


void Compiler::_visitVariableDeclarationStatement(AST::VariableDeclarationStatement* variable_declaration_statement) {
    auto var_name = variable_declaration_statement->name->castToIdentifierLiteral();

    // Check if the variable is already declared
    if (this->env->isVariable(var_name->value)) { errors::raiseDuplicateVariableError(this->file_path, this->source, var_name->value, variable_declaration_statement, "Variable is already declared"); }

    auto var_value = variable_declaration_statement->value;
    RecordStructType* var_type = variable_declaration_statement->value_type ? this->_parseType(variable_declaration_statement->value_type) : nullptr;

    if (var_value == nullptr) {
        llvm::Value* alloca = this->llvm_ir_builder.CreateAlloca(var_type->struct_type || var_type->name == "raw_array" ? this->ll_pointer : var_type->stand_alone_type);
        auto var = new RecordVariable(var_name->value, nullptr, alloca, var_type);
        this->env->addRecord(var);
        return;
    }

    auto [var_value_resolved, var_value_alloca, _var_generic, vartt] = this->_resolveValue(var_value);
    auto var_generic = std::get<RecordStructType*>(_var_generic);

    if (vartt != resolveType::StructInst && !(vartt == resolveType::StructType && var_generic->name == "nullptr")) {
        errors::raiseWrongTypeError(this->file_path, this->source, var_value, nullptr, {var_type}, "Cannot assign module or type to variable");
    }

    if (variable_declaration_statement->value_type && !_checkType(var_generic, var_type)) {
        if (this->canConvertType(var_generic, var_type)) {
            auto converted = this->convertType({var_value_resolved, var_value_alloca, var_generic}, var_type);
            var_value_resolved = converted.value;
            var_value_alloca = converted.alloca;
            var_generic = std::get<RecordStructType*>(converted.variant);
        } else {
            errors::raiseWrongTypeError(this->file_path, this->source, var_value, var_generic, {var_type}, "Cannot assign mismatched type");
        }
    }
    llvm::Value* alloca = this->llvm_ir_builder.CreateAlloca(var_generic->struct_type || var_generic->name == "raw_array" ? this->ll_pointer : var_generic->stand_alone_type);
    if (var_generic->struct_type || var_generic->name == "raw_array") {
        this->llvm_ir_builder.CreateStore(var_value_resolved ? var_value_resolved : this->llvm_ir_builder.CreateLoad(this->ll_pointer, var_value_alloca),
                                          alloca,
                                          variable_declaration_statement->is_volatile);
        auto var = new RecordVariable(var_name->value, nullptr, alloca, var_generic);
        this->env->addRecord(var);
    } else {
        this->llvm_ir_builder.CreateStore(var_value_resolved ? var_value_resolved : this->llvm_ir_builder.CreateLoad(var_generic->stand_alone_type, var_value_alloca),
                                          alloca,
                                          variable_declaration_statement->is_volatile);
        auto var = new RecordVariable(var_name->value, nullptr, alloca, var_generic);
        this->env->addRecord(var);
    }
}

void Compiler::_visitVariableAssignmentStatement(AST::VariableAssignmentStatement* variable_assignment_statement) {
    auto var_value = variable_assignment_statement->value;
    auto [value, value_alloca, _assignmentType, vtt] = this->_resolveValue(var_value);
    auto assignmentType = std::get<RecordStructType*>(_assignmentType);

    if (vtt != resolveType::StructInst && !(vtt == resolveType::StructType && assignmentType->name == "nullptr")) {
        errors::raiseWrongTypeError(this->file_path, this->source, var_value, nullptr, {assignmentType}, "Cannot assign module or type to variable");
    }

    auto [_, alloca, _var_type, att] = this->_resolveValue(variable_assignment_statement->name);
    auto var_type = std::get<RecordStructType*>(_var_type);

    if (!_checkType(var_type, assignmentType)) {
        if (this->canConvertType(assignmentType, var_type)) {
            auto converted = this->convertType({value, value_alloca, assignmentType}, var_type);
            value = converted.value;
            value_alloca = converted.alloca;
            assignmentType = std::get<RecordStructType*>(converted.variant);
        } else {
            errors::raiseWrongTypeError(this->file_path, this->source, var_value, var_type, {var_type}, "Cannot assign mismatched type");
        }
    }

    if (assignmentType->struct_type) {
        if (assignmentType->name == "nullptr") {
            this->llvm_ir_builder.CreateStore(llvm::Constant::getNullValue(this->ll_pointer), alloca);
        } else {
            this->llvm_ir_builder.CreateStore(value ? value : this->llvm_ir_builder.CreateLoad(this->ll_pointer, value_alloca), alloca);
        }
    } else {
        this->llvm_ir_builder.CreateStore(value ? value : this->llvm_ir_builder.CreateLoad(var_type->stand_alone_type, value_alloca), alloca);
    }
}

void Compiler::_visitSwitchCaseStatement(AST::SwitchCaseStatement* switch_statement) {
    // Create a new basic block for the switch statement
    llvm::Function* function = llvm_ir_builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* default_block = llvm::BasicBlock::Create(llvm_context, "default", function);
    llvm::BasicBlock* end_block = llvm::BasicBlock::Create(llvm_context, "end_switch", function);

    // Resolve the condition value
    ResolvedValue condition_value = _resolveValue(switch_statement->condition);

    // Create the switch instruction
    llvm::SwitchInst* switch_inst = llvm_ir_builder.CreateSwitch(condition_value.value, default_block, switch_statement->cases.size());
    llvm::BasicBlock* garbage_block = llvm::BasicBlock::Create(llvm_context, "garbage", function);
    llvm_ir_builder.SetInsertPoint(garbage_block);
    // Create basic blocks for each case
    std::vector<llvm::BasicBlock*> case_blocks;
    for (size_t i = 0; i < switch_statement->cases.size(); ++i) {
        llvm::BasicBlock* case_block = llvm::BasicBlock::Create(llvm_context, "case", function);
        case_blocks.push_back(case_block);
    }

    // Populate the switch instruction with cases
    for (size_t i = 0; i < switch_statement->cases.size(); ++i) {
        auto [case_expr, case_stmt] = switch_statement->cases[i];
        ResolvedValue case_value = _resolveValue(case_expr);
        switch_inst->addCase(llvm::cast<llvm::ConstantInt>(case_value.value), case_blocks[i]);
    }

    llvm_ir_builder.CreateUnreachable();
    // Visit each case block
    for (size_t i = 0; i < switch_statement->cases.size(); ++i) {
        llvm_ir_builder.SetInsertPoint(case_blocks[i]);
        auto [case_expr, case_stmt] = switch_statement->cases[i];
        _visitBlockStatement(case_stmt->castToBlockStatement());
        if (!llvm_ir_builder.GetInsertBlock()->getTerminator()) { llvm_ir_builder.CreateBr(end_block); }
    }

    // Visit the default block
    llvm_ir_builder.SetInsertPoint(default_block);
    if (switch_statement->other) { _visitBlockStatement(switch_statement->other->castToBlockStatement()); }
    if (!llvm_ir_builder.GetInsertBlock()->getTerminator()) { llvm_ir_builder.CreateBr(end_block); }

    // Set the insert point to the end block
    llvm_ir_builder.SetInsertPoint(end_block);
}

Compiler::ResolvedValue Compiler::_resolveIntegerLiteral(AST::IntegerLiteral* integer_literal) {
    auto value = llvm::ConstantInt::get(this->llvm_context, llvm::APInt(64, integer_literal->value));
    return {value, nullptr, this->env->getStruct("int"), resolveType::StructInst};
}

Compiler::ResolvedValue Compiler::_resolveFloatLiteral(AST::FloatLiteral* float_literal) {
    auto value = llvm::ConstantFP::get(this->llvm_context, llvm::APFloat(float_literal->value));
    return {value, nullptr, this->env->getStruct("float"), resolveType::StructInst};
}

Compiler::ResolvedValue Compiler::_resolveStringLiteral(AST::StringLiteral* string_literal) {
    auto value = this->llvm_ir_builder.CreateGlobalStringPtr(string_literal->value);
    return {value, nullptr, this->env->getStruct("str"), resolveType::StructInst};
}

Compiler::ResolvedValue Compiler::_resolveIdentifierLiteral(AST::IdentifierLiteral* identifier_literal) {
    if (identifier_literal->value == "nullptr") {
        return {llvm::Constant::getNullValue(this->ll_pointer), llvm::Constant::getNullValue(this->ll_pointer), this->env->getStruct("nullptr"), resolveType::StructType};
    }
    if (this->env->isVariable(identifier_literal->value)) {
        auto variable = this->env->getVariable(identifier_literal->value);
        auto currentStructType = variable->variable_type;
        currentStructType->meta_data = identifier_literal->meta_data;
        if (currentStructType->struct_type || currentStructType->name == "raw_array") {
            auto loadInst = this->llvm_ir_builder.CreateLoad(this->ll_pointer, variable->allocainst);
            return {loadInst, variable->allocainst, currentStructType, resolveType::StructInst};
        } else {
            auto loadInst = this->llvm_ir_builder.CreateLoad(currentStructType->stand_alone_type, variable->allocainst);
            return {loadInst, variable->allocainst, currentStructType, resolveType::StructInst};
        }
    } else if (this->env->isModule(identifier_literal->value)) {
        return {nullptr, nullptr, this->env->getModule(identifier_literal->value), resolveType::Module};
    } else if (this->env->isStruct(identifier_literal->value)) {
        return {nullptr, nullptr, this->env->getStruct(identifier_literal->value), resolveType::StructType};
    } else if (this->env->isGenericStruct(identifier_literal->value)) {
        return {nullptr, nullptr, this->env->getGenericStruct(identifier_literal->value), resolveType::GStructType};
    }
    errors::raiseNotDefinedError(this->file_path, this->source, identifier_literal, "Variable or function or struct `" + identifier_literal->value + "` not defined", "Recheck the Name");
}

Compiler::ResolvedValue Compiler::_resolveInfixExpression(AST::InfixExpression* infix_expression) {
    return this->_visitInfixExpression(infix_expression);
}

Compiler::ResolvedValue Compiler::_resolveIndexExpression(AST::IndexExpression* index_expression) {
    return this->_visitIndexExpression(index_expression);
}

Compiler::ResolvedValue Compiler::_resolveCallExpression(AST::CallExpression* call_expression) {
    return this->_visitCallExpression(call_expression);
}

Compiler::ResolvedValue Compiler::_resolveBooleanLiteral(AST::BooleanLiteral* boolean_literal) {
    auto value = boolean_literal->value ? llvm::ConstantInt::getTrue(this->llvm_context) : llvm::ConstantInt::getFalse(this->llvm_context);
    auto alloca = this->llvm_ir_builder.CreateAlloca(this->env->getStruct("bool")->stand_alone_type);
    this->llvm_ir_builder.CreateStore(value, alloca);
    return {value, nullptr, this->env->getStruct("bool"), resolveType::StructInst};
}

Compiler::ResolvedValue Compiler::_resolveArrayLiteral(AST::ArrayLiteral* raw_array_literal) {
    return this->_visitArrayLiteral(raw_array_literal);
}

Compiler::ResolvedValue Compiler::_resolveValue(AST::Node* node) {
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
            errors::raiseUnknownNodeTypeError(this->file_path,
                                              this->source,
                                              AST::nodeTypeToString(node->type()),
                                              node->meta_data.st_line_no,
                                              node->meta_data.st_col_no,
                                              node->meta_data.end_line_no,
                                              node->meta_data.end_col_no,
                                              "Unknown node type: " + AST::nodeTypeToString(node->type()));
    }
}

Compiler::ResolvedValue Compiler::_visitArrayLiteral(AST::ArrayLiteral* raw_array_literal) {
    std::vector<llvm::Value*> values;
    RecordStructType* struct_type = nullptr;
    RecordStructType* first_generic = nullptr;

    for (const auto& element : raw_array_literal->elements) {
        // Validate the raw_array element's type
        this->_validateraw_arrayElement(element, first_generic);

        // Resolve the value of the element
        auto [value, value_alloca, _generic, vtt] = this->_resolveValue(element);
        RecordStructType* generic = std::get<RecordStructType*>(_generic);

        // Initialize struct_type and first_generic if not set
        if (!struct_type) {
            struct_type = generic;
            first_generic = generic;
        }
        // Handle type conversion if necessary
        ResolvedValue resolved_value = {value, value_alloca, _generic, vtt};
        this->_handleTypeConversion(element, resolved_value, first_generic);
        generic = std::get<RecordStructType*>(resolved_value.variant);

        // Collect the validated and possibly converted value
        values.push_back(resolved_value.value);
    }

    // Create the LLVM raw_array type
    auto raw_array_type = llvm::ArrayType::get(struct_type->struct_type ? struct_type->struct_type : struct_type->stand_alone_type, values.size());
    llvm::Value* raw_array;

    // Allocate memory for the raw_array
    if (raw_array_literal->_new) {
        llvm::Value* gep = this->llvm_ir_builder.CreateGEP(raw_array_type, llvm::ConstantPointerNull::get(this->ll_pointer), llConstInt::get(this->ll_int, 1));
        llvm::Value* size = this->llvm_ir_builder.CreatePtrToInt(gep, this->ll_int);
        raw_array = this->llvm_ir_builder.CreateCall(this->env->getFunction("malloc", {this->env->getStruct("int")})->function, {size});
    } else {
        raw_array = this->llvm_ir_builder.CreateAlloca(raw_array_type);
    }

    // Store each element in the LLVM raw_array
    for (size_t i = 0; i < values.size(); ++i) {
        auto element_ptr = this->llvm_ir_builder.CreateGEP(raw_array_type, raw_array, {this->llvm_ir_builder.getInt64(0), this->llvm_ir_builder.getInt64(i)});
        // if (first_generic->struct_type || first_generic->name == "raw_array") { this->_incrementRC(values[i]); }
        this->llvm_ir_builder.CreateStore(values[i], element_ptr);
    }

    // Create the raw_array struct and manage reference counting
    auto raw_array_struct = new RecordStructType(*this->env->getStruct("raw_array"));
    raw_array_struct->generic_sub_types.push_back(first_generic);

    return {raw_array, raw_array, raw_array_struct, resolveType::StructInst};
}

// Implementation of _validateraw_arrayElement
void Compiler::_validateraw_arrayElement(AST::Node* element, RecordStructType*& first_generic) {
    auto [value, value_alloca, _generic, vtt] = this->_resolveValue(element);
    if (vtt != resolveType::StructInst) { errors::raiseArrayTypeError(this->file_path, this->source, element, first_generic, "Cannot add Module or type in raw_array"); }
}

// Implementation of _handleTypeConversion
void Compiler::_handleTypeConversion(AST::Expression* element, ResolvedValue& resolved_value, RecordStructType*& first_generic) {
    RecordStructType* generic = std::get<RecordStructType*>(resolved_value.variant);

    if (!_checkType(first_generic, generic)) {
        if (this->canConvertType(generic, first_generic)) {
            ResolvedValue converted = this->convertType({resolved_value.value, resolved_value.alloca, generic}, first_generic);
            resolved_value.value = converted.value;
            resolved_value.alloca = converted.alloca;
            resolved_value.variant = converted.variant;
        } else {
            errors::raiseArrayTypeError(this->file_path, this->source, element, first_generic, "raw_array with multiple types or generics");
        }
    }
}

void Compiler::_handleValueReturnStatement(AST::ReturnStatement* return_statement) {
    auto value = return_statement->value;
    auto [return_value, return_alloca, _return_type, _] = _resolveAndValidateReturnValue(value);
    auto return_type = std::get<RecordStructType*>(_return_type);
    if (this->env->current_function == nullptr) {
        errors::raiseNodeOutsideError("Return outside function", this->source, return_statement, errors::OutsideNodeType::Return, "Return statement outside of a function");
    }

    _checkAndConvertReturnType(value, return_value, return_alloca, return_type);

    _createReturnInstruction(return_value, return_alloca, return_type);

    throw DoneRet(); // Indicates to stop parsing further statements in the current block
}

Compiler::ResolvedValue Compiler::_resolveAndValidateReturnValue(AST::Expression* value) {
    auto resolved_value = this->_resolveValue(value);

    if (resolved_value.type != resolveType::StructInst) {
        if (resolved_value.type == resolveType::StructType && std::get<RecordStructType*>(resolved_value.variant)->name == "void") {
            this->llvm_ir_builder.CreateRetVoid();
            throw DoneRet();
        } else {
            errors::raiseWrongTypeError(this->file_path,
                                        this->source,
                                        value,
                                        std::get<RecordStructType*>(resolved_value.variant),
                                        {this->env->current_function->return_type},
                                        "Cannot return module or type from function");
        }
    }

    RecordStructType* return_type = std::get<RecordStructType*>(resolved_value.variant);
    return {resolved_value.value, resolved_value.alloca, return_type, resolved_value.type};
}

void Compiler::_checkAndConvertReturnType(AST::Expression* value, llvm::Value*& return_value, llvm::Value*& return_alloca, RecordStructType*& return_type) {
    if (!_checkType(this->env->current_function->return_type, return_type)) {
        if (this->canConvertType(return_type, this->env->current_function->return_type)) {
            auto [converted_value, converted_alloca, converted_type, _] = this->convertType({return_value, return_alloca, return_type}, this->env->current_function->return_type);
            return_value = converted_value;
            return_alloca = converted_alloca;
            if (!return_value) {
                if (!return_value && !return_alloca) {
                    errors::raiseWrongTypeError(this->file_path,
                                                this->source,
                                                value,
                                                return_type,
                                                {this->env->current_function->return_type},
                                                "Return Type mismatch & also no viable conversion avalable");
                }
            }
            return_type = std::get<RecordStructType*>(converted_type);
        } else {
            errors::raiseWrongTypeError(this->file_path, this->source, value, return_type, {this->env->current_function->return_type}, "Return Type mismatch & also no viable conversion avalable");
        }
    }
}


void Compiler::_createReturnInstruction(llvm::Value* return_value, llvm::Value* return_alloca, RecordStructType* return_type) {
    auto function_return_type = this->env->current_function->function->getReturnType();
    if (return_type->struct_type || return_type->name == "raw_array") {
        this->llvm_ir_builder.CreateRet(return_value ? return_value : this->llvm_ir_builder.CreateLoad(this->ll_pointer, return_alloca));
    } else {
        this->llvm_ir_builder.CreateRet(return_value ? return_value : this->llvm_ir_builder.CreateLoad(return_type->stand_alone_type, return_alloca));
    }
}

void Compiler::_visitReturnStatement(AST::ReturnStatement* return_statement) {
    if (!return_statement->value && this->env->current_function->return_type->name == "void") {
        this->llvm_ir_builder.CreateRetVoid();
        throw DoneRet(); // Indicates to stop parsing further statements in the current block
    } else {
        _handleValueReturnStatement(return_statement);
    }
}

RecordStructType* Compiler::_parseType(AST::Type* type) {
    vector<RecordStructType*> generics;
    for (auto gen : type->generics) { generics.push_back(this->_parseType(gen)); }

    auto [_, __, _struct, stt] = this->_resolveValue(type->name);
    if (stt != resolveType::StructType) {
        if (stt == resolveType::GStructType) {
            auto gstructs = std::get<vector<RecordGenericStructType*>>(_struct);
            auto prev_env = this->env;

            for (auto gstruct : gstructs) {
                if (generics.size() != gstruct->structAST->generics.size()) { continue; }

                this->env = new Enviornment(gstruct->env);
                gstruct->env->childes.push_back(this->env);
                Str struct_name = gstruct->structAST->name->castToIdentifierLiteral()->value;
                auto struct_record = new RecordStructType(struct_name);

                if (!gstruct->env->isStruct(struct_name, false, generics)) {
                    for (auto [generic, rg] : llvm::zip(generics, gstruct->structAST->generics)) {
                        auto generic_copy = new RecordStructType(*generic);
                        generic_copy->name = rg->name->castToIdentifierLiteral()->value;
                        this->env->addRecord(generic_copy);
                    }

                    vector<llvm::Type*> field_types;
                    auto fields = gstruct->structAST->fields;
                    this->env->addRecord(struct_record); // Dont Copy struct type here because it's get modify down the road

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
                    delete struct_record;
                    struct_record = gstruct->env->getStruct(struct_name, false, generics);
                }

                gstruct->env->addRecord(new RecordStructType(*struct_record));
                this->env = prev_env;
                return struct_record;
            }

            errors::raiseNoOverloadError(this->file_path, this->source, {}, type->name, "No GStruct Viable");
        }

        errors::raiseWrongTypeError(this->file_path, this->source, type->name, nullptr, {}, "module is not a Type");
    }

    auto struct_ = std::get<RecordStructType*>(_struct);
    if (struct_->name == "raw_array") {
        struct_ = new RecordStructType(*struct_);
        struct_->generic_sub_types.push_back(generics[0]);
    }
    return struct_;
}

void Compiler::_visitIfElseStatement(AST::IfElseStatement* if_statement) {
    // Resolve the condition value
    auto resolved_cond = this->_resolveValue(if_statement->condition);

    // Ensure the condition is of type StructInst and can be converted to bool if necessary
    if (resolved_cond.type != resolveType::StructInst) {
        errors::raiseWrongTypeError(this->file_path, this->source, if_statement->condition, nullptr, {this->env->getStruct("bool")}, "If-else condition can't be module or type");
    } else if (!_checkType(std::get<RecordStructType*>(resolved_cond.variant), this->env->getStruct("bool"))) {
        if (!this->canConvertType(std::get<RecordStructType*>(resolved_cond.variant), this->env->getStruct("bool"))) {
            errors::raiseWrongTypeError(this->file_path, this->source, if_statement->condition, nullptr, {this->env->getStruct("bool")}, "If-else condition can't be module or type");
        } else {
            resolved_cond = this->convertType(resolved_cond, this->env->getStruct("bool"));
        }
    }

    // Save the current environment and create a new one for the if-else block
    auto prev_env = this->env;
    auto new_env = Enviornment(prev_env);
    this->env = &new_env;

    // Get the current function and create basic blocks for then, else, and continue
    auto current_function = this->llvm_ir_builder.GetInsertBlock()->getParent();
    llBB* then_block = llBB::Create(this->llvm_context, "then", current_function);
    llBB* continue_block = llBB::Create(this->llvm_context, "cont", current_function);
    llBB* else_block = if_statement->alternative ? llBB::Create(this->llvm_context, "else", current_function) : nullptr;

    // Create conditional branch based on the presence of an else block
    if (else_block) {
        this->llvm_ir_builder.CreateCondBr(resolved_cond.value, then_block, else_block);
    } else {
        this->llvm_ir_builder.CreateCondBr(resolved_cond.value, then_block, continue_block);
    }

    // Set insertion point to then block and compile the consequence
    this->llvm_ir_builder.SetInsertPoint(then_block);
    try {
        this->compile(if_statement->consequence);
        this->llvm_ir_builder.CreateBr(continue_block);
    } catch (DoneRet) {
    } catch (DoneBr) {}

    // If there is an else block, set insertion point to else block and compile the alternative
    if (else_block) {
        auto new_env = Enviornment(prev_env);
        this->env = &new_env;
        this->llvm_ir_builder.SetInsertPoint(else_block);
        try {
            this->compile(if_statement->alternative);
            this->llvm_ir_builder.CreateBr(continue_block);
        } catch (DoneRet) {
        } catch (DoneBr) {}
    }

    // Restore the previous environment and set insertion point to continue block
    this->env = prev_env;
    this->llvm_ir_builder.SetInsertPoint(continue_block);
}

void Compiler::_visitWhileStatement(AST::WhileStatement* while_statement) {
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
    auto [condition_val, condition_alloca, condition_type_variant, condition_resolve_type] = this->_resolveValue(condition);

    // Ensure the condition is of type StructInst
    if (condition_resolve_type != resolveType::StructInst) {
        errors::raiseWrongTypeError(this->file_path, this->source, condition, nullptr, {this->env->getStruct("bool")}, "While loop condition cannot be a module or type.");
    }

    // Retrieve the actual condition type
    auto conditionType = std::get<RecordStructType*>(condition_type_variant);

    // Check if the condition type is bool, possibly converting if necessary
    if (!_checkType(conditionType, this->env->getStruct("bool"))) {
        if (this->canConvertType(conditionType, this->env->getStruct("bool"))) {
            auto converted = this->convertType({condition_val, condition_alloca, conditionType}, this->env->getStruct("bool"));
            condition_val = converted.value;
        } else {
            errors::raiseWrongTypeError(this->file_path, this->source, condition, conditionType, {this->env->getStruct("bool")}, "While loop condition must be of type bool.");
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
    auto prev_env = this->env;
    auto new_env = Enviornment(prev_env);
    this->env = &new_env;

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
        auto new_env = Enviornment(prev_env);
        this->env = &new_env;
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
        auto new_env = Enviornment(prev_env);
        this->env = &new_env;
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
    this->env = prev_env;

    // Exit the loop environment
    this->env->exitLoop();
}

void Compiler::_visitForStatement(AST::ForStatement* for_statement) {
    // Resolve the iterable object from the 'from' expression
    auto [iterable_value, iterable_alloca, _iterable_type, resolve_type] = this->_resolveValue(for_statement->from);

    // Ensure the resolved type is a struct instance
    if (resolve_type != resolveType::StructInst) { errors::raiseWrongTypeError(this->file_path, this->source, for_statement->from, nullptr, {}, "Cannot loop over a module or type"); }

    // Get the actual struct type of the iterable
    auto iterable_type = std::get<RecordStructType*>(_iterable_type);

    // Check if the iterable has an __iter__ method
    if (iterable_type->is_method("__iter__", {iterable_type})) {
        // Retrieve the __iter__ method and its return type (iterator type)
        auto iter_method = iterable_type->get_method("__iter__", {iterable_type});
        auto iterator_type = iter_method->return_type;

        // Ensure the iterator has both __next__ and __done__ methods
        bool has_next = iterable_type->is_method("__next__", {iterable_type, iterator_type});
        bool has_done = iterable_type->is_method("__done__", {iterable_type, iterator_type}, {}, this->env->getStruct("bool"));
        if (!has_next) {
            errors::raiseNotDefinedError(this->file_path, this->source, for_statement->from, "__next__ method not defined in struct `" + iterable_type->name + "`");
        } else if (!has_done) {
            errors::raiseNotDefinedError(this->file_path, this->source, for_statement->from, "__done__ method not defined in struct `" + iterable_type->name + "`");
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
        auto prev_env = this->env;
        auto new_env = Enviornment(prev_env);
        this->env = &new_env;

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
        RecordVariable* loop_var;
        if (next_method->return_type->struct_type) {
            loop_var = new RecordVariable(for_statement->get->value, loop_var_alloca, loop_var_alloca, next_method->return_type);
        } else {
            llvm::Value* loaded_value = this->llvm_ir_builder.CreateLoad(next_method->return_type->stand_alone_type, loop_var_alloca);
            loop_var = new RecordVariable(for_statement->get->value, loaded_value, loop_var_alloca, next_method->return_type);
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
            auto new_env = Enviornment(prev_env);
            this->env = &new_env;
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
            auto new_env = Enviornment(prev_env);
            this->env = &new_env;
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
        this->env = prev_env;
        prev_env->childes.push_back(this->env);
        this->llvm_ir_builder.SetInsertPoint(continue_block);

        // Exit the loop environment
        this->env->exitLoop();
    }
    std::cerr << "TODO: Raise Error" << std::endl;
    exit(1);
}

void Compiler::_visitTryCatchStatement(AST::TryCatchStatement* tc_statement) {
    std::cerr << "TODO: Add Suport to Handel Exception" << std::endl;
    exit(1);
}

void Compiler::_visitRaiseStatement(AST::RaiseStatement* raise_statement) {
    std::cerr << "TODO: Add Suport to raise Exception" << std::endl;
    exit(1);
}

namespace Utils {
const Str readFileToString(const std::filesystem::path& filePath); // Defined in main.cpp
}

compilationState::RecordFile* findOrCreateFileRecord(compilationState::RecordFolder* rootFolder, const std::filesystem::path& relativePath); // Defined in `main.cpp`

void Compiler::_visitImportStatement(AST::ImportStatement* import_statement, RecordModule* module) {
    // Extract the relative path from the import statement
    Str relative_path = import_statement->relativePath;

    // Derive the module name by taking the substring after the last '/' and replacing '.' with '_'
    Str module_name;
    if (import_statement->as == "") {
        module_name = relative_path.substr(relative_path.find_last_of('/') + 1);
        std::replace(module_name.begin(), module_name.end(), '.', '_');
    } else module_name = import_statement->as;

    // Determine the path for the imported source file
    std::filesystem::path gc_source_path = this->file_path.parent_path() / (relative_path + ".gc");
    if (!std::filesystem::exists(gc_source_path)) {
        errors::raiseCompletionError(this->file_path,
                                     this->source,
                                     import_statement->meta_data.st_line_no,
                                     import_statement->meta_data.st_col_no,
                                     import_statement->meta_data.end_line_no,
                                     import_statement->meta_data.end_col_no,
                                     "Imported file not found: " + gc_source_path.string());
    }

    // Check if the file is already being compiled
    compilationState::RecordFile* local_file_record = findOrCreateFileRecord(this->file_record->parent, relative_path + ".gc");

    // Wait until the file is compiled
    while (!local_file_record->compiled) { std::this_thread::sleep_for(std::chrono::milliseconds(100)); }

    // Read the source code from the file
    Str gc_source = Utils::readFileToString(gc_source_path.string());
    Str previous_source = this->source;
    this->source = gc_source;

    // Parse the source code into an AST
    auto lexer = new Lexer(gc_source_path, gc_source);
    auto parser = new parser::Parser(lexer);
    auto program = parser->parseProgram();
    delete lexer;
    delete parser;

    // Create a new module record if not importing into an existing module
    RecordModule* import_module = module;
    if (!module) {
        import_module = new RecordModule(module_name);
        this->env->addRecord(import_module);
    }

    // Save the current environment and create a new one for the imported module
    auto prev_env = this->env;
    this->env = new Enviornment(prev_env, StrRecordMap{}, module_name);
    prev_env->childes.push_back(this->env);

    // Iterate over each statement in the imported program
    for (auto& stmt : program->statements) {
        switch (stmt->type()) {
            case AST::NodeType::FunctionStatement:
                this->_importFunctionDeclarationStatement(stmt->castToFunctionStatement(), import_module, local_file_record);
                break;
            case AST::NodeType::StructStatement:
                this->_importStructStatement(stmt->castToStructStatement(), import_module, local_file_record);
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
    this->env = prev_env;
    this->source = previous_source;
}

void Compiler::_importFunctionDeclarationStatement(AST::FunctionStatement* function_declaration_statement, RecordModule* module, compilationState::RecordFile* local_file_record) {
    // Utilize the helper function with the module
    this->_createFunctionRecord(function_declaration_statement, nullptr, module, local_file_record);
}

void Compiler::_importStructStatement(AST::StructStatement* struct_statement, RecordModule* module, compilationState::RecordFile* local_file_record) {
    // Utilize the helper function with the module
    this->_createStructRecord(struct_statement, module, local_file_record);
}
