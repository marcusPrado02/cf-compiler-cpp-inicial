#pragma once
#include <memory>
#include <string>
#include <vector>
#include <optional>
#include "cf/common/position.hpp"
#include "cf/lexer/token.hpp"

namespace cf {

    // ---- Tipos CF ----
    enum class CfType { Inteiro, Logico, Caractere, Desconhecido };

    // ---- Expressões ----
    struct Expr {
        Position pos{};
        CfType inferred{CfType::Desconhecido}; // <- tipo inferido pelo analisador semântico
        virtual ~Expr() = default;
    };

    /**
     * Usar std::unique_ptr para gerenciar memória automaticamente.
     */
    using ExprPtr = std::unique_ptr<Expr>;

    /**
     * Tipos de expressões:
     * - ExprInteger: números inteiros (ex: 123, 0, -456)
     * - ExprChar: caracteres (ex: 'a', '\n', '\x41')
     * - ExprString: strings (ex: "hello", "line\n")
     * - ExprIdent: identificadores (ex: varName, _temp, contador1)
     * - ExprBinary: expressões binárias (ex: a + b, x * y)
     * - ExprGroup: expressões agrupadas por parênteses (ex: (a + b) * c)
     * - ExprBool: valores booleanos (true, false)
     */
    struct ExprInteger : Expr {
        std::string digits; // como lido
    };

    struct ExprChar : Expr {
        std::string content; // ex: "\\n" ou "a"
    };

    struct ExprString : Expr {
        std::string content; // sem aspas externas, com escapes
    };

    struct ExprIdent : Expr {
        std::string name; // preserva grafia original
    };

    /**
     * Operadores binários suportados.
     */
    enum class BinOp {
        Add, Sub, Mul, Div, Mod, Pow,
        Eq, Ne, Gt, Lt, Ge, Le,
        And, Or
    };

    struct ExprBinary : Expr {
        BinOp op;
        ExprPtr lhs;
        ExprPtr rhs;
    };

    struct ExprGroup : Expr {
        ExprPtr inner;
    };

    struct ExprBool : Expr {
        bool value;
    };


    // ---- Comandos ----
    /**
     * Comandos suportados:
     * - StmtDecl: declaração de variável (ex: inteiro x;)
     * - StmtAssign: atribuição (ex: x = 10;)
     * - StmtPrint: comando de impressão (ex: Imprimir(x, "hello");)
     * - StmtIf: comando condicional (ex: Se (x > 0) { ... } Senão { ... })
     * - StmtWhile: laço de repetição (ex: Enquanto (x < 10) { ... })
     * - StmtFor: laço para (ex: Para i em (1, 10, 2) { ... })
     * - StmtBlock: bloco de comandos (ex: { comando1; comando2; })
     */
    struct Stmt {
        Position pos{};
        virtual ~Stmt() = default;
    };
    using StmtPtr = std::unique_ptr<Stmt>;

    struct StmtDecl : Stmt {
        CfType type{CfType::Desconhecido};
        std::string name;
        ExprPtr init;
    };

    struct StmtAssign : Stmt {
        std::string name;
        ExprPtr value;
    };

    struct StmtPrint : Stmt {
        std::vector<ExprPtr> args; // Imprimir(arg1, arg2, ...)
    };

    struct StmtIf : Stmt {
        ExprPtr cond;
        std::vector<StmtPtr> then_body;
        std::vector<StmtPtr> else_body; // vazio se não houver
    };

    struct StmtWhile : Stmt {
        ExprPtr cond;
        std::vector<StmtPtr> body;
    };

    struct StmtFor : Stmt {
        // Para ident em (begin, end, [step]) { body }
        std::string var;
        ExprPtr begin;
        ExprPtr end;
        std::optional<ExprPtr> step; // opcional
        std::vector<StmtPtr> body;
    };

    struct StmtBlock : Stmt {
        std::vector<StmtPtr> body;
    };

    // ---- Programa ----
    /**
     * Programa consiste em uma lista de declarações e comandos top-level.
     */
    struct Program {
        std::vector<StmtPtr> items; // declarações e comandos top-level
    };

}
