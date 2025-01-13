#include "../lexer/lexer.hpp"
#include "../parser/parser.hpp"
#include "compiler.hpp"
#include <llvm/IR/DerivedTypes.h>

void compiler::Compiler::initilizeArray() {
    Lexer lexer_instance(
R"(
@generic(T: Any)
struct array {
    data: raw_array[T];
    len: int;
    def __index__(self: array[T], idx: int) -> T {
        if (idx < self.len) {
            return self.data[idx];
        }
        puts("Fuck You, Index out of range");
        exit(1);
    }
};
)",
this->file_path);
    auto parser_instance = parser::Parser(&lexer_instance);
    auto ast_instance = parser_instance.parseProgram();
    this->compile(ast_instance->statements[0]);
    // delete ast_instance;
    // TODO: Fix Memory Error, Cant Delete AST cs its 1 node is in the Struc type;
    this->ll_array = llvm::StructType::create(this->llvm_context, {this->ll_pointer, this->ll_int}, "array");
};