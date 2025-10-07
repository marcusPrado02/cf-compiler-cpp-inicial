#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include "cf/ir/ast.hpp"

namespace cf {

struct TypeInfo {
    enum class Kind { Inteiro, Logico, Caractere, String, Invalid } kind{Kind::Invalid};
};

class Scope {
public:
    void enter();
    void leave();
    bool declare(const std::string& name, TypeInfo t);
    TypeInfo lookup(const std::string& name) const;
private:
    std::vector<std::unordered_map<std::string, TypeInfo>> stack_{{}};
};

class SemanticAnalyzer {
public:
    void analyze(Program& p);
private:
    void analyze_stmt(Stmt& s);
    TypeInfo analyze_expr(Expr& e);
    Scope scope_;
};

}
