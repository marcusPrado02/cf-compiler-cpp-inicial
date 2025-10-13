#include <cassert>
#include <string>
#include "cf/lexer/lexer.hpp"
#include "cf/parser/parser.hpp"
#include "cf/semantic/semantics.hpp"
#include "cf/common/diagnostic.hpp"

using namespace cf;

static void run_ok(const std::string& src){
    Lexer L(src);
    Parser P(std::move(L));
    Program prog = P.parse_program();
    Semantic S; S.analyze(prog);
}

static bool run_err(const std::string& src){
    try {
        Lexer L(src);
        Parser P(std::move(L));
        Program prog = P.parse_program();
        Semantic S; S.analyze(prog);
        return false;
    } catch (const CompileError&) { return true; }
}

int main(){
    // OK: declara, usa, imprime
    run_ok("Inteiro x; x <- 1 + 2; Imprimir(x);");

    // OK: char promovendo para int em aritmético e relacional
    run_ok("Caractere c; Inteiro x; x <- c + 1; Se x > c { Imprimir(x); } Senao { Imprimir('a'); }");

    // Erro: variável não declarada
    assert(run_err("x <- 1;"));

    // Erro: redeclaração no mesmo escopo
    assert(run_err("Inteiro x; Inteiro x;"));

    // Erro: atribuição incompatível (Logico <- Int)
    assert(run_err("Logico b; b <- 1;"));

    // Erro: operação lógica com não-lógicos
    assert(run_err("Inteiro a; Inteiro b; Se a & b { Imprimir(a); }"));

    // Erro: condição de Se não-lógica
    assert(run_err("Inteiro a; Se a { Imprimir(a); }"));

    // OK: While + bloco
    run_ok("Inteiro n; n <- 3; Enquanto n > 0 { n <- n - 1; }");

    // For: begin/end/step Int e var local do laço
    run_ok("Para i em (1, 5, 2) { Imprimir(i); }");

    // Erro: step não-inteiro
    assert(run_err("Para i em (1, 5, 'a') { Imprimir(i); }"));

    return 0;
}
