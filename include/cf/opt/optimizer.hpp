#pragma once
#include "cf/ir/ast.hpp"

namespace cf {

/**
 * Pass de otimização simples em AST:
 *  - Constant folding de operações aritméticas com operandos literais
 *  - Simplificações algébricas básicas (+0, *1, *0, **1, **0, -0)
 *
 * Este pass modifica o Program in-place.
 */
class Optimizer {
    public:
        void run(Program& p);

    private:
        // Stmt
        void optimize_stmt(StmtPtr& s);
        void optimize_block(std::vector<StmtPtr>& body);

        // Expr
        ExprPtr optimize_expr(ExprPtr e);
        ExprPtr optimize_binary(ExprBinary* bin);
        ExprPtr optimize_group(ExprGroup* grp);

        // helpers para literais
        static bool int_literal(const Expr* e, int& value);
        static bool int_like_literal(const Expr* e, int& value); // int ou char
    };

} 
