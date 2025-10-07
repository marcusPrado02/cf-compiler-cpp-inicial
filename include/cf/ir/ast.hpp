#pragma once
#include <memory>
#include <string>
#include <vector>
#include <variant>
#include "cf/lexer/token.hpp"

namespace cf {

struct Expr;
struct Stmt;
using ExprPtr = std::unique_ptr<Expr>;
using StmtPtr = std::unique_ptr<Stmt>;

enum class BinOp { Add, Sub, Mul, Div, Mod, Pow, And, Or, Eq, Ne, Gt, Lt, Ge, Le };
enum class UnOp { Pos, Neg };

struct Expr {
    struct IntLit { long value; };
    struct CharLit { char value; };
    struct BoolLit { bool value; };
    struct StringLit { std::string value; };
    struct Ident { std::string name; };
    struct Unary { UnOp op; ExprPtr rhs; };
    struct Binary { BinOp op; ExprPtr lhs; ExprPtr rhs; };
    struct Paren { ExprPtr inner; };
    Position pos{};
    std::variant<IntLit, CharLit, BoolLit, StringLit, Ident, Unary, Binary, Paren> node;
};

struct Stmt {
    struct VarDeclItem { std::string name; std::unique_ptr<Expr> init; };
    enum class Type { Inteiro, Logico, Caractere };
    struct VarDecl { Type type; std::vector<VarDeclItem> items; };
    struct Block { std::vector<StmtPtr> items; };
    struct Assign { std::string name; ExprPtr value; };
    struct While { ExprPtr cond; std::unique_ptr<Block> body; };
    struct If { ExprPtr cond; std::unique_ptr<Block> thenB; std::unique_ptr<Block> elseB; };
    struct For { std::string var; ExprPtr begin; ExprPtr end; ExprPtr step; std::unique_ptr<Block> body; };
    struct Print { std::vector<ExprPtr> args; };
    Position pos{};
    std::variant<VarDecl, Block, Assign, While, If, For, Print> node;
};

struct Program {
    std::vector<StmtPtr> items;
};

}
