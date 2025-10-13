#include <cassert>
#include <string>
#include "cf/lexer/lexer.hpp"
#include "cf/lexer/token.hpp"
#include "cf/common/diagnostic.hpp"

using namespace cf;

int main(){
    // 1) Keywords / identifiers / numbers / operators
    {
        std::string src = R"(Inteiro x <- 10; Enquanto x <> 0 { x <- x - 1; } Em Se Senao Para Imprimir Verdade Mentira)";
        Lexer L(src);
        // checa alguns tokens chaves na sequência
        Token t = L.next(); assert(t.kind == TokenKind::KwInteiro);
        t = L.next(); assert(t.kind == TokenKind::Identifier);
        t = L.next(); assert(t.kind == TokenKind::Assign);
        t = L.next(); assert(t.kind == TokenKind::Integer);
        // consome até encontrar alguns marcadores
        bool seenEnquanto=false, seenNe=false, seenLBrace=false, seenKwEm=false, seenSenao=false;
        bool seenPara=false, seenImprimir=false, seenVerdade=false, seenMentira=false;
        for(;;){
            t = L.next();
            if (t.kind == TokenKind::KwEnquanto) seenEnquanto=true;
            if (t.kind == TokenKind::Ne) seenNe=true;
            if (t.kind == TokenKind::LBrace) seenLBrace=true;
            if (t.kind == TokenKind::KwEm) seenKwEm=true;
            if (t.kind == TokenKind::KwSenao) seenSenao=true;
            if (t.kind == TokenKind::KwPara) seenPara=true;
            if (t.kind == TokenKind::KwImprimir) seenImprimir=true;
            if (t.kind == TokenKind::KwVerdade) seenVerdade=true;
            if (t.kind == TokenKind::KwMentira) seenMentira=true;
            if (t.kind == TokenKind::End) break;
        }
        assert(seenEnquanto && seenNe && seenLBrace && seenKwEm && seenSenao && seenPara && seenImprimir && seenVerdade && seenMentira);
    }
    // 2) Strings com escapes
    {
        std::string src = R"("ok\n\"\\")";
        Lexer L(src);
        Token t = L.next();
        assert(t.kind == TokenKind::String);
        assert(L.next().kind == TokenKind::End);
    }
    // 3) Char literals
    {
        std::string src = R"('a' '\n' '\\')";
        Lexer L(src);
        assert(L.next().kind == TokenKind::Char);
        assert(L.next().kind == TokenKind::Char);
        assert(L.next().kind == TokenKind::Char);
        assert(L.next().kind == TokenKind::End);
    }
    // 4) Comentários (linha e bloco)
    {
        std::string src = R"($ linha
$$ bloco $$ 123)";
        Lexer L(src);
        assert(L.next().kind == TokenKind::Integer);
        assert(L.next().kind == TokenKind::End);
    }
    // 5) Erro: string não terminada
    {
        std::string src = R"("unclosed)";
        try{
            Lexer L(src);
            (void)L.next();
            assert(false && "deveria ter lançado exceção");
        } catch (const CompileError&){ /* ok */ }
    }
    // 6) Erro: comentário de bloco não fechado
    {
        std::string src = "$$";
        try{
            Lexer L(src);
            (void)L.next();
            assert(false && "deveria ter lançado exceção");
        } catch (const CompileError&){ /* ok */ }
    }
    return 0;
}
