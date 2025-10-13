#pragma once
#include <iostream>
#include "cf/ir/ast.hpp"

namespace cf {

struct AstDump {
    /**
     * ostream onde o dump será escrito.
     */
    std::ostream& os;
    /**
     * Nível de indentação atual (número de espaços = 2 * indent).
     */
    int indent{0};

    /**
     * Padroniza indentação no stream.
     */
    void pad(){ for(int i=0;i<indent;i++) os << "  "; }

    /**
     * Dump do AST para o stream dado.
     * 
     * Cada nó é impresso em uma linha, com indentação representando a hierarquia.
     * Exemplo:
     * (Program
     *   (Decl 0 x)
     *   (Assign x (Int 10))
     *   (Print
     *     (Id x)
     *     (Str "hello"))
     */
    void dump(const Program& p){
        os << "(Program\n";
        indent++;
        for (auto const& s : p.items) dump(*s);
        indent--;
        os << ")\n";
    }

    /**
     * Dump de Stmt.
     * 
     * Cada tipo de Stmt é tratado via dynamic_cast.
     */
    void dump(const Stmt& s){
        if (auto d = dynamic_cast<const StmtDecl*>(&s)) {
            pad(); os << "(Decl " << (int)d->type << " " << d->name << ")\n";
        } else if (auto a = dynamic_cast<const StmtAssign*>(&s)) {
            pad(); os << "(Assign " << a->name << " ";
            indent++; dump(*a->value); indent--; os << ")\n";
        } else if (auto pr = dynamic_cast<const StmtPrint*>(&s)) {
            pad(); os << "(Print";
            indent++;
            for (auto const& e : pr->args) { os << "\n"; pad(); dump(*e); }
            indent--;
            os << ")\n";
        } else if (auto w = dynamic_cast<const StmtWhile*>(&s)) {
            pad(); os << "(While\n"; indent++; pad(); os << "cond:\n"; indent++; dump(*w->cond); indent--;
            pad(); os << "body:\n"; indent++; for (auto const& st : w->body) dump(*st); indent-=2; pad(); os << ")\n";
        } else if (auto i = dynamic_cast<const StmtIf*>(&s)) {
            pad(); os << "(If\n";
            indent++; pad(); os << "cond:\n"; indent++; dump(*i->cond); indent--;
            pad(); os << "then:\n"; indent++; for (auto const& st : i->then_body) dump(*st); indent--;
            if (!i->else_body.empty()) { pad(); os << "else:\n"; indent++; for (auto const& st : i->else_body) dump(*st); indent--; }
            indent--; pad(); os << ")\n";
        } else if (auto f = dynamic_cast<const StmtFor*>(&s)) {
            pad(); os << "(For var=" << f->var << "\n";
            indent++; pad(); os << "begin:\n"; indent++; dump(*f->begin); indent--;
            pad(); os << "end:\n"; indent++; dump(*f->end); indent--;
            if (f->step) { pad(); os << "step:\n"; indent++; dump(**f->step); indent--; }
            pad(); os << "body:\n"; indent++; for (auto const& st : f->body) dump(*st); indent-=2; pad(); os << ")\n";
        } else if (auto b = dynamic_cast<const StmtBlock*>(&s)) {
            pad(); os << "(Block\n"; indent++; for (auto const& st : b->body) dump(*st); indent--; pad(); os << ")\n";
        } else {
            pad(); os << "(Stmt?)\n";
        }
    }

    /**
     * Dump de Expr.
     * 
     * Cada tipo de Expr é tratado via dynamic_cast.
     */
    void dump(const Expr& e){
        if (auto i = dynamic_cast<const ExprInteger*>(&e)) {
            os << "(Int " << i->digits << ")";
        } else if (auto c = dynamic_cast<const ExprChar*>(&e)) {
            os << "(Char '" << c->content << "')";
        } else if (auto s = dynamic_cast<const ExprString*>(&e)) {
            os << "(Str \"" << s->content << "\")";
        } else if (auto id = dynamic_cast<const ExprIdent*>(&e)) {
            os << "(Id " << id->name << ")";
        } else if (auto g = dynamic_cast<const ExprGroup*>(&e)) {
            os << "(Group "; dump(*g->inner); os << ")";
        } else if (auto b = dynamic_cast<const ExprBinary*>(&e)) {
            os << "(Bin ";
            switch (b->op) {
                case BinOp::Add: os << "+"; break; case BinOp::Sub: os << "-"; break;
                case BinOp::Mul: os << "*"; break; case BinOp::Div: os << "/"; break; case BinOp::Mod: os << "%"; break;
                case BinOp::Pow: os << "**"; break;
                case BinOp::Eq: os << "="; break; case BinOp::Ne: os << "<>"; break;
                case BinOp::Gt: os << ">"; break; case BinOp::Lt: os << "<"; break;
                case BinOp::Ge: os << ">="; break; case BinOp::Le: os << "<="; break;
                case BinOp::And: os << "&"; break; case BinOp::Or: os << "^"; break;
            }
            os << " "; dump(*b->lhs); os << " "; dump(*b->rhs); os << ")";
        } else {
            os << "(Expr?)";
        }
    }
};

}
