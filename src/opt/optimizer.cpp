#include "cf/opt/optimizer.hpp"
#include <cstdlib>
#include <sstream>

namespace cf {
    // ---------- helpers de literal ----------

    // ExprInteger: converte digits -> int
    bool Optimizer::int_literal(const Expr* e, int& value) {
        if (auto i = dynamic_cast<const ExprInteger*>(e)) {
            try {
                value = std::stoi(i->digits);
                return true;
            } catch (...) {
                return false;
            }
        }
        return false;
    }

    // Inteiro ou Caractere tratado como inteiro
    bool Optimizer::int_like_literal(const Expr* e, int& value) {
        if (auto i = dynamic_cast<const ExprInteger*>(e)) {
            try {
                value = std::stoi(i->digits);
                return true;
            } catch (...) {
                return false;
            }
        }
        if (auto c = dynamic_cast<const ExprChar*>(e)) {
            // content tem 1 char ou escape simples
            if (c->content.size() == 1) {
                value = static_cast<unsigned char>(c->content[0]);
                return true;
            }
            if (c->content.size() == 2 && c->content[0] == '\\') {
                switch (c->content[1]) {
                    case 'n': value = 10; return true;
                    case 't': value = 9;  return true;
                    case '\\': value = '\\'; return true;
                    case '\'': value = '\''; return true;
                    case '\"': value = '\"'; return true;
                    default:
                        value = static_cast<unsigned char>(c->content[1]);
                        return true;
                }
            }
        }
        return false;
    }

    // ---------- expr ----------

    ExprPtr Optimizer::optimize_group(ExprGroup* grp) {
        grp->inner = optimize_expr(std::move(grp->inner));
        // Se o inner virou literal/ident/binário já simples, podemos opcionalmente
        // remover o Group, mas para manter a expressividade, só removemos quando
        // é literal ou identificador.
        if (dynamic_cast<ExprInteger*>(grp->inner.get()) ||
            dynamic_cast<ExprChar*>(grp->inner.get()) ||
            dynamic_cast<ExprString*>(grp->inner.get()) ||
            dynamic_cast<ExprIdent*>(grp->inner.get())) {
            return std::move(grp->inner);
        }
        return ExprPtr(grp);
    }

    ExprPtr Optimizer::optimize_binary(ExprBinary* bin) {
        // Primeiro otimiza recursivamente os filhos
        bin->lhs = optimize_expr(std::move(bin->lhs));
        bin->rhs = optimize_expr(std::move(bin->rhs));

        int lv = 0, rv = 0;
        bool l_is_int = int_like_literal(bin->lhs.get(), lv);
        bool r_is_int = int_like_literal(bin->rhs.get(), rv);

        // Constant folding aritmético: se ambos são int-like
        if (l_is_int && r_is_int) {
            bool folded = false;
            int result = 0;

            switch (bin->op) {
                case BinOp::Add: result = lv + rv; folded = true; break;
                case BinOp::Sub: result = lv - rv; folded = true; break;
                case BinOp::Mul: result = lv * rv; folded = true; break;
                case BinOp::Div:
                    if (rv != 0) { result = lv / rv; folded = true; }
                    break;
                case BinOp::Mod:
                    if (rv != 0) { result = lv % rv; folded = true; }
                    break;
                case BinOp::Pow: {
                    if (rv >= 0) {
                        int acc = 1;
                        for (int i = 0; i < rv; ++i) acc *= lv;
                        result = acc;
                        folded = true;
                    }
                    break;
                }
                // Relacionais e lógicos NÃO dobramos aqui para não quebrar CfType::Logico
                default:
                    break;
            }

            if (folded) {
                auto lit = std::make_unique<ExprInteger>();
                lit->pos = bin->pos;
                lit->digits = std::to_string(result);
                return lit;
            }
        }

        // Simplificações algébricas básicas quando pelo menos um é literal inteiro
        if (bin->op == BinOp::Add) {
            // x + 0 -> x ; 0 + x -> x
            if (r_is_int && rv == 0) return std::move(bin->lhs);
            if (l_is_int && lv == 0) return std::move(bin->rhs);
        }
        if (bin->op == BinOp::Sub) {
            // x - 0 -> x
            if (r_is_int && rv == 0) return std::move(bin->lhs);
        }
        if (bin->op == BinOp::Mul) {
            // x * 0 -> 0 ; 0 * x -> 0
            if (r_is_int && rv == 0) {
                auto lit = std::make_unique<ExprInteger>();
                lit->pos = bin->pos;
                lit->digits = "0";
                return lit;
            }
            if (l_is_int && lv == 0) {
                auto lit = std::make_unique<ExprInteger>();
                lit->pos = bin->pos;
                lit->digits = "0";
                return lit;
            }
            // x * 1 -> x ; 1 * x -> x
            if (r_is_int && rv == 1) return std::move(bin->lhs);
            if (l_is_int && lv == 1) return std::move(bin->rhs);
        }
        if (bin->op == BinOp::Div) {
            // x / 1 -> x
            if (r_is_int && rv == 1) return std::move(bin->lhs);
        }
        if (bin->op == BinOp::Pow) {
            // x ** 0 -> 1
            if (r_is_int && rv == 0) {
                auto lit = std::make_unique<ExprInteger>();
                lit->pos = bin->pos;
                lit->digits = "1";
                return lit;
            }
            // x ** 1 -> x
            if (r_is_int && rv == 1) return std::move(bin->lhs);
        }

        return ExprPtr(bin);
    }

