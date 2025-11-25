#include <cassert>
#include <string>
#include <sstream>
#include "cf/lexer/lexer.hpp"
#include "cf/parser/parser.hpp"
#include "cf/semantic/semantics.hpp"
#include "cf/codegen/codegen.hpp"

using namespace cf;

static std::string gen(const std::string &src)
{
    Lexer L(src);
    Parser P(std::move(L));
    Program prog = P.parse_program();
    Semantic S;
    S.analyze(prog);
    Codegen C;
    return C.emit(prog);
}

int main()
{
    // 1) Programa simples com Imprimir string
    {
        auto asmcode = gen("Imprimir(\"Ola\");");

        // Deve ter seção de código e dados
        assert(asmcode.find(".text") != std::string::npos);
        assert(asmcode.find(".data") != std::string::npos);

        // Deve ter string no pool
        assert(asmcode.find("L.str.") != std::string::npos);

        // Deve usar syscall de print_string (MIPS: li $v0, 4 / syscall)
        assert(asmcode.find("li $v0, 4") != std::string::npos);
        assert(asmcode.find("syscall") != std::string::npos);
    }

    // 2) While com decremento
    {
        auto asmcode = gen("Inteiro n; n <- 2; Enquanto n > 0 { n <- n - 1; }");
        // Label de while deve aparecer (L_while.cond_X, mas a substring while.cond é suficiente)
        assert(asmcode.find("while.cond") != std::string::npos);
    }

    // 3) For com step
    {
        auto asmcode = gen("Para i em (1, 3, 1) { Imprimir(i); }");

        // Label de for (L_for.cond_X, substring for.cond basta)
        assert(asmcode.find("for.cond") != std::string::npos);

        // Incremento do laço: i += step  -> add $t0, $t0, $t2
        assert(asmcode.find("add $t0, $t0, $t2") != std::string::npos);
    }

    return 0;
}
