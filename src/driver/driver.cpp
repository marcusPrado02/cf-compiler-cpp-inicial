#include "cf/driver/driver.hpp"
#include "cf/lexer/lexer.hpp"
#include "cf/parser/parser.hpp"
#include "cf/semantic/semantic.hpp"

namespace cf {

std::string Driver::compile_to_asm(const std::string& source){
    Lexer lex(source);
    Parser parser(std::move(lex));
    Program p = parser.parse();
    SemanticAnalyzer sema;
    sema.analyze(p);
    CodegenRV32I gen;
    return gen.emit(p);
}

}