    ExprPtr Optimizer::optimize_expr(ExprPtr e) {
        if (!e) return nullptr;

        if (auto* bin = dynamic_cast<ExprBinary*>(e.get())) {
            // transfere ownership pra função especializada
            e.release();
            return optimize_binary(bin);
        }
        if (auto* grp = dynamic_cast<ExprGroup*>(e.get())) {
            e.release();
            return optimize_group(grp);
        }

        // Literais, identificadores, strings já são folhas
        return e;
    }

    // ---------- stmts ----------

    void Optimizer::optimize_block(std::vector<StmtPtr>& body) {
        for (auto& st : body) {
            optimize_stmt(st);
        }
    }

    void Optimizer::optimize_stmt(StmtPtr& s) {
        if (!s) return;

        if (auto* d = dynamic_cast<StmtDecl*>(s.get())) {
            (void)d;
            return; // nada a otimizar em declarações simples
        }
        if (auto* a = dynamic_cast<StmtAssign*>(s.get())) {
            a->value = optimize_expr(std::move(a->value));
            return;
        }
        if (auto* p = dynamic_cast<StmtPrint*>(s.get())) {
            for (auto& e : p->args) {
                e = optimize_expr(std::move(e));
            }
            return;
        }
        if (auto* w = dynamic_cast<StmtWhile*>(s.get())) {
            w->cond = optimize_expr(std::move(w->cond));
            optimize_block(w->body);
            return;
        }
        if (auto* i = dynamic_cast<StmtIf*>(s.get())) {
            i->cond = optimize_expr(std::move(i->cond));
            optimize_block(i->then_body);
            optimize_block(i->else_body);
            return;
        }
        if (auto* f = dynamic_cast<StmtFor*>(s.get())) {
            f->begin = optimize_expr(std::move(f->begin));
            f->end   = optimize_expr(std::move(f->end));
            if (f->step.has_value()) {
                f->step = optimize_expr(std::move(*f->step));
            }
            optimize_block(f->body);
            return;
        }
        if (auto* b = dynamic_cast<StmtBlock*>(s.get())) {
            optimize_block(b->body);
            return;
        }
    }

    void Optimizer::run(Program& p) {
        for (auto& st : p.items) {
            optimize_stmt(st);
        }
    }

}
