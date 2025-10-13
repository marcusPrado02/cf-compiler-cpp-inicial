#include <cassert>
#include <string>
#include <sstream>
#include "cf/lexer/lexer.hpp"
#include "cf/parser/parser.hpp"
#include "cf/parser/ast_dump.hpp"
#include "cf/common/diagnostic.hpp"

using namespace cf;

static std::string dump_ast(const std::string& src){
    Lexer L(src);
    Parser P(std::move(L));
    Program prog = P.parse_program();
    std::ostringstream os; AstDump{os}.dump(prog);
    return os.str();
}

int main(){
    // 1) Declarações + atribuição
    {
        std::string src =
            "Inteiro x; "
            "x <- 10; ";
        auto s = dump_ast(src);
        assert(s.find("(Decl 0 x)") != std::string::npos); // CfType::Inteiro -> 0
        assert(s.find("(Assign x (Int 10))") != std::string::npos);
    }
    // 2) If/Else + comparação + soma
    {
        std::string src =
            "Inteiro x; x <- 1; "
            "Se x = 1 { x <- x + 1; } Senao { x <- x + 2; }";
        auto s = dump_ast(src);
        assert(s.find("(If") != std::string::npos);
        assert(s.find("(Bin = (Id x) (Int 1))") != std::string::npos);
        assert(s.find("(Assign x (Bin + (Id x) (Int 1)))") != std::string::npos);
        assert(s.find("(Assign x (Bin + (Id x) (Int 2)))") != std::string::npos);
    }
    // 3) While e multiplicação
    {
        std::string src =
            "Inteiro n; n <- 3; Enquanto n > 0 { n <- n - 1; }";
        auto s = dump_ast(src);
        assert(s.find("(While") != std::string::npos);
        assert(s.find("(Bin > (Id n) (Int 0))") != std::string::npos);
    }
    // 4) For com step
    {
        std::string src =
            "Para i em (1, 5, 2) { Imprimir(i); }";
        auto s = dump_ast(src);
        assert(s.find("(For var=i") != std::string::npos);
        assert(s.find("step:") != std::string::npos);
    }
    // 5) For sem step
    {
        std::string src =
            "Para i em (1, 3) { Imprimir(i); }";
        auto s = dump_ast(src);
        assert(s.find("(For var=i") != std::string::npos);
        assert(s.find("step:") == std::string::npos); // sem step
    }
    // 6) Pow right-assoc: a ** b ** c == a ** (b ** c)
    {
        std::string src = "Inteiro a; a <- 2 ** 3 ** 4;";
        auto s = dump_ast(src);
        // Estrutura deve conter (Bin ** (Int 2) (Bin ** (Int 3) (Int 4)))
        assert(s.find("(Bin ** (Int 2) (Bin ** (Int 3) (Int 4)))") != std::string::npos);
    }
    // 7) Erro: falta ';' em atribuição
    {
        std::string src = "Inteiro x; x <- 1";
        try {
            dump_ast(src);
            assert(false && "deveria falhar por falta de ';'");
        } catch (const CompileError&) { /* ok */ }
    }
    // 8) Erro: bloco não fechado
    {
        std::string src = "Se 1 { Inteiro a;";
        try {
            dump_ast(src);
            assert(false && "deveria falhar por '}' faltando");
        } catch (const CompileError&) { /* ok */ }
    }

    return 0;
}
