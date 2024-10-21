#include "compiler.hpp"
#include <llvm/IR/Instructions.h>
#include <memory>
#include <unordered_map>
#include <vector>

compiler::Compiler::Compiler() : llvm_context(llvm::LLVMContext()), llvm_ir_builder(llvm_context) {
    this->llvm_module = std::make_unique<llvm::Module>("main", llvm_context);
    this->_initializeBuiltins();
}

compiler::Compiler::Compiler(const std::string& source) : llvm_context(llvm::LLVMContext()), llvm_ir_builder(llvm_context), source(source) {
    this->llvm_module = std::make_unique<llvm::Module>("main", llvm_context);
    this->_initializeBuiltins();
}

void compiler::Compiler::_initializeBuiltins() {
    auto _int = std::make_shared<enviornment::RecordStructType>("int", llvm::Type::getInt64Ty(llvm_context));
    this->enviornment.add(_int);
    auto _float = std::make_shared<enviornment::RecordStructType>("float", llvm::Type::getDoubleTy(llvm_context));
    this->enviornment.add(_float);
    auto _char = std::make_shared<enviornment::RecordStructType>("char", llvm::Type::getInt8Ty(llvm_context));
    this->enviornment.add(_char);
    auto _string = std::make_shared<enviornment::RecordStructType>("str", llvm::PointerType::get(llvm::Type::getInt8Ty(llvm_context), 0));
    this->enviornment.add(_string);
    auto _void = std::make_shared<enviornment::RecordStructType>("void", llvm::Type::getVoidTy(llvm_context));
    this->enviornment.add(_void);
    auto _bool = std::make_shared<enviornment::RecordStructType>("bool", llvm::Type::getInt1Ty(llvm_context));
    this->enviornment.add(_bool);

    // Create the global variable 'true'
    llvm::GlobalVariable* globalTrue =
        new llvm::GlobalVariable(*this->llvm_module, this->enviornment.get_struct("bool")->stand_alone_type, true, llvm::GlobalValue::ExternalLinkage,
                                 llvm::ConstantInt::get(this->enviornment.get_struct("bool")->stand_alone_type, 1), "True");

    // Create the global variable 'false'
    llvm::GlobalVariable* globalFalse =
        new llvm::GlobalVariable(*this->llvm_module, this->enviornment.get_struct("bool")->stand_alone_type, true, llvm::GlobalValue::ExternalLinkage,
                                 llvm::ConstantInt::get(this->enviornment.get_struct("bool")->stand_alone_type, 0), "False");
    auto recordTrue = std::make_shared<enviornment::RecordVariable>("True", globalTrue, this->enviornment.get_struct("bool")->stand_alone_type,
                                                                    nullptr, this->enviornment.get_struct("bool"));
    this->enviornment.add(recordTrue);
    auto recordFalse = std::make_shared<enviornment::RecordVariable>("False", globalFalse, this->enviornment.get_struct("bool")->stand_alone_type,
                                                                     nullptr, this->enviornment.get_struct("bool"));
    this->enviornment.add(recordFalse);

    // Create the function type: void puts(const char*)
    llvm::Type* voidType = llvm::Type::getVoidTy(llvm_context);                          // void return type
    llvm::FunctionType* putsType = llvm::FunctionType::get(voidType, _string->stand_alone_type, false);
    auto puts = llvm::Function::Create(putsType, llvm::Function::ExternalLinkage, "puts", this->llvm_module.get());
    std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordVariable>>> putsParams = {{"string", nullptr}};

    // Create the function type: int print(const char*)
    llvm::Type* returnType = llvm::Type::getInt32Ty(llvm_context);                          // int return type
    llvm::FunctionType* funcType = llvm::FunctionType::get(returnType, _string->stand_alone_type, false);
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, "print", this->llvm_module.get());
    std::vector<std::tuple<std::string, std::shared_ptr<enviornment::RecordVariable>>> params = {{"string", nullptr}};
    this->enviornment.add(std::make_shared<enviornment::RecordFunction>("print", func, funcType, params, _int));
}

