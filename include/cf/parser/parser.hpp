#pragma once
#include <memory>
#include "cf/ir/ast.hpp"
#include "cf/lexer/lexer.hpp"
#include "cf/common/diagnostic.hpp"

namespace cf
{

    class Parser
    {
    public:
        explicit Parser(Lexer lex) : lex_(std::move(lex)) {}
        Program parse_program();

    private:
        // helpers
        const Token &peek() { return lex_.peek(); }
        Token next() { return lex_.next(); }
        bool at(TokenKind k) { return lex_.peek().kind == k; }
        bool eat(TokenKind k, const char *expectMsg = nullptr);
        void expect(TokenKind k, const char *msg);

        // mapeia keyword de tipo para CfType
        static CfType map_type(TokenKind k);

        // não-terminais
        std::vector<StmtPtr> parse_decl_or_stmt();
        std::vector<StmtPtr> parse_declaration();
        StmtPtr parse_statement();
        StmtPtr parse_assign_tail_after_ident(Token identTok);
        std::vector<StmtPtr> parse_block(); // consome { ... }
        StmtPtr parse_if();
        StmtPtr parse_while();
        StmtPtr parse_for();
        StmtPtr parse_print();

        // expressões (precedência)
        ExprPtr parse_expr(); // ExprLogico
        ExprPtr parse_logical();
        ExprPtr parse_rel();
        ExprPtr parse_add();
        ExprPtr parse_mul();
        ExprPtr parse_pow(); // right-assoc
        ExprPtr parse_primary();

        // util para mensagens
        [[noreturn]] void syntax_error(const Token &got, const std::string &msg);
        Position pos_of(const Token &t) const { return t.pos; }

    private:
        /**
         * Lexer usado para obter tokens do fonte.
         */
        Lexer lex_;
    };

}
