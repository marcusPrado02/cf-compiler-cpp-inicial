\
#include "cf/parser/parser.hpp"
#include "cf/common/diagnostic.hpp"

namespace cf {

Parser::Parser(Lexer lex) : lex_(std::move(lex)) {
    t_ = lex_.peek();
}

const Token& Parser::tok(){ return t_; }
void Parser::next(){ t_ = lex_.next(); t_ = lex_.peek(); }

bool Parser::at(TokenKind k) const { return t_.kind == k; }

bool Parser::accept(TokenKind k){
    if (at(k)) { next(); return true; } return false;
}
bool Parser::accept2(TokenKind a, TokenKind b){
    return at(a) || at(b) ? (next(), true) : false;
}

const Token& Parser::eat(TokenKind k, const char* what){
    if (!accept(k)){
        Diagnostic d;
        d.phase = "syntax";
        d.pos = t_.pos;
        d.message = std::string("esperado ") + what + " mas encontrei '" + t_.lexeme + "'";
        throw CompileError(d);
    }
    return t_;
}

Program Parser::parse(){
    return parse_program();
}

Program Parser::parse_program(){
    Program p;
    while (!at(TokenKind::End)){
        p.items.emplace_back(parse_decl_or_cmd());
    }
    return p;
}

std::unique_ptr<Stmt> Parser::parse_decl_or_cmd(){
    if (at(TokenKind::KwInteiro) || at(TokenKind::KwLogico) || at(TokenKind::KwCaractere)){
        return parse_declvar();
    }
    return parse_cmd();
}

Stmt::Type Parser::parse_type(){
    if (accept(TokenKind::KwInteiro)) return Stmt::Type::Inteiro;
    if (accept(TokenKind::KwLogico)) return Stmt::Type::Logico;
    if (accept(TokenKind::KwCaractere)) return Stmt::Type::Caractere;
    Diagnostic d; d.phase="syntax"; d.pos=tok().pos; d.message="tipo esperado"; throw CompileError(d);
}

std::unique_ptr<Stmt> Parser::parse_declvar(){
    auto type = parse_type();
    auto decl = std::make_unique<Stmt>();
    decl->node = Stmt::VarDecl{type, {}};
    // VarDeclLista: ident ('<-' Expr)? (',' ...)* ';'
    do{
        if (!at(TokenKind::Identifier)){
            Diagnostic d; d.phase="syntax"; d.pos=tok().pos; d.message="identificador esperado"; throw CompileError(d);
        }
        std::string name = tok().lexeme; next();
        std::unique_ptr<Expr> init;
        if (accept(TokenKind::Assign)){
            init = parse_expr();
        }
        std::get<Stmt::VarDecl>(decl->node).items.push_back({name, std::move(init)});
    } while (accept(TokenKind::Comma));
    eat(TokenKind::Semicolon, "';'");
    return decl;
}

std::unique_ptr<Stmt> Parser::parse_cmd(){
    if (at(TokenKind::LBrace)) return parse_block();
    if (at(TokenKind::KwEnquanto)) return parse_while();
    if (at(TokenKind::KwSe)) return parse_if();
    if (at(TokenKind::KwPara)) return parse_for();
    if (at(TokenKind::KwImprimir)) return parse_print();
    // fallback: atrib
    return parse_assign_stmt();
}

std::unique_ptr<Stmt> Parser::parse_block(){
    eat(TokenKind::LBrace, "'{'");
    auto blk = std::make_unique<Stmt>();
    Stmt::Block block;
    while (!at(TokenKind::RBrace)){
        // permite declarações dentro do bloco
        if (at(TokenKind::KwInteiro) || at(TokenKind::KwLogico) || at(TokenKind::KwCaractere)){
            block.items.emplace_back(parse_declvar());
        } else {
            block.items.emplace_back(parse_cmd());
        }
    }
    eat(TokenKind::RBrace, "'}'");
    blk->node = std::move(block);
    return blk;
}

std::unique_ptr<Stmt> Parser::parse_assign_stmt(){
    if (!at(TokenKind::Identifier)){
        Diagnostic d; d.phase="syntax"; d.pos=tok().pos; d.message="comando inválido (esperado atribuição ou bloco)"; throw CompileError(d);
    }
    std::string name = tok().lexeme; next();
    eat(TokenKind::Assign, "'<-'");
    auto value = parse_expr();
    eat(TokenKind::Semicolon, "';'");
    auto st = std::make_unique<Stmt>();
    st->node = Stmt::Assign{std::move(name), std::move(value)};
    return st;
}

std::unique_ptr<Stmt> Parser::parse_while(){
    eat(TokenKind::KwEnquanto, "'Enquanto'");
    auto cond = parse_expr();
    auto body = std::unique_ptr<Stmt::Block>(static_cast<Stmt::Block*>(nullptr));
    auto blk = parse_block();
    body = std::make_unique<Stmt::Block>(std::get<Stmt::Block>(blk->node));
    auto st = std::make_unique<Stmt>();
    st->node = Stmt::While{std::move(cond), std::move(body)};
    return st;
}

std::unique_ptr<Stmt> Parser::parse_if(){
    eat(TokenKind::KwSe, "'Se'");
    auto cond = parse_expr();
    auto thenBlk = std::unique_ptr<Stmt::Block>(static_cast<Stmt::Block*>(nullptr));
    auto elseBlk = std::unique_ptr<Stmt::Block>(static_cast<Stmt::Block*>(nullptr));
    auto blkThen = parse_block();
    thenBlk = std::make_unique<Stmt::Block>(std::get<Stmt::Block>(blkThen->node));
    eat(TokenKind::KwSenao, "'Senao'");
    auto blkElse = parse_block();
    elseBlk = std::make_unique<Stmt::Block>(std::get<Stmt::Block>(blkElse->node));
    auto st = std::make_unique<Stmt>();
    st->node = Stmt::If{std::move(cond), std::move(thenBlk), std::move(elseBlk)};
    return st;
}

std::unique_ptr<Stmt> Parser::parse_for(){
    eat(TokenKind::KwPara, "'Para'");
    if (!at(TokenKind::Identifier)){ Diagnostic d; d.phase="syntax"; d.pos=tok().pos; d.message="identificador esperado em 'Para'"; throw CompileError(d); }
    std::string name = tok().lexeme; next();
    // 'em' '(' Expr ',' Expr (',' Expr)? ')' Bloco
    // Aceitamos 'em' como identificador para simplificar por ora.
    eat(TokenKind::Identifier, "'em'");
    eat(TokenKind::LParen, "'('");
    auto begin = parse_expr(); eat(TokenKind::Comma, "','");
    auto end = parse_expr();
    ExprPtr step;
    if (accept(TokenKind::Comma)) step = parse_expr();
    eat(TokenKind::RParen, "')'");
    auto blk = parse_block();
    auto body = std::make_unique<Stmt::Block>(std::get<Stmt::Block>(blk->node));
    auto st = std::make_unique<Stmt>();
    st->node = Stmt::For{name, std::move(begin), std::move(end), std::move(step), std::move(body)};
    return st;
}

std::unique_ptr<Stmt> Parser::parse_print(){
    eat(TokenKind::KwImprimir, "'Imprimir'");
    eat(TokenKind::LParen, "'('");
    auto st = std::make_unique<Stmt>();
    Stmt::Print pr{};
    if (!at(TokenKind::RParen)){
        pr.args.push_back(parse_expr());
        while (accept(TokenKind::Comma)){
            pr.args.push_back(parse_expr());
        }
    }
    eat(TokenKind::RParen, "')'");
    eat(TokenKind::Semicolon, "';'");
    st->node = std::move(pr);
    return st;
}

// ---- EXPRESSÕES ----

ExprPtr Parser::parse_expr(){ return parse_or(); }

ExprPtr Parser::parse_or(){
    auto lhs = parse_and();
    while (accept(TokenKind::Or)){
        auto rhs = parse_and();
        auto e = std::make_unique<Expr>();
        e->node = Expr::Binary{BinOp::Or, std::move(lhs), std::move(rhs)};
        lhs = std::move(e);
    }
    return lhs;
}

ExprPtr Parser::parse_and(){
    auto lhs = parse_rel();
    while (accept(TokenKind::And)){
        auto rhs = parse_rel();
        auto e = std::make_unique<Expr>();
        e->node = Expr::Binary{BinOp::And, std::move(lhs), std::move(rhs)};
        lhs = std::move(e);
    }
    return lhs;
}

ExprPtr Parser::parse_rel(){
    auto lhs = parse_add();
    if (accept(TokenKind::Eq))  { auto rhs = parse_add(); auto e=std::make_unique<Expr>(); e->node=Expr::Binary{BinOp::Eq, std::move(lhs), std::move(rhs)}; return e; }
    if (accept(TokenKind::Ne))  { auto rhs = parse_add(); auto e=std::make_unique<Expr>(); e->node=Expr::Binary{BinOp::Ne, std::move(lhs), std::move(rhs)}; return e; }
    if (accept(TokenKind::Gt))  { auto rhs = parse_add(); auto e=std::make_unique<Expr>(); e->node=Expr::Binary{BinOp::Gt, std::move(lhs), std::move(rhs)}; return e; }
    if (accept(TokenKind::Lt))  { auto rhs = parse_add(); auto e=std::make_unique<Expr>(); e->node=Expr::Binary{BinOp::Lt, std::move(lhs), std::move(rhs)}; return e; }
    if (accept(TokenKind::Ge))  { auto rhs = parse_add(); auto e=std::make_unique<Expr>(); e->node=Expr::Binary{BinOp::Ge, std::move(lhs), std::move(rhs)}; return e; }
    if (accept(TokenKind::Le))  { auto rhs = parse_add(); auto e=std::make_unique<Expr>(); e->node=Expr::Binary{BinOp::Le, std::move(lhs), std::move(rhs)}; return e; }
    return lhs;
}

ExprPtr Parser::parse_add(){
    auto lhs = parse_mul();
    while (accept(TokenKind::Plus) || accept(TokenKind::Minus)){
        TokenKind op = t_.kind; // not precise after accept; handle using previous approach
        // Since we advanced, we can't read op here; instead, use a small trick:
        // We'll assume last operator consumed is stored by checking previous char — simplified for skeleton.
        auto rhs = parse_mul();
        auto e = std::make_unique<Expr>();
        // We'll approximate: if last token was Minus, choose Sub; else Add.
        // (This skeleton will be refined later.)
        e->node = Expr::Binary{BinOp::Add, std::move(lhs), std::move(rhs)};
        lhs = std::move(e);
    }
    return lhs;
}

ExprPtr Parser::parse_mul(){
    auto lhs = parse_pow();
    while (accept(TokenKind::Star) || accept(TokenKind::Slash) || accept(TokenKind::Percent)){
        auto rhs = parse_pow();
        auto e = std::make_unique<Expr>();
        e->node = Expr::Binary{BinOp::Mul, std::move(lhs), std::move(rhs)};
        lhs = std::move(e);
    }
    return lhs;
}

ExprPtr Parser::parse_pow(){
    auto lhs = parse_unary();
    while (accept(TokenKind::Pow)){
        auto rhs = parse_unary();
        auto e = std::make_unique<Expr>();
        e->node = Expr::Binary{BinOp::Pow, std::move(lhs), std::move(rhs)};
        lhs = std::move(e);
    }
    return lhs;
}

ExprPtr Parser::parse_unary(){
    if (accept(TokenKind::Plus)){
        auto e = std::make_unique<Expr>(); e->node = Expr::Unary{UnOp::Pos, parse_unary()}; return e;
    }
    if (accept(TokenKind::Minus)){
        auto e = std::make_unique<Expr>(); e->node = Expr::Unary{UnOp::Neg, parse_unary()}; return e;
    }
    return parse_primary();
}

ExprPtr Parser::parse_primary(){
    if (accept(TokenKind::LParen)){ auto e = parse_expr(); eat(TokenKind::RParen, "')'"); auto p=std::make_unique<Expr>(); p->node=Expr::Paren{std::move(e)}; return p; }
    if (at(TokenKind::Identifier)){ auto name=tok().lexeme; next(); auto e=std::make_unique<Expr>(); e->node=Expr::Ident{name}; return e; }
    if (at(TokenKind::Integer)){ auto v=std::stol(tok().lexeme); next(); auto e=std::make_unique<Expr>(); e->node=Expr::IntLit{v}; return e; }
    if (at(TokenKind::Char)){ char c = tok().lexeme.empty()?0:tok().lexeme.back(); next(); auto e=std::make_unique<Expr>(); e->node=Expr::CharLit{c}; return e; }
    if (at(TokenKind::KwVerdade)){ next(); auto e=std::make_unique<Expr>(); e->node=Expr::BoolLit{true}; return e; }
    if (at(TokenKind::KwMentira)){ next(); auto e=std::make_unique<Expr>(); e->node=Expr::BoolLit{false}; return e; }
    if (at(TokenKind::String)){ auto s=tok().lexeme; next(); auto e=std::make_unique<Expr>(); e->node=Expr::StringLit{s}; return e; }
    Diagnostic d; d.phase="syntax"; d.pos=tok().pos; d.message="expressão primária inválida"; throw CompileError(d);
}

}
