#pragma once
#include "cf/lexer/lexer.hpp"
#include "cf/ir/ast.hpp"

namespace cf {

class Parser {
public:
    explicit Parser(Lexer lex);
    Program parse();
private:
    // helpers
    const Token& tok();
    const Token& eat(TokenKind k, const char* what);
    bool accept(TokenKind k);
    bool accept2(TokenKind a, TokenKind b);
    bool at(TokenKind k) const;
    void next();
    // grammar
    Program parse_program();
    std::unique_ptr<Stmt> parse_decl_or_cmd();
    std::unique_ptr<Stmt> parse_declvar();
    Stmt::Type parse_type();
    std::unique_ptr<Stmt> parse_cmd();
    std::unique_ptr<Stmt> parse_block();
    std::unique_ptr<Stmt> parse_assign_stmt();
    std::unique_ptr<Stmt> parse_while();
    std::unique_ptr<Stmt> parse_if();
    std::unique_ptr<Stmt> parse_for();
    std::unique_ptr<Stmt> parse_print();

    // expressions (precedência)
    ExprPtr parse_expr();      // Or
    ExprPtr parse_or();
    ExprPtr parse_and();
    ExprPtr parse_rel();
    ExprPtr parse_add();
    ExprPtr parse_mul();
    ExprPtr parse_pow();
    ExprPtr parse_unary();
    ExprPtr parse_primary();

    Lexer lex_;
    Token t_;
};

}