void compiler::Compiler::compile(std::shared_ptr<AST::Node> node) {
    std::cout << "Compiling node of type: " << *AST::nodeTypeToString(node->type()) << std::endl;
    switch(node->type()) {
    case AST::NodeType::Program:
        std::cout << "Visiting Program node" << std::endl;
        this->_visitProgram(std::static_pointer_cast<AST::Program>(node));
        break;
    case AST::NodeType::ExpressionStatement: {
        std::cout << "Visiting ExpressionStatement node" << std::endl;
        this->_visitExpressionStatement(std::static_pointer_cast<AST::ExpressionStatement>(node));
        break;
    }
    case AST::NodeType::InfixedExpression: {
        std::cout << "Visiting InfixedExpression node" << std::endl;
        this->_visitInfixExpression(std::static_pointer_cast<AST::InfixExpression>(node));
        break;
    }
    case AST::NodeType::VariableDeclarationStatement: {
        std::cout << "Visiting VariableDeclarationStatement node" << std::endl;
        this->_visitVariableDeclarationStatement(std::static_pointer_cast<AST::VariableDeclarationStatement>(node));
        break;
    }
    case AST::NodeType::VariableAssignmentStatement: {
        std::cout << "Visiting VariableAssignmentStatement node" << std::endl;
        this->_visitVariableAssignmentStatement(std::static_pointer_cast<AST::VariableAssignmentStatement>(node));
        break;
    }
    case AST::NodeType::IfElseStatement: {
        std::cout << "Visiting IfElseStatement node" << std::endl;
        this->_visitIfElseStatement(std::static_pointer_cast<AST::IfElseStatement>(node));
        break;
    }
    case AST::NodeType::FunctionStatement: {
        std::cout << "Visiting FunctionStatement node" << std::endl;
        this->_visitFunctionDeclarationStatement(std::static_pointer_cast<AST::FunctionStatement>(node));
        break;
    }
    case AST::NodeType::CallExpression: {
        std::cout << "Visiting CallExpression node" << std::endl;
        this->_visitCallExpression(std::static_pointer_cast<AST::CallExpression>(node));
        break;
    }
    case AST::NodeType::ReturnStatement: {
        std::cout << "Visiting ReturnStatement node" << std::endl;
        this->_visitReturnStatement(std::static_pointer_cast<AST::ReturnStatement>(node));
        break;
    }
    case AST::NodeType::BlockStatement: {
        std::cout << "Visiting BlockStatement node" << std::endl;
        this->_visitBlockStatement(std::static_pointer_cast<AST::BlockStatement>(node));
        break;
    }
    case AST::NodeType::WhileStatement: {
        std::cout << "Visiting WhileStatement node" << std::endl;
        this->_visitWhileStatement(std::static_pointer_cast<AST::WhileStatement>(node));
        break;
    }
    case AST::NodeType::BreakStatement: {
        std::cout << "Visiting BreakStatement node" << std::endl;
        if(this->enviornment.loop_end_block.empty()) {
            std::cout << "Break statement outside loop" << std::endl;
        }
        this->llvm_ir_builder.CreateBr(this->enviornment.loop_end_block.top());
        break;
    }
    case AST::NodeType::ContinueStatement: {
        std::cout << "Visiting ContinueStatement node" << std::endl;
        if(this->enviornment.loop_condition_block.empty()) {
            std::cout << "Continue statement outside loop" << std::endl;
        }
        this->llvm_ir_builder.CreateBr(this->enviornment.loop_condition_block.top());
        break;
    }
    case AST::NodeType::BooleanLiteral: {
        std::cout << "Visiting BooleanLiteral node" << std::endl;
        auto boolean_literal = std::static_pointer_cast<AST::BooleanLiteral>(node);
        auto value = llvm::ConstantInt::get(llvm_context, llvm::APInt(1, boolean_literal->value, true));
        break;
    }
    case AST::NodeType::StructStatement: {
        std::cout << "Visiting StructStatement node" << std::endl;
        this->_visitStructStatement(std::static_pointer_cast<AST::StructStatement>(node));
        break;
    }
    default:
        std::cout << "Unknown node type: " << *AST::nodeTypeToString(node->type()) << std::endl;
        errors::CompletionError("Unknown node type", this->source, node->meta_data.st_line_no, node->meta_data.end_line_no,
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

void compiler::Compiler::_visitBlockStatement(std::shared_ptr<AST::BlockStatement> block_statement) {
    for(auto stmt : block_statement->statements) {
        this->compile(stmt);
    }
};

std::tuple<std::vector<llvm::Value*>, std::shared_ptr<enviornment::RecordStructType>> compiler::Compiler::_visitInfixExpression(
    std::shared_ptr<AST::InfixExpression> infixed_expression) {
    auto op = infixed_expression->op;
    auto left = infixed_expression->left;
    auto right = infixed_expression->right;
    auto [left_value, left_type] = this->_resolveValue(left);
    auto [right_value, right_type] = this->_resolveValue(right);
    if(left_value.size() != 1 || right_value.size() != 1) {
        std::cout << "Infix expression with multiple values" << std::endl;
        exit(1);
    }
    auto left_val = left_value[0];
    auto right_val = right_value[0];
    if(left_type->stand_alone_type != right_type->stand_alone_type) {
        // errors::TypeMismatchError(this->source, infixed_expression->meta_data, "Type mismatch").raise();
        std::cout << "Type mismatch" << std::endl;
        exit(1);
    }
    if(left_type->stand_alone_type->isIntegerTy() && right_type->stand_alone_type->isIntegerTy()) {
        if(op == token::TokenType::Plus) {
            return {{this->llvm_ir_builder.CreateAdd(left_val, right_val)}, this->enviornment.get_struct("int")};
        } else if(op == token::TokenType::Dash) {
            return {{this->llvm_ir_builder.CreateSub(left_val, right_val)}, this->enviornment.get_struct("int")};
        } else if(op == token::TokenType::Asterisk) {
            return {{this->llvm_ir_builder.CreateMul(left_val, right_val)}, this->enviornment.get_struct("int")};
        } else if(op == token::TokenType::ForwardSlash) {
            return {{this->llvm_ir_builder.CreateSDiv(left_val, right_val)}, this->enviornment.get_struct("int")};
        } else if(op == token::TokenType::Percent) {
            return {{this->llvm_ir_builder.CreateSRem(left_val, right_val)}, this->enviornment.get_struct("int")};
        } else if(op == token::TokenType::EqualEqual) {
            return {{this->llvm_ir_builder.CreateICmpEQ(left_val, right_val)}, this->enviornment.get_struct("bool")};
        } else if(op == token::TokenType::NotEquals) {
            return {{this->llvm_ir_builder.CreateICmpNE(left_val, right_val)}, this->enviornment.get_struct("bool")};
        } else if(op == token::TokenType::LessThan) {
            return {{this->llvm_ir_builder.CreateICmpSLT(left_val, right_val)}, this->enviornment.get_struct("bool")};
        } else if(op == token::TokenType::GreaterThan) {
            return {{this->llvm_ir_builder.CreateICmpSGT(left_val, right_val)}, this->enviornment.get_struct("bool")};
        } else if(op == token::TokenType::LessThanOrEqual) {
            return {{this->llvm_ir_builder.CreateICmpSLE(left_val, right_val)}, this->enviornment.get_struct("bool")};
        } else if(op == token::TokenType::GreaterThanOrEqual) {
            return {{this->llvm_ir_builder.CreateICmpSGE(left_val, right_val)}, this->enviornment.get_struct("bool")};
        } else {
            // errors::InternalCompilationError("Unknown operator", this->source, infixed_expression->meta_data.st_line_no,
            //                                  infixed_expression->meta_data.end_line_no, "Unknown operator")
            //     .raise();
            std::cout << "Unknown operator" << std::endl;
            exit(1);
        }
    } else {
        std::cout << "Unknown operator" << std::endl;
        exit(1);
    }
};


void compiler::Compiler::_visitVariableDeclarationStatement(std::shared_ptr<AST::VariableDeclarationStatement> variable_declaration_statement) {
    std::cout << "Entering _visitVariableDeclarationStatement" << std::endl;
    auto var_name = std::static_pointer_cast<AST::IdentifierLiteral>(variable_declaration_statement->name);
    std::cout << "Variable name: " << var_name->value << std::endl;
    auto var_value = variable_declaration_statement->value;
    std::cout << "Resolving variable type" << std::endl;
    if(!this->enviornment.is_struct(std::static_pointer_cast<AST::IdentifierLiteral>(
                                        std::static_pointer_cast<AST::GenericType>(variable_declaration_statement->value_type)->name)
                                        ->value)) {
        std::cout << "Variable type not defined" << std::endl;
        exit(1);
    }
    auto var_type = this->enviornment.get_struct(
        std::static_pointer_cast<AST::IdentifierLiteral>(std::static_pointer_cast<AST::GenericType>(variable_declaration_statement->value_type)->name)
            ->value);
    std::cout << "Variable type resolved: " << var_type->name << std::endl;
    std::cout << "Resolving variable value" << std::endl;
    auto [var_value_resolved, _] = this->_resolveValue(var_value);
    if(var_value_resolved.size() == 1) {
        if (var_type->stand_alone_type != nullptr) {
            std::cout << "Variable value resolved" << std::endl;
            auto alloca = this->llvm_ir_builder.CreateAlloca(var_type->stand_alone_type, nullptr, var_name->value);
            std::cout << "Created alloca for variable: " << var_name->value << std::endl;
            this->llvm_ir_builder.CreateStore(var_value_resolved[0], alloca);
            std::cout << "Stored value in alloca" << std::endl;
            auto var =
                std::make_shared<enviornment::RecordVariable>(var_name->value, var_value_resolved[0], var_type->stand_alone_type, alloca, var_type);
            this->enviornment.add(var);
            std::cout << "Variable added to environment: " << var_name->value << std::endl;
        }
        else {
            std::cout << "Variable value resolved" << std::endl;
            auto alloca = this->llvm_ir_builder.CreateAlloca(var_type->struct_type, nullptr, var_name->value);
            std::cout << "Created alloca for variable: " << var_name->value << std::endl;
            this->llvm_ir_builder.CreateStore(var_value_resolved[0], alloca);
            std::cout << "Stored value in alloca" << std::endl;
            auto var =
                std::make_shared<enviornment::RecordVariable>(var_name->value, var_value_resolved[0], var_type->struct_type, alloca, var_type);
            this->enviornment.add(var);
            std::cout << "Variable added to environment: " << var_name->value << std::endl;
        }
    } else {
        std::cout << "Variable declaration with multiple values" << std::endl;
        exit(1);
    }
    std::cout << "Exiting _visitVariableDeclarationStatement" << std::endl;
}

void compiler::Compiler::_visitVariableAssignmentStatement(std::shared_ptr<AST::VariableAssignmentStatement> variable_assignment_statement) {
    auto var_name = std::static_pointer_cast<AST::IdentifierLiteral>(variable_assignment_statement->name);
    auto var_value = variable_assignment_statement->value;
    auto [value, _] = this->_resolveValue(var_value);
    if(this->enviornment.is_variable(var_name->value)) {
        auto var = this->enviornment.get_variable(var_name->value);
        if(value.size() == 1) {
            this->llvm_ir_builder.CreateStore(value[0], var->allocainst);
        } else {
            // errors::InternalCompilationError("Variable assignment with multiple values", this->source, var_name->meta_data.st_line_no,
            //                                  var_name->meta_data.end_line_no, "Variable assignment with multiple values")
            //     .raise();
            std::cout << "Variable assignment with multiple values" << value.size() << std::endl;
            exit(1);
        }
    } else {
        errors::CompletionError("Variable not defined", this->source, var_name->meta_data.st_line_no, var_name->meta_data.end_line_no,
                                "Variable `" + var_name->value + "` not defined")
            .raise();
        // errors::UndefinedVariableError(this->source, var_name->meta_data, "Assigning to undefine Variable",
        //                                "Define the variable fist before reassigning")
        //     .raise();
    }
};

std::vector<std::string> splitString(const std::string& input) {
    size_t pos = input.find('.');
    return {input.substr(0, pos), input.substr(pos + 1)};
}

std::tuple<std::vector<llvm::Value*>, std::shared_ptr<enviornment::RecordStructType>> compiler::Compiler::_resolveValue(
    std::shared_ptr<AST::Node> node) {
    std::cout << "Resolving value for node type: " << *AST::nodeTypeToString(node->type()) << std::endl;
    switch(node->type()) {
    case AST::NodeType::IntegerLiteral: {
        std::cout << "Node is IntegerLiteral" << std::endl;
        auto integer_literal = std::static_pointer_cast<AST::IntegerLiteral>(node);
        auto value = llvm::ConstantInt::get(llvm_context, llvm::APInt(64, integer_literal->value, true));
        return {{value}, this->enviornment.get_struct("int")};
    }
    case AST::NodeType::FloatLiteral: {
        std::cout << "Node is FloatLiteral" << std::endl;
        auto float_literal = std::static_pointer_cast<AST::FloatLiteral>(node);
        auto value = llvm::ConstantFP::get(llvm_context, llvm::APFloat(float_literal->value));
        return {{value}, this->enviornment.get_struct("float")};
    }
    case AST::NodeType::StringLiteral: {
        std::cout << "Node is StringLiteral" << std::endl;
        auto string_literal = std::static_pointer_cast<AST::StringLiteral>(node);
        auto value = this->llvm_ir_builder.CreateGlobalStringPtr(string_literal->value);
        return {{value}, this->enviornment.get_struct("str")};
    }
    case AST::NodeType::IdentifierLiteral: {
        std::cout << "Node is IdentifierLiteral" << std::endl;
        auto identifier_literal = std::static_pointer_cast<AST::IdentifierLiteral>(node);
        auto var_name = splitString(std::static_pointer_cast<AST::IdentifierLiteral>(node)->value);
        std::shared_ptr<enviornment::RecordStructType> currentStructType = nullptr;
        llvm::Value* alloca = nullptr;
        for (auto type : var_name) {
            if (currentStructType == nullptr) {
                if(!this->enviornment.is_variable(type)) {
                    std::cout << "Variable not defined: " << type << std::endl;
                    errors::CompletionError("Variable not defined", this->source, identifier_literal->meta_data.st_line_no,
                                            identifier_literal->meta_data.end_line_no, "Variable `" + identifier_literal->value + "` not defined")
                        .raise();
                    return {{nullptr}, nullptr};
                }
                currentStructType = this->enviornment.get_variable(type)->struct_type;
                alloca = this->enviornment.get_variable(type)->allocainst;
            }
            else {
                int x = 0;
                for (auto field: currentStructType->fields) {
                    if (field == type) {
                        std::cout << "Accessing field: " << field << " in struct: " << currentStructType->name << std::endl;
                        std::cout << "currentStructType->stand_alone_type: " << (currentStructType->struct_type != nullptr) << std::endl;
                        std::cout << "Alloca: " << alloca << std::endl;
                        std::cout << "x: " << x << std::endl;
                        alloca = this->llvm_ir_builder.CreateStructGEP(currentStructType->struct_type, alloca, x);
                        std::cout << "Alloca: " << alloca << std::endl;
                        currentStructType = currentStructType->sub_types[field];
                        std::cout << "Current struct type: " << currentStructType->name << std::endl;
                        break;
                    }
                    x++;
                }
            }
        }
        if (alloca) {
            auto loadedValue = this->llvm_ir_builder.CreateLoad(currentStructType->stand_alone_type, alloca);
            return {{loadedValue}, currentStructType};
        }
        return {{nullptr}, nullptr};
    }
    case AST::NodeType::InfixedExpression: {
        std::cout << "Node is InfixedExpression" << std::endl;
        return this->_visitInfixExpression(std::static_pointer_cast<AST::InfixExpression>(node));
    }
    case AST::NodeType::CallExpression: {
        std::cout << "Node is CallExpression" << std::endl;
        return this->_visitCallExpression(std::static_pointer_cast<AST::CallExpression>(node));
    }
    case AST::NodeType::BooleanLiteral: {
        std::cout << "Node is BooleanLiteral" << std::endl;
        auto boolean_literal = std::static_pointer_cast<AST::BooleanLiteral>(node);
        auto value = boolean_literal->value ? this->enviornment.get_variable("True")->value : this->enviornment.get_variable("False")->value;
        return {{value}, this->enviornment.get_struct("bool")};
    }
    default: {
        std::cout << "Compiling unknown node type" << std::endl;
        this->compile(node);
        return {{}, nullptr};
    }
    }
};

void compiler::Compiler::_visitReturnStatement(std::shared_ptr<AST::ReturnStatement> return_statement) {
    auto value = return_statement->value;
    auto [return_value, return_type] = this->_resolveValue(value);
    if(return_value.size() != 1) {
        // errors::InternalCompilationError("Return statement with multiple values", this->source, return_statement->meta_data.st_line_no,
        //                                  return_statement->meta_data.end_line_no, "Return statement with multiple values")
        //     .raise();
        std::cout << "Return statement with multiple values" << std::endl;
        exit(1);
    }
    this->llvm_ir_builder.CreateRet(return_value[0]);
};

void compiler::Compiler::_visitFunctionDeclarationStatement(std::shared_ptr<AST::FunctionStatement> function_declaration_statement) {
    auto name = std::static_pointer_cast<AST::IdentifierLiteral>(function_declaration_statement->name)->value;
    auto body = function_declaration_statement->body;
    auto params = function_declaration_statement->parameters;
    std::vector<std::string> param_name;
    std::vector<llvm::Type*> param_types;
    std::vector<std::shared_ptr<enviornment::RecordStructType>> param_types_record;
    for(auto param : params) {
        param_name.push_back(std::static_pointer_cast<AST::IdentifierLiteral>(param->name)->value);
        auto param_type = this->enviornment
                              .get_struct(
                                  std::static_pointer_cast<AST::IdentifierLiteral>(std::static_pointer_cast<AST::GenericType>(param->value_type)->name)->value);
        param_types.push_back(param_type->stand_alone_type ? param_type->stand_alone_type : param_type->struct_type);
        param_types_record.push_back(this->enviornment.get_struct(
            std::static_pointer_cast<AST::IdentifierLiteral>(std::static_pointer_cast<AST::GenericType>(param->value_type)->name)->value));
    }
    auto return_type = this->enviornment.get_struct(std::static_pointer_cast<AST::IdentifierLiteral>(
                                                       std::static_pointer_cast<AST::GenericType>(function_declaration_statement->return_type)->name)
                                                       ->value);
    auto llvm_return_type = return_type->stand_alone_type ? return_type->stand_alone_type : return_type->struct_type;
    // auto return_type_record =
    //     *this->enviornment.get_class(std::static_pointer_cast<AST::IdentifierLiteral>(function_declaration_statement->return_type)->value);
    auto func_type = llvm::FunctionType::get(llvm_return_type, param_types, false);
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
    for(const auto& [arg, param_type_record] : llvm::zip(func->args(), param_types_record)) {
        auto alloca = this->llvm_ir_builder.CreateAlloca(arg.getType(), nullptr, arg.getName());
        this->llvm_ir_builder.CreateStore(&arg, alloca);
        auto record = std::make_shared<enviornment::RecordVariable>(std::string(arg.getName()), &arg, arg.getType(), alloca, param_type_record);
        arguments.push_back(std::make_tuple(std::string(arg.getName()), record));
        this->enviornment.add(record);
    }
    auto func_record = std::make_shared<enviornment::RecordFunction>(name, func, func_type, arguments, return_type);
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

std::tuple<std::vector<llvm::Value*>, std::shared_ptr<enviornment::RecordStructType>> compiler::Compiler::_visitCallExpression(
    std::shared_ptr<AST::CallExpression> call_expression) {
    auto name = std::static_pointer_cast<AST::IdentifierLiteral>(call_expression->name)->value;
    auto param = call_expression->arguments;
    auto args = std::vector<llvm::Value*>();
    for(auto arg : param) {
        auto [value, type] = this->_resolveValue(arg);
        args.push_back(value[0]);
    }
    if(this->enviornment.is_function(name)) {
        auto func_record = this->enviornment.get_function(name);
        auto returnValue = this->llvm_ir_builder.CreateCall(
            llvm::cast<llvm::Function>(func_record->function), args,
            func_record->function_type->getReturnType() != this->enviornment.get_struct("void")->stand_alone_type ? "calltmp" : "");
        return {{returnValue}, func_record->return_type};
    }
    else if (this->enviornment.is_struct(name)) {
        auto struct_record = this->enviornment.get_struct(name);
        auto struct_type = struct_record->struct_type;
        auto alloca = this->llvm_ir_builder.CreateAlloca(struct_type, nullptr, name);
        for (unsigned i = 0; i < args.size(); ++i) {
            auto field_ptr = this->llvm_ir_builder.CreateStructGEP(struct_type, alloca, i);
            this->llvm_ir_builder.CreateStore(args[i], field_ptr);
        }
        return {{alloca}, struct_record};
    }
    errors::CompletionError("Function not defined", this->source, call_expression->meta_data.st_line_no, call_expression->meta_data.end_line_no,
                            "Function `" + name + "` not defined")
        .raise();
    return {{nullptr}, nullptr};
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
        this->llvm_ir_builder.CreateCondBr(condition_val[0], ThenBB, ContBB);
        this->llvm_ir_builder.SetInsertPoint(ThenBB);
        this->compile(consequence);
        this->llvm_ir_builder.CreateBr(ContBB);
        this->llvm_ir_builder.SetInsertPoint(ContBB);
    } else {
        auto func = this->llvm_ir_builder.GetInsertBlock()->getParent();
        llvm::BasicBlock* ThenBB = llvm::BasicBlock::Create(llvm_context, "then", func);
        llvm::BasicBlock* ElseBB = llvm::BasicBlock::Create(llvm_context, "else", func);
        llvm::BasicBlock* ContBB = llvm::BasicBlock::Create(llvm_context, "cont", func);
        this->llvm_ir_builder.CreateCondBr(condition_val[0], ThenBB, ElseBB);
        this->llvm_ir_builder.SetInsertPoint(ThenBB);
        this->compile(consequence);
        this->llvm_ir_builder.CreateBr(ContBB);
        this->llvm_ir_builder.SetInsertPoint(ElseBB);
        this->compile(alternative);
        this->llvm_ir_builder.CreateBr(ContBB);
        this->llvm_ir_builder.SetInsertPoint(ContBB);
    }
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
    this->llvm_ir_builder.CreateCondBr(condition_val[0], BodyBB, ContBB);
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

void compiler::Compiler::_visitStructStatement(std::shared_ptr<AST::StructStatement> struct_statement) {
    std::cout << "Entering Struct Statement" << std::endl;
    std::string struct_name = std::static_pointer_cast<AST::IdentifierLiteral>(struct_statement->name)->value;
    std::cout << "Struct name: " << struct_name << std::endl;
    std::vector<llvm::Type*> field_types;
    auto fields = struct_statement->fields;
    std::cout << "Number of fields: " << fields.size() << std::endl;
    auto struct_record = std::make_shared<enviornment::RecordStructType>(struct_name);
    for(auto field : fields) {
        auto field_decl = std::static_pointer_cast<AST::VariableDeclarationStatement>(field);
        std::string field_name = std::static_pointer_cast<AST::IdentifierLiteral>(field_decl->name)->value;
        std::string field_type_name = std::static_pointer_cast<AST::IdentifierLiteral>(std::static_pointer_cast<AST::GenericType>(field_decl->value_type)->name)->value;
        std::cout << "Processing field: " << field_name << std::endl;
        struct_record->fields.push_back(field_name);
        auto field_type = this->enviornment.get_struct(field_type_name);
        if(field_type->stand_alone_type == nullptr) {
            std::cout << "Field type is a struct type" << std::endl;
            field_types.push_back(field_type->struct_type);
        } else {
            std::cout << "Field type is a standalone type" << std::endl;
            field_types.push_back(field_type->stand_alone_type);
        }
        struct_record->sub_types[field_name] = field_type;
    }
    auto struct_type = llvm::StructType::create(this->llvm_context, field_types, struct_name);
    std::cout << "LLVM StructType created" << std::endl;
    struct_type->setBody(field_types);
    struct_record->struct_type = struct_type;
    this->enviornment.add(struct_record);
    std::cout << "Struct added to environment" << std::endl;
    std::cout << "Exiting Struct Statement" << std::endl;
};
