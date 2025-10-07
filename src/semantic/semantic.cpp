#include "cf/semantic/semantic.hpp"
#include "cf/common/diagnostic.hpp"

namespace cf {

void Scope::enter(){ stack_.push_back({}); }
void Scope::leave(){ if (stack_.size()>1) stack_.pop_back(); }
bool Scope::declare(const std::string& name, TypeInfo t){
    auto& top = stack_.back();
    return top.emplace(name, t).second;
}
TypeInfo Scope::lookup(const std::string& name) const{
    for (auto it = stack_.rbegin(); it!=stack_.rend(); ++it){
        auto f = it->find(name);
        if (f != it->end()) return f->second;
    }
    return {};
}

void SemanticAnalyzer::analyze(Program& p){
    scope_.enter();
    for (auto& s : p.items) analyze_stmt(*s);
    scope_.leave();
}

void SemanticAnalyzer::analyze_stmt(Stmt& s){
    // Esqueleto mínimo (futuro: verificação de tipos completa)
    std::visit([&](auto& node){
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, Stmt::VarDecl>){
            for (auto& it : node.items){
                TypeInfo t;
                switch (node.type){
                    case Stmt::Type::Inteiro: t.kind=TypeInfo::Kind::Inteiro; break;
                    case Stmt::Type::Logico: t.kind=TypeInfo::Kind::Logico; break;
                    case Stmt::Type::Caractere: t.kind=TypeInfo::Kind::Caractere; break;
                }
                scope_.declare(it.name, t);
            }
        } else if constexpr (std::is_same_v<T, Stmt::Block>){
            scope_.enter();
            for (auto& item : node.items) analyze_stmt(*item);
            scope_.leave();
        } else if constexpr (std::is_same_v<T, Stmt::Assign>){
            (void)scope_.lookup(node.name); // TODO: erro se não declarado
            (void)analyze_expr(*node.value);
        } else if constexpr (std::is_same_v<T, Stmt::While>){
            (void)analyze_expr(*node.cond);
            for (auto& item : node.body->items) analyze_stmt(*item);
        } else if constexpr (std::is_same_v<T, Stmt::If>){
            (void)analyze_expr(*node.cond);
            for (auto& item : node.thenB->items) analyze_stmt(*item);
            for (auto& item : node.elseB->items) analyze_stmt(*item);
        } else if constexpr (std::is_same_v<T, Stmt::For>){
            scope_.enter();
            scope_.declare(node.var, TypeInfo{TypeInfo::Kind::Inteiro});
            (void)analyze_expr(*node.begin);
            (void)analyze_expr(*node.end);
            if (node.step) (void)analyze_expr(*node.step);
            for (auto& item : node.body->items) analyze_stmt(*item);
            scope_.leave();
        } else if constexpr (std::is_same_v<T, Stmt::Print>){
            for (auto& a : node.args) (void)analyze_expr(*a);
        }
    }, s.node);
}

TypeInfo SemanticAnalyzer::analyze_expr(Expr& e){
    return std::visit([&](auto& node)->TypeInfo{
        using T = std::decay_t<decltype(node)>;
        if constexpr (std::is_same_v<T, Expr::IntLit>) return {TypeInfo::Kind::Inteiro};
        if constexpr (std::is_same_v<T, Expr::CharLit>) return {TypeInfo::Kind::Caractere};
        if constexpr (std::is_same_v<T, Expr::BoolLit>) return {TypeInfo::Kind::Logico};
        if constexpr (std::is_same_v<T, Expr::StringLit>) return {TypeInfo::Kind::String};
        if constexpr (std::is_same_v<T, Expr::Ident>) return scope_.lookup(node.name);
        if constexpr (std::is_same_v<T, Expr::Unary>) return analyze_expr(*node.rhs);
        if constexpr (std::is_same_v<T, Expr::Binary>){
            auto lt = analyze_expr(*node.lhs);
            auto rt = analyze_expr(*node.rhs);
            return lt; // TODO: regras de tipos por operador
        }
        if constexpr (std::is_same_v<T, Expr::Paren>) return analyze_expr(*node.inner);
        return {};
    }, e.node);
}

}
