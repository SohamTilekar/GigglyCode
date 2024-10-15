#include "compiler.hpp"

compiler::Compiler::Compiler() : llvm_context(llvm::LLVMContext()), llvm_ir_builder(llvm_context) {
    this->llvm_module = std::make_unique<llvm::Module>("main", llvm_context);
    this->_initializeBuiltins();
}

compiler::Compiler::Compiler(const std::string& source) : llvm_context(llvm::LLVMContext()), llvm_ir_builder(llvm_context), source(source) {
    this->llvm_module = std::make_unique<llvm::Module>("main", llvm_context);
    this->_initializeBuiltins();
}

void compiler::Compiler::_initializeBuiltins() {
    this->enviornment.add(std::make_shared<enviornment::RecordBuiltinType>("int", llvm::Type::getInt32Ty(llvm_context)));
    this->enviornment.add(std::make_shared<enviornment::RecordBuiltinType>("float", llvm::Type::getFloatTy(llvm_context)));
    this->enviornment.add(std::make_shared<enviornment::RecordBuiltinType>("bool", llvm::Type::getInt1Ty(llvm_context)));
    this->enviornment.add(std::make_shared<enviornment::RecordBuiltinType>("char", llvm::Type::getInt8Ty(llvm_context)));
    this->enviornment.add(std::make_shared<enviornment::RecordBuiltinType>("str", llvm::PointerType::get(llvm::Type::getInt8Ty(llvm_context), 0)));
    this->enviornment.add(std::make_shared<enviornment::RecordBuiltinType>("void", llvm::Type::getVoidTy(llvm_context)));
    this->enviornment.add(std::make_shared<enviornment::RecordBuiltinType>("None", this->enviornment.get_builtin_type("void")));

    // Creating puts function for printing strings from the libc
    std::vector<llvm::Type*> puts_args = {llvm::PointerType::get(llvm::Type::getInt8Ty(llvm_context), 0)};
    llvm::FunctionType* puts_type = llvm::FunctionType::get(llvm::Type::getInt32Ty(llvm_context), puts_args, false);
    llvm::Function::Create(puts_type, llvm::Function::ExternalLinkage, "puts", this->llvm_module.get());

    // Creating print function for printing strings which uses the puts under the hood
    std::vector<llvm::Type*> print_args = {llvm::PointerType::get(llvm::Type::getInt8Ty(llvm_context), 0)};
    llvm::FunctionType* print_type = llvm::FunctionType::get(this->enviornment.get_builtin_type("void"), print_args, false);
    llvm::Function::Create(print_type, llvm::Function::ExternalLinkage, "print", this->llvm_module.get());
    this->llvm_ir_builder.SetInsertPoint(llvm::BasicBlock::Create(llvm_context, "entry", this->llvm_module->getFunction("print")));
    auto* arg = this->llvm_module->getFunction("print")->arg_begin();
    auto record = std::make_shared<enviornment::RecordVariable>(std::string(arg->getName()), nullptr, arg->getType(), nullptr);
    std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordVariable>>> arguments = {
        std::make_tuple(std::string(arg->getName()), record)};
    this->llvm_ir_builder.CreateCall(this->llvm_module->getFunction("puts"), this->llvm_ir_builder.GetInsertBlock()->getParent()->arg_begin(),
                                     "calltmp");
    this->llvm_ir_builder.CreateRetVoid();
    this->llvm_ir_builder.ClearInsertionPoint();
    this->enviornment.add(std::make_shared<enviornment::RecordFunction>("print", this->llvm_module->getFunction("print"), print_type, arguments));

    // Create the global variable 'true'
    llvm::GlobalVariable* globalTrue =
        new llvm::GlobalVariable(*this->llvm_module, this->enviornment.get_builtin_type("bool"), true, llvm::GlobalValue::ExternalLinkage,
                                 llvm::ConstantInt::get(this->enviornment.get_builtin_type("bool"), 1), "True");

    // Create the global variable 'false'
    llvm::GlobalVariable* globalFalse =
        new llvm::GlobalVariable(*this->llvm_module, this->enviornment.get_builtin_type("bool"), true, llvm::GlobalValue::ExternalLinkage,
                                 llvm::ConstantInt::get(this->enviornment.get_builtin_type("bool"), 0), "False");
    auto recordTrue = std::make_shared<enviornment::RecordVariable>("True", globalTrue, this->enviornment.get_builtin_type("bool"), nullptr);
    this->enviornment.add(recordTrue);
    auto recordFalse = std::make_shared<enviornment::RecordVariable>("False", globalFalse, this->enviornment.get_builtin_type("bool"), nullptr);
    this->enviornment.add(recordFalse);
};

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
    case AST::NodeType::BlockStatement: {
        this->_visitBlockStatement(std::static_pointer_cast<AST::BlockStatement>(node));
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
    case AST::NodeType::WhileStatement: {
        this->_visitWhileStatement(std::static_pointer_cast<AST::WhileStatement>(node));
        break;
    }
    case AST::NodeType::BreakStatement: {
        if(this->enviornment.loop_end_block.empty()) {
            // errors::BreakOutsideLoopError(this->source, node->meta_data, "Break statement outside loop").raise();
            std::cout << "Break statement outside loop" << std::endl;
        }
        this->llvm_ir_builder.CreateBr(this->enviornment.loop_end_block.top());
        break;
    }
    case AST::NodeType::ContinueStatement: {
        if(this->enviornment.loop_condition_block.empty()) {
            // errors::ContinueOutsideLoopError(this->source, node->meta_data, "Continue statement outside loop").raise();
            std::cout << "Continue statement outside loop" << std::endl;
        }
        this->llvm_ir_builder.CreateBr(this->enviornment.loop_condition_block.top());
        break;
    }
    case AST::NodeType::BooleanLiteral: {
        auto boolean_literal = std::static_pointer_cast<AST::BooleanLiteral>(node);
        auto value = llvm::ConstantInt::get(llvm_context, llvm::APInt(1, boolean_literal->value, true));
        break;
    }
    case AST::NodeType::ClassDeclarationStatement: {
        this->_visitClassDeclarationStatement(std::static_pointer_cast<AST::ClassDeclarationStatement>(node));
        break;
    }
    default:
        errors::InternalCompilationError("Unknown node type", this->source, node->meta_data.st_line_no, node->meta_data.end_line_no,
                                         "Unknown node type: " + *AST::nodeTypeToString(node->type()))
            .raise();
        break;
    }
};

