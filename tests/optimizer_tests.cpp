#include <cassert>
#include <string>
#include <sstream>
#include "cf/lexer/lexer.hpp"
#include "cf/parser/parser.hpp"
#include "cf/semantic/semantics.hpp"
#include "cf/opt/optimizer.hpp"
#include "cf/parser/ast_dump.hpp"

using namespace cf;

static std::string dump_after_opt(const std::string& src) {
    Lexer L(src);
    Parser P(std::move(L));
    Program prog = P.parse_program();
    Semantic S; 
    S.analyze(prog);      // garante que a AST é bem-tipada
    Optimizer O;
    O.run(prog);          // otimiza
    S.analyze(prog);      // reanota tipos
    std::ostringstream os;
    AstDump{os}.dump(prog);
    return os.str();
}

int main() {
    // 1) Constant folding simples: 2 + 3 * 4 = 14
    {
        std::string src = "Inteiro x; x <- 2 + 3 * 4;";
        std::string d = dump_after_opt(src);
        // Esperamos algo como: (Assign x (Int 14))
        assert(d.find("(Assign x (Int 14))") != std::string::npos);
    }

    // 2) x + 0 -> x
    {
        std::string src = "Inteiro x; x <- x + 0;";
        std::string d = dump_after_opt(src);
        // Deve virar (Assign x (Id x))
        assert(d.find("(Assign x (Id x))") != std::string::npos);
    }

    // 3) 0 + x -> x
    {
        std::string src = "Inteiro x; x <- 0 + x;";
        std::string d = dump_after_opt(src);
        assert(d.find("(Assign x (Id x))") != std::string::npos);
    }

    // 4) x * 1 -> x e x * 0 -> 0
    {
        std::string src = "Inteiro x; x <- x * 1;";
        std::string d = dump_after_opt(src);
        assert(d.find("(Assign x (Id x))") != std::string::npos);

        src = "Inteiro y; y <- y * 0;";
        d = dump_after_opt(src);
        // y <- 0
        assert(d.find("(Assign y (Int 0))") != std::string::npos);
    }

    // 5) potencia: 2 ** 3 -> 8, x ** 1 -> x, x ** 0 -> 1
    {
        std::string src = "Inteiro a; a <- 2 ** 3;";
        std::string d = dump_after_opt(src);
        assert(d.find("(Assign a (Int 8))") != std::string::npos);

        src = "Inteiro b; b <- b ** 1;";
        d = dump_after_opt(src);
        assert(d.find("(Assign b (Id b))") != std::string::npos);

        src = "Inteiro c; c <- c ** 0;";
        d = dump_after_opt(src);
        assert(d.find("(Assign c (Int 1))") != std::string::npos);
    }

    // 6) Expressao dentro de While / If também é otimizada
    {
        std::string src =
            "Inteiro x; x <- 1 + 1; "
            "Enquanto x + 0 > 1 { x <- x - 1; }";
        std::string d = dump_after_opt(src);
        // x <- 2
        assert(d.find("(Assign x (Int 2))") != std::string::npos);
        // condicao: (Bin > (Id x) (Int 1)) sem +0
        assert(d.find("(Bin > (Id x) (Int 1))") != std::string::npos);
    }

    return 0;
}
