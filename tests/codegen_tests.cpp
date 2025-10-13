#include <cassert>
#include <string>
#include <sstream>
#include "cf/lexer/lexer.hpp"
#include "cf/parser/parser.hpp"
#include "cf/semantic/semantics.hpp"
#include "cf/codegen/codegen.hpp"

using namespace cf;

static std::string gen(const std::string& src){
    Lexer L(src);
    Parser P(std::move(L));
    Program prog = P.parse_program();
    Semantic S; S.analyze(prog);
    Codegen C; return C.emit(prog);
}

int main(){
    // Atribuição simples
    {
        auto asmcode = gen("Inteiro x; x <- 10;");
        assert(asmcode.find("alloc 4 bytes") != std::string::npos);
        assert(asmcode.find("li a0, 10") != std::string::npos);
        assert(asmcode.find("sw a0") != std::string::npos);
    }
    // If/Else + comparação
    {
        auto asmcode = gen("Inteiro x; x <- 1; Se x = 1 { Imprimir(\"ok\\n\"); } Senao { Imprimir(\"no\\n\"); }");
        assert(asmcode.find("beqz a0, .Lelse") != std::string::npos);
        assert(asmcode.find("print_str") != std::string::npos);
    }
    // While com decremento
    {
        auto asmcode = gen("Inteiro n; n <- 2; Enquanto n > 0 { n <- n - 1; }");
        assert(asmcode.find("while.cond") != std::string::npos);
    }
    // For com step
    {
        auto asmcode = gen("Para i em (1, 3, 1) { Imprimir(i); }");
        assert(asmcode.find("for.cond") != std::string::npos);
        assert(asmcode.find("add t0, t0, t2") != std::string::npos);
    }
    return 0;
}
