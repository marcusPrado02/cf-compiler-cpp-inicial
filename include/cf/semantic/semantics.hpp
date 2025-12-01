#pragma once
#include "cf/ir/ast.hpp"
#include "cf/semantic/scope.hpp"
#include "cf/semantic/types.hpp"
#include "cf/common/diagnostic.hpp"

namespace cf
{

    class Semantic
    {
    public:
        /**
         * Analisa semanticamente o programa.
         */
        void analyze(Program &p); // lança CompileError em caso de erro

    private:
        // Stmts
        void check_stmt(Stmt &s);
        void check_decl(StmtDecl &s);
        void check_assign(StmtAssign &s);
        void check_print(StmtPrint &s);
        void check_if(StmtIf &s);
        void check_while(StmtWhile &s);
        void check_for(StmtFor &s);

        // Exprs
        CfType check_expr(Expr &e);
        CfType check_binary(ExprBinary &e);
        CfType check_group(ExprGroup &e);

        // util
        [[noreturn]] void sem_error(const Position &pos, const std::string &msg);

        bool expr_has_string(Expr &e);

    private:
        /**
         * Pilha de escopos para variáveis (simples, sem funções/métodos ainda).
         */
        ScopeStack scopes_;
    };

}