void compiler::Compiler::_visitProgram(std::shared_ptr<AST::Program> program) {
    for(auto stmt : program->statements) {
        this->compile(stmt);
    }
};

void compiler::Compiler::_visitExpressionStatement(std::shared_ptr<AST::ExpressionStatement> expression_statement) {
    this->compile(expression_statement->expr);
};

std::tuple<llvm::Value*, llvm::Type*> compiler::Compiler::_visitInfixExpression(std::shared_ptr<AST::InfixExpression> infixed_expression) {
    auto op = infixed_expression->op;
    auto left = infixed_expression->left;
    auto right = infixed_expression->right;
    auto [left_value, left_type] = this->_resolveValue(left);
    auto [right_value, right_type] = this->_resolveValue(right);
    llvm::Value* result_value = nullptr;
    llvm::Type* result_type = nullptr;
    if(left_type == this->enviornment.get_builtin_type("int") && right_type == this->enviornment.get_builtin_type("int")) {
        if(op == token::TokenType::Plus) {
            result_value = this->llvm_ir_builder.CreateAdd(left_value, right_value, "addtmp");
            result_type = this->enviornment.get_builtin_type("int");
        } else if(op == token::TokenType::Dash) {
            result_value = this->llvm_ir_builder.CreateSub(left_value, right_value, "subtmp");
            result_type = this->enviornment.get_builtin_type("int");
        } else if(op == token::TokenType::Asterisk) {
            result_value = this->llvm_ir_builder.CreateMul(left_value, right_value, "multmp");
            result_type = this->enviornment.get_builtin_type("int");
        } else if(op == token::TokenType::ForwardSlash) {
            result_value = this->llvm_ir_builder.CreateSDiv(left_value, right_value, "divtmp");
            result_type = this->enviornment.get_builtin_type("int");
        } else if(op == token::TokenType::GreaterThan) {
            result_value = this->llvm_ir_builder.CreateICmpSGT(left_value, right_value, "gttmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::LessThan) {
            result_value = this->llvm_ir_builder.CreateICmpSLT(left_value, right_value, "lttmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::EqualEqual) {
            result_value = this->llvm_ir_builder.CreateICmpEQ(left_value, right_value, "eqtmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::GreaterThanOrEqual) {
            result_value = this->llvm_ir_builder.CreateICmpSGE(left_value, right_value, "getmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::LessThanOrEqual) {
            result_value = this->llvm_ir_builder.CreateICmpSLE(left_value, right_value, "letmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::NotEquals) {
            result_value = this->llvm_ir_builder.CreateICmpNE(left_value, right_value, "netmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else {
            errors::UnsupportedOperationError(this->source, infixed_expression->meta_data,
                                              "`int` Dose Not Support Operator: `" +
                                                  std::get<std::string>(infixed_expression->meta_data.more_data["operator_literal"]) + "`")
                .raise();
        }
    } else if(left_type == this->enviornment.get_builtin_type("float") && right_type == this->enviornment.get_builtin_type("float")) {
        if(op == token::TokenType::Plus) {
            result_value = this->llvm_ir_builder.CreateFAdd(left_value, right_value, "addtmp");
            result_type = this->enviornment.get_builtin_type("float");
        } else if(op == token::TokenType::Dash) {
            result_value = this->llvm_ir_builder.CreateFSub(left_value, right_value, "subtmp");
            result_type = this->enviornment.get_builtin_type("float");
        } else if(op == token::TokenType::Asterisk) {
            result_value = this->llvm_ir_builder.CreateFMul(left_value, right_value, "multmp");
            result_type = this->enviornment.get_builtin_type("float");
        } else if(op == token::TokenType::ForwardSlash) {
            result_value = this->llvm_ir_builder.CreateFDiv(left_value, right_value, "divtmp");
            result_type = this->enviornment.get_builtin_type("float");
        } else if(op == token::TokenType::GreaterThan) {
            result_value = this->llvm_ir_builder.CreateFCmpOGT(left_value, right_value, "gttmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::LessThan) {
            result_value = this->llvm_ir_builder.CreateFCmpOLT(left_value, right_value, "lttmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::EqualEqual) {
            result_value = this->llvm_ir_builder.CreateFCmpOEQ(left_value, right_value, "eqtmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::GreaterThanOrEqual) {
            result_value = this->llvm_ir_builder.CreateFCmpOGE(left_value, right_value, "getmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::LessThanOrEqual) {
            result_value = this->llvm_ir_builder.CreateFCmpOLE(left_value, right_value, "letmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::NotEquals) {
            result_value = this->llvm_ir_builder.CreateFCmpONE(left_value, right_value, "netmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else {
            errors::UnsupportedOperationError(this->source, infixed_expression->meta_data,
                                              "`float` Dose Not Support Operator: `" +
                                                  std::get<std::string>(infixed_expression->meta_data.more_data["operator_literal"]) + "`")
                .raise();
        }
    } else if(left_type == this->enviornment.get_builtin_type("bool") && right_type == this->enviornment.get_builtin_type("bool")) {
        if(op == token::TokenType::And) {
            result_value = this->llvm_ir_builder.CreateAnd(left_value, right_value, "andtmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::Or) {
            result_value = this->llvm_ir_builder.CreateOr(left_value, right_value, "ortmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::EqualEqual) {
            result_value = this->llvm_ir_builder.CreateICmpEQ(left_value, right_value, "eqtmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::NotEquals) {
            result_value = this->llvm_ir_builder.CreateICmpNE(left_value, right_value, "netmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::GreaterThan) {
            result_value = this->llvm_ir_builder.CreateICmpSGT(left_value, right_value, "gttmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::LessThan) {
            result_value = this->llvm_ir_builder.CreateICmpSLT(left_value, right_value, "lttmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::GreaterThanOrEqual) {
            result_value = this->llvm_ir_builder.CreateICmpSGE(left_value, right_value, "getmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else if(op == token::TokenType::LessThanOrEqual) {
            result_value = this->llvm_ir_builder.CreateICmpSLE(left_value, right_value, "letmp");
            result_type = this->enviornment.get_builtin_type("bool");
        } else {
            errors::UnsupportedOperationError(this->source, infixed_expression->meta_data,
                                              "`bool` Dose Not Support Operator: `" +
                                                  std::get<std::string>(infixed_expression->meta_data.more_data["operator_literal"]) + "`")
                .raise();
        }
    } else {
        std::cout << "Unknown Type" << std::endl;
    }
    return {result_value, result_type};
};

void compiler::Compiler::_visitVariableDeclarationStatement(std::shared_ptr<AST::VariableDeclarationStatement> variable_declaration_statement) {
    auto var_name = std::static_pointer_cast<AST::IdentifierLiteral>(variable_declaration_statement->name);
    auto var_value = variable_declaration_statement->value;
    auto var_type =
        this->enviornment.get_builtin_type(std::dynamic_pointer_cast<AST::IdentifierLiteral>(
                                               std::dynamic_pointer_cast<AST::GenericType>(variable_declaration_statement->value_type)->name)
                                               ->value);
    auto [value, type] = this->_resolveValue(var_value);
    if(!this->enviornment.contains(var_name->value, true)) {
        llvm::AllocaInst* alloca =
            this->llvm_ir_builder.CreateAlloca(var_type, nullptr, std::dynamic_pointer_cast<AST::IdentifierLiteral>(var_name)->value);
        this->llvm_ir_builder.CreateStore(value, alloca);
        auto record = std::make_shared<enviornment::RecordVariable>(var_name->value, value, var_type, alloca);
        record->set_meta_data(variable_declaration_statement->meta_data.st_line_no, variable_declaration_statement->meta_data.st_col_no,
                              variable_declaration_statement->meta_data.end_line_no, variable_declaration_statement->meta_data.end_col_no);
        record->meta_data.more_data["name_line_no"] = var_name->meta_data.st_line_no;
        record->meta_data.more_data["name_st_col_no"] = var_name->meta_data.st_col_no;
        record->meta_data.more_data["name_end_col_no"] = var_name->meta_data.end_col_no;
        this->enviornment.add(record);
    } else if(this->enviornment.is_variable(var_name->value)) {
        errors::VariableRedeclarationError(this->source, this->enviornment.get_variable(var_name->value)->meta_data, *variable_declaration_statement)
            .raise();
    } else {
        errors::VariableRedeclarationError(this->source, this->enviornment.get(var_name->value)->meta_data, *variable_declaration_statement).raise();
    }
};

void compiler::Compiler::_visitVariableAssignmentStatement(std::shared_ptr<AST::VariableAssignmentStatement> variable_assignment_statement) {
    auto var_name = std::static_pointer_cast<AST::IdentifierLiteral>(variable_assignment_statement->name);
    auto var_value = variable_assignment_statement->value;
    auto [value, type] = this->_resolveValue(var_value);
    if(this->enviornment.is_variable(var_name->value)) {
        auto var = this->enviornment.get_variable(var_name->value);
        this->llvm_ir_builder.CreateStore(value, var->allocainst);
    } else {
        errors::UndefinedVariableError(this->source, var_name->meta_data, "Assigning to undefine Variable",
                                       "Define the variable fist before reassigning")
            .raise();
    }
};

void compiler::Compiler::_visitIfElseStatement(std::shared_ptr<AST::IfElseStatement> if_statement) {
    auto condition = if_statement->condition;
    auto consequence = if_statement->consequence;
    auto alternative = if_statement->alternative;
    auto [condition_val, _] = this->_resolveValue(condition);
    if(alternative == nullptr) {
        auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(llvm_context, "then", func);
        llvm::BasicBlock* ContBB = llvm::BasicBlock::Create(llvm_context, "cont", func);
        this->llvm_ir_builder.CreateCondBr(condition_val, ThenBB, ContBB);
        this->llvm_ir_builder.SetInsertPoint(ThenBB);
        this->compile(consequence);
        this->llvm_ir_builder.CreateBr(ContBB);
        this->llvm_ir_builder.SetInsertPoint(ContBB);
    } else {
        auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(llvm_context, "then", func);
        llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(llvm_context, "else", func);
        llvm::BasicBlock* ContBB = llvm::BasicBlock::Create(llvm_context, "cont", func);
        this->llvm_ir_builder.CreateCondBr(condition_val, ThenBB, ElseBB);
        this->llvm_ir_builder.SetInsertPoint(ThenBB);
        this->compile(consequence);
        this->llvm_ir_builder.CreateBr(ContBB);
        this->llvm_ir_builder.SetInsertPoint(ElseBB);
        this->compile(alternative);
        this->llvm_ir_builder.CreateBr(ContBB);
        this->llvm_ir_builder.SetInsertPoint(ContBB);
    }
};

void compiler::Compiler::_visitBlockStatement(std::shared_ptr<AST::BlockStatement> block_statement) {
    for(auto stmt : block_statement->statements) {
        this->compile(stmt);
    }
};

void compiler::Compiler::_visitFunctionDeclarationStatement(std::shared_ptr<AST::FunctionStatement> function_declaration_statement) {
    auto name = std::static_pointer_cast<AST::IdentifierLiteral>(function_declaration_statement->name)->value;
    auto body = function_declaration_statement->body;
    auto params = function_declaration_statement->parameters;
    std::vector<std::string> param_name;
    std::vector<llvm::Type*> param_types;
    for(auto param : params) {
        param_name.push_back(std::static_pointer_cast<AST::IdentifierLiteral>(param->name)->value);
        param_types.push_back(this->enviornment.get_builtin_type(
            std::dynamic_pointer_cast<AST::IdentifierLiteral>(std::dynamic_pointer_cast<AST::GenericType>(param->value_type)->name)->value));
    }
    auto return_type =
        this->enviornment.get_builtin_type(std::dynamic_pointer_cast<AST::IdentifierLiteral>(
                                               std::dynamic_pointer_cast<AST::GenericType>(function_declaration_statement->returnType)->name)
                                               ->value);
    auto func_type = llvm::FunctionType::get(return_type, param_types, false);
    auto func = llvm::Function::Create(func_type, llvm::Function::ExternalLinkage, name, this->llvm_module.get());

    // name the parameters
    unsigned idx = 0;
    for(auto& arg : func->args()) {
        arg.setName(param_name[idx++]);
    }
    auto bb = llvm::BasicBlock::Create(llvm_context, name + "_entry", func);
    this->llvm_ir_builder.SetInsertPoint(bb);
    auto prev_env = std::make_shared<enviornment::Enviornment>(this->enviornment);
    this->enviornment = enviornment::Enviornment(prev_env, {}, name);
    std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordVariable>>> arguments;
    for(auto& arg : func->args()) {
        auto alloca = this->llvm_ir_builder.CreateAlloca(arg.getType(), nullptr, arg.getName());
        this->llvm_ir_builder.CreateStore(&arg, alloca);
        auto record = std::make_shared<enviornment::RecordVariable>(std::string(arg.getName()), &arg, arg.getType(), alloca);
        arguments.push_back(std::make_tuple(std::string(arg.getName()), record));
        this->enviornment.add(record);
    }
    auto func_record = std::make_shared<enviornment::RecordFunction>(name, func, func_type, arguments);
    func_record->set_meta_data(function_declaration_statement->meta_data.st_line_no, function_declaration_statement->meta_data.st_col_no,
                               function_declaration_statement->meta_data.end_line_no, function_declaration_statement->meta_data.end_col_no);
    func_record->meta_data.more_data["name_line_no"] = function_declaration_statement->name->meta_data.st_line_no;
    func_record->meta_data.more_data["name_st_col_no"] = function_declaration_statement->name->meta_data.st_col_no;
    func_record->meta_data.more_data["name_end_col_no"] = function_declaration_statement->name->meta_data.end_col_no;
    this->enviornment.add(func_record);
    // adding the alloca for the parameters
    this->current_function = func;
    this->compile(body);
    this->enviornment = *prev_env;
    this->enviornment.add(func_record);
}

std::tuple<llvm::Value*, llvm::Type*> compiler::Compiler::_visitCallExpression(std::shared_ptr<AST::CallExpression> call_expression) {
    auto name = std::static_pointer_cast<AST::IdentifierLiteral>(call_expression->name)->value;
    auto param = call_expression->arguments;
    auto args = std::vector<llvm::Value*>();
    for(auto arg : param) {
        auto [value, type] = this->_resolveValue(arg);
        args.push_back(value);
    }
    if(this->enviornment.is_function(name)) {
        auto func_record = this->enviornment.get_function(name);
        auto returnValue = this->llvm_ir_builder.CreateCall(
            llvm::cast<llvm::Function>(func_record->function), args,
            func_record->function_type->getReturnType() != this->enviornment.get_builtin_type("void") ? "calltmp" : "");
        return {returnValue, func_record->function_type};
    }
    errors::UndefinedFunctionError(this->source, call_expression->meta_data, "Function `" + name + "` not defined", "Define the function first")
        .raise();
    std::exit(1);
    return {nullptr, nullptr};
};

void compiler::Compiler::_visitReturnStatement(std::shared_ptr<AST::ReturnStatement> return_statement) {
    auto value = return_statement->value;
    auto [return_value, return_type] = this->_resolveValue(value);
    this->llvm_ir_builder.CreateRet(return_value);
};

void compiler::Compiler::_visitWhileStatement(std::shared_ptr<AST::WhileStatement> while_statement) {
    auto condition = while_statement->condition;
    auto body = while_statement->body;
    auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();
    llvm::BasicBlock* CondBB = llvm::BasicBlock::Create(llvm_context, "cond", func);
    llvm::BasicBlock* BodyBB = llvm::BasicBlock::Create(llvm_context, "body", func);
    llvm::BasicBlock* ContBB = llvm::BasicBlock::Create(llvm_context, "cont", func);
    this->llvm_ir_builder.CreateBr(CondBB);
    this->llvm_ir_builder.SetInsertPoint(CondBB);
    auto [condition_val, _] = this->_resolveValue(condition);
    this->llvm_ir_builder.CreateCondBr(condition_val, BodyBB, ContBB);
    this->llvm_ir_builder.SetInsertPoint(BodyBB);
    this->enviornment.loop_body_block.push(BodyBB);
    this->enviornment.loop_end_block.push(ContBB);
    this->enviornment.loop_condition_block.push(CondBB);
    this->compile(body);
    this->enviornment.loop_body_block.pop();
    this->enviornment.loop_end_block.pop();
    this->enviornment.loop_condition_block.pop();
    this->llvm_ir_builder.CreateBr(CondBB);
    this->llvm_ir_builder.SetInsertPoint(ContBB);
};

void compiler::Compiler::_visitClassDeclarationStatement(std::shared_ptr<AST::ClassDeclarationStatement> class_declaration) {
    auto name = std::static_pointer_cast<AST::IdentifierLiteral>(class_declaration->name)->value;
    auto variables = class_declaration->variables;
    auto methods = class_declaration->methods;
    std::vector<llvm::Type*> Elements;
    std::vector<std::string> ElementNames;
    for(auto var : variables) {
        auto var_name = std::static_pointer_cast<AST::IdentifierLiteral>(var->name)->value;
        auto var_type = this->enviornment.get_builtin_type(
            std::dynamic_pointer_cast<AST::IdentifierLiteral>(std::dynamic_pointer_cast<AST::GenericType>(var->value_type)->name)->value);
        Elements.push_back(var_type);
        ElementNames.push_back(var_name);
    }
    llvm::StructType* MyStructType = llvm::StructType::create(this->llvm_context, Elements, name);
    this->enviornment.add(std::make_shared<enviornment::RecordClassType>(name, MyStructType, ElementNames));
    auto prev_env = std::make_shared<enviornment::Enviornment>(this->enviornment);
    this->enviornment = enviornment::Enviornment(prev_env, {}, name);
    this->enviornment.add(std::make_shared<enviornment::RecordClassType>(name, MyStructType, ElementNames));
};

std::tuple<llvm::Value*, llvm::Type*> compiler::Compiler::_resolveValue(std::shared_ptr<AST::Node> node) {
    switch(node->type()) {
    case AST::NodeType::IntegerLiteral: {
        auto integer_literal = std::static_pointer_cast<AST::IntegerLiteral>(node);
        auto value = llvm::ConstantInt::get(llvm_context, llvm::APInt(32, integer_literal->value, true));
        return {value, this->enviornment.get_builtin_type("int")};
    }
    case AST::NodeType::FloatLiteral: {
        auto float_literal = std::static_pointer_cast<AST::FloatLiteral>(node);
        auto value = llvm::ConstantFP::get(llvm_context, llvm::APFloat(float_literal->value));
        return {value, this->enviornment.get_builtin_type("float")};
    }
    case AST::NodeType::BooleanLiteral: {
        auto boolean_literal = std::static_pointer_cast<AST::BooleanLiteral>(node);
        auto value = llvm::ConstantInt::get(llvm_context, llvm::APInt(1, boolean_literal->value, true));
        return {value, this->enviornment.get_builtin_type("bool")};
    }
    case AST::NodeType::StringLiteral: {
        auto string_literal = std::static_pointer_cast<AST::StringLiteral>(node);
        auto value = this->llvm_ir_builder.CreateGlobalStringPtr(string_literal->value);
        auto alloca = this->llvm_ir_builder.CreateAlloca(value->getType());
        this->llvm_ir_builder.CreateStore(value, alloca);
        return {alloca, this->enviornment.get_builtin_type("str")};
    }
    case AST::NodeType::IdentifierLiteral: {
        auto identifier_literal = std::static_pointer_cast<AST::IdentifierLiteral>(node);
        if(!this->enviornment.is_variable(identifier_literal->value)) {
            errors::UndefinedVariableError(this->source, identifier_literal->meta_data, "Variable `" + identifier_literal->value + "` not defined",
                                           "Define the variable first")
                .raise();
            return {nullptr, nullptr};
        }
        auto var = this->enviornment.get_variable(identifier_literal->value);
        return std::tuple<llvm::Value*, llvm::Type*>{this->llvm_ir_builder.CreateLoad(var->type, var->allocainst, identifier_literal->value),
                                                     var->type};
    }
    case AST::NodeType::CallExpression: {
        auto call_expression = std::static_pointer_cast<AST::CallExpression>(node);
        return this->_visitCallExpression(call_expression);
    }
    case AST::NodeType::ExpressionStatement: {
        auto expression_statement = std::static_pointer_cast<AST::ExpressionStatement>(node);
        return this->_resolveValue(expression_statement->expr);
    }
    case AST::NodeType::InfixedExpression: {
        auto infix_expression = std::static_pointer_cast<AST::InfixExpression>(node);
        return this->_visitInfixExpression(infix_expression);
    }
    case AST::NodeType::ClassDeclarationStatement: {
        auto class_declaration = std::static_pointer_cast<AST::ClassDeclarationStatement>(node);
        this->_visitClassDeclarationStatement(class_declaration);
    }
    default:
        errors::InternalCompilationError("Unknown node type", this->source, node->meta_data.st_line_no, node->meta_data.end_line_no,
                                         "Unknown node type: " + *AST::nodeTypeToString(node->type()))
            .raise();
        return {nullptr, nullptr};
    }
}
