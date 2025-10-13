#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include "cf/ir/ast.hpp"

namespace cf {

    struct Symbol {
        CfType type{CfType::Desconhecido};
    };

    class ScopeStack {
    public:
        /**
         * Entra em um novo escopo.
         */
        void enter() { scopes_.emplace_back(); }
        /**
         * Sai do escopo atual.
         */
        void leave() { if (!scopes_.empty()) scopes_.pop_back(); }

        /**
         * Declara no escopo atual; falha se já existe no MESMO escopo
         */
        bool declare(const std::string& name, CfType ty) {
            if (scopes_.empty()) enter();
            auto& top = scopes_.back();
            if (top.count(name)) return false;
            top[name] = Symbol{ty};
            return true;
        }

        /**
         * Procura do topo para baixo
         */
        std::optional<Symbol> find(const std::string& name) const {
            for (int i = (int)scopes_.size()-1; i >= 0; --i) {
                auto it = scopes_[i].find(name);
                if (it != scopes_[i].end()) return it->second;
            }
            return std::nullopt;
        }

    private:
        std::vector<std::unordered_map<std::string, Symbol>> scopes_;
    };

} 
