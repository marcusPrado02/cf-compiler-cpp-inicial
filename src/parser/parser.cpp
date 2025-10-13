#include "cf/parser/parser.hpp"
#include <sstream>

namespace cf {

    // ---------- util ----------
    /**
     * Tenta consumir um token do tipo k.
     * 
     * Se o token atual é do tipo k, consome e retorna true. Caso contrário, retorna false.
     * Se expectMsg for não-nulo, lança erro de sintaxe com a mensagem dada.
     */
    bool Parser::eat(TokenKind k, const char* expectMsg) {
        if (at(k)) { next(); return true; }
        if (expectMsg) {
            syntax_error(peek(), expectMsg);
        }
        return false;
    }

    /**
     * Exige que o token atual seja do tipo k, consumindo-o.
     * 
     * Se o token atual não é do tipo k, lança erro de sintaxe com a mensagem dada.
     */
    void Parser::expect(TokenKind k, const char* msg) {
        if (!eat(k, nullptr)) syntax_error(peek(), msg);
    }

    /**
     * Mapeia keyword de tipo (TokenKind) para CfType.
     * Se o token não é uma keyword de tipo, retorna CfType::Desconhecido.
     */
    CfType Parser::map_type(TokenKind k) {
        switch (k) {
            case TokenKind::KwInteiro:   return CfType::Inteiro;
            case TokenKind::KwLogico:    return CfType::Logico;
            case TokenKind::KwCaractere: return CfType::Caractere;
            default:                     return CfType::Desconhecido;
        }
    }

    /**
     * Lança erro de sintaxe com a mensagem dada, incluindo o token "got" que causou o erro.
     */
    [[noreturn]] void Parser::syntax_error(const Token& got, const std::string& msg) {
        Diagnostic d;
        d.phase = "syntax";
        d.pos = got.pos;
        d.message = msg + " (encontrado: " + to_string(got.kind) + " \"" + got.lexeme + "\")";
        throw CompileError(d);
    }

    // ---------- Programa ----------

    /**
     * Análise sintática do programa.
     * 
     * - Enquanto não EOF, consome Declaração ou Comando.
     * - Retorna Program com os itens lidos.
     */
    Program Parser::parse_program() {
        Program p;
        while (!at(TokenKind::End)) {
            p.items.push_back(parse_decl_or_stmt());
        }
        return p;
    }

    /**
     * Declaração ou Comando.
     * 
     * - Se começa por keyword de tipo, é Declaração.
     * - Caso contrário, é Comando.
     */
    StmtPtr Parser::parse_decl_or_stmt() {
        // Início por keyword de tipo => declaração
        if (at(TokenKind::KwInteiro) || at(TokenKind::KwLogico) || at(TokenKind::KwCaractere))
            return parse_declaration();
        // Caso contrário => comando
        return parse_statement();
    }

    // ---------- Declarações ----------

    /**
     * Declaração: Tipo Ident ';'
     * 
     * - Consome keyword de tipo (KwInteiro/Logico/Caractere).
     * - Consome identificador.
     * - Se o próximo token não é identificador, lança erro de sintaxe.
     * - Consome ';'.
     * - Retorna StmtDecl.
     */
    StmtPtr Parser::parse_declaration() {
        Token typeTok = next();
        CfType ty = map_type(typeTok.kind);
        Token ident = peek();
        if (!eat(TokenKind::Identifier)) {
            syntax_error(ident, "esperado identificador após tipo");
        }
        StmtDecl* d = new StmtDecl{};
        d->pos = typeTok.pos;
        d->type = ty;
        d->name = ident.lexeme;
        expect(TokenKind::Semicolon, "esperado ';' após declaração");
        return StmtPtr(d);
    }

    // ---------- Comandos ----------

    /**
     * Comando: expressão ';'
     * 
     * - Se começa por keyword de comando, é o respectivo comando.  
     * - Se começa por identificador, é atribuição.
     * - Caso contrário, lança erro de sintaxe.
     */
    StmtPtr Parser::parse_statement() {
        if (at(TokenKind::KwEnquanto)) return parse_while();
        if (at(TokenKind::KwSe))       return parse_if();
        if (at(TokenKind::KwPara))     return parse_for();
        if (at(TokenKind::KwImprimir)) return parse_print();

        // Atribuição começa por Ident
        if (at(TokenKind::Identifier)) {
            Token id = next();
            return parse_assign_tail_after_ident(id);
        }

        syntax_error(peek(), "comando inválido");
    }

    /**
     * Atribuição: Ident '<-' Expr ';'
     * 
     * - Consome '<-'.
     * - Consome expressão.
     * - Consome ';'.
     * - Retorna StmtAssign.
     */
    StmtPtr Parser::parse_assign_tail_after_ident(Token identTok) {
        expect(TokenKind::Assign, "esperado '<-' após identificador em atribuição");
        auto value = parse_expr();
        expect(TokenKind::Semicolon, "esperado ';' após atribuição");
        auto s = std::make_unique<StmtAssign>();
        s->pos = identTok.pos;
        s->name = identTok.lexeme;
        s->value = std::move(value);
        return s;
    }

    /**
     * Bloco: '{' {Declaração ou Comando} '}'
     * 
     * - Consome '{'.
     * - Enquanto não encontra '}', consome Declaração ou Comando.
     * - Se encontra EOF antes de '}', lança erro de sintaxe.
     * - Consome '}'.
     * - Retorna vetor de StmtPtr com os itens do bloco.
     */
    std::vector<StmtPtr> Parser::parse_block() {
        expect(TokenKind::LBrace, "esperado '{' para abrir bloco");
        std::vector<StmtPtr> items;
        while (!at(TokenKind::RBrace)) {
            if (at(TokenKind::End))
                syntax_error(peek(), "EOF dentro de bloco — '}' esperado");
            items.push_back(parse_decl_or_stmt());
        }
        expect(TokenKind::RBrace, "esperado '}' para fechar bloco");
        return items;
    }

    /**
     * Comando if: 'Se' Expr Bloco ['Senao' Bloco]
     * 
     * - Consome 'Se'.
     * - Consome expressão.
     * - Consome bloco (then).
     * - Se próximo token é 'Senao', consome e consome bloco (else).
     * - Retorna StmtIf.
     */
    StmtPtr Parser::parse_if() {
        Token kw = next(); // Se
        auto cond = parse_expr();
        auto then_body = parse_block();
        std::vector<StmtPtr> else_body;
        if (at(TokenKind::KwSenao)) {
            next();
            else_body = parse_block();
        }
        auto s = std::make_unique<StmtIf>();
        s->pos = kw.pos;
        s->cond = std::move(cond);
        s->then_body = std::move(then_body);
        s->else_body = std::move(else_body);
        return s;
    }

    /**
     * Comando while: 'Enquanto' Expr Bloco
     * 
     * - Consome 'Enquanto'.
     * - Consome expressão.
     * - Consome bloco.
     * - Retorna StmtWhile.
     */
    StmtPtr Parser::parse_while() {
        Token kw = next(); // Enquanto
        auto cond = parse_expr();
        auto body = parse_block();
        auto s = std::make_unique<StmtWhile>();
        s->pos = kw.pos;
        s->cond = std::move(cond);
        s->body = std::move(body);
        return s;
    }

    /**
     * Comando for: 'Para' Ident 'em' '(' Expr ',' Expr [',' Expr] ')' Bloco
     * 
     * - Consome 'Para'.
     * - Consome identificador.
     * - Consome 'em'.
     * - Consome '('.
     * - Consome expressão inicial.
     * - Consome ','.
     * - Consome expressão final.
     * - Se próximo token é ',', consome e consome expressão de passo.
     * - Consome ')'.
     * - Consome bloco.
     * - Retorna StmtFor.
     */
    StmtPtr Parser::parse_for() {
        Token kw = next(); // Para
        Token var = peek();
        expect(TokenKind::Identifier, "esperado identificador após 'Para'");
        expect(TokenKind::KwEm, "esperado keyword 'em' após identificador de 'Para'");
        expect(TokenKind::LParen, "esperado '(' após 'em' em 'Para'");
        auto begin = parse_expr();
        expect(TokenKind::Comma, "esperado ',' após expressão inicial em 'Para'");
        auto end = parse_expr();
        std::optional<ExprPtr> step;
        if (at(TokenKind::Comma)) {
            next();
            step = parse_expr();
        }
        expect(TokenKind::RParen, "esperado ')' ao final da cabeçalho de 'Para'");
        auto body = parse_block();

        auto s = std::make_unique<StmtFor>();
        s->pos = kw.pos;
        s->var = var.lexeme;
        s->begin = std::move(begin);
        s->end = std::move(end);
        if (step) s->step = std::move(*step);
        s->body = std::move(body);
        return s;
    }

    /**
     * Comando print: 'Imprimir' '(' [Expr {',' Expr}] ')' ';'
     * 
     * - Consome 'Imprimir'.
     * - Consome '('.
     * - Se próximo token não é ')', consome expressão e, enquanto próximo token é ',', consome ',' e expressão.
     * - Consome ')'.
     * - Consome ';'.
     * - Retorna StmtPrint.
     */
    StmtPtr Parser::parse_print() {
        Token kw = next(); // Imprimir
        expect(TokenKind::LParen, "esperado '(' após 'Imprimir'");
        std::vector<ExprPtr> args;
        if (!at(TokenKind::RParen)) {
            // ListaExpr: Expr {, Expr}
            args.push_back(parse_expr());
            while (at(TokenKind::Comma)) {
                next();
                args.push_back(parse_expr());
            }
        }
        expect(TokenKind::RParen, "esperado ')' após argumentos de 'Imprimir'");
        expect(TokenKind::Semicolon, "esperado ';' após 'Imprimir(...)'");
        auto s = std::make_unique<StmtPrint>();
        s->pos = kw.pos;
        s->args = std::move(args);
        return s;
    }

    // ---------- Expressões (precedência) ----------

    /**
     * Expressão: ExprLogico
     * 
     * - Retorna o resultado de parse_logical().
     */
    ExprPtr Parser::parse_expr() {
        return parse_logical();
    }

    /**
     * ExprLogico: ExprRel {(&& | ||) ExprRel}*
     * 
     * - Consome ExprRel.
     * - Enquanto próximo token é '&&' ou '||', consome o operador e ExprRel.
     * - Retorna árvore de ExprBinary com os operadores lidos (esquerda-para-direita).
     * Note que '&&' e '||' têm a mesma precedência e são left-associative.
     */
    ExprPtr Parser::parse_logical() {
        auto lhs = parse_rel();
        while (at(TokenKind::And) || at(TokenKind::Or)) {
            Token op = next();
            auto rhs = parse_rel();
            auto e = std::make_unique<ExprBinary>();
            e->pos = op.pos;
            e->op  = (op.kind == TokenKind::And) ? BinOp::And : BinOp::Or;
            e->lhs = std::move(lhs);
            e->rhs = std::move(rhs);
            lhs = std::move(e);
        }
        return lhs;
    }

    /**
     * ExprRel: ExprAdd [(== | != | > | < | >= | <=) ExprAdd]
     * 
     * - Consome ExprAdd.   
     * - Se próximo token é um operador relacional, consome o operador e ExprAdd.
     * - Retorna ExprBinary se operador relacional foi lido, ou ExprAdd caso contrário.
     * Note que operadores relacionais têm a mesma precedência e são non-associative
     */
    ExprPtr Parser::parse_rel() {
        auto lhs = parse_add();
        if (at(TokenKind::Eq) || at(TokenKind::Ne) || at(TokenKind::Gt) || at(TokenKind::Lt)
            || at(TokenKind::Ge) || at(TokenKind::Le)) {
            Token op = next();
            auto rhs = parse_add();
            auto e = std::make_unique<ExprBinary>();
            e->pos = op.pos;
            switch (op.kind) {
                case TokenKind::Eq: e->op = BinOp::Eq; break;
                case TokenKind::Ne: e->op = BinOp::Ne; break;
                case TokenKind::Gt: e->op = BinOp::Gt; break;
                case TokenKind::Lt: e->op = BinOp::Lt; break;
                case TokenKind::Ge: e->op = BinOp::Ge; break;
                case TokenKind::Le: e->op = BinOp::Le; break;
                default: break;
            }
            e->lhs = std::move(lhs);
            e->rhs = std::move(rhs);
            return e;
        }
        return lhs;
    }

    /**
     * ExprAdd: ExprMult {(+ | -) ExprMult}*
     *
     * - Consome ExprMult.
     * - Enquanto próximo token é '+' ou '-', consome o operador e ExprMult.
     * - Retorna árvore de ExprBinary com os operadores lidos (esquerda-para-direita).
     * Note que '+' e '-' têm a mesma precedência e são left-associative.
     */
    ExprPtr Parser::parse_add() {
        auto lhs = parse_mul();
        while (at(TokenKind::Plus) || at(TokenKind::Minus)) {
            Token op = next();
            auto rhs = parse_mul();
            auto e = std::make_unique<ExprBinary>();
            e->pos = op.pos;
            e->op  = (op.kind == TokenKind::Plus) ? BinOp::Add : BinOp::Sub;
            e->lhs = std::move(lhs);
            e->rhs = std::move(rhs);
            lhs = std::move(e);
        }
        return lhs;
    }

    /**
     * ExprMult: ExprPow {(* | / | %) ExprPow}*
     * 
     * - Consome ExprPow.
     * - Enquanto próximo token é '*', '/' ou '%', consome o operador e ExprPow.
     * - Retorna árvore de ExprBinary com os operadores lidos (esquerda-para-direita).
     * Note que '*', '/' e '%' têm a mesma precedência e são left-associative.
     */
    ExprPtr Parser::parse_mul() {
        auto lhs = parse_pow();
        while (at(TokenKind::Star) || at(TokenKind::Slash) || at(TokenKind::Percent)) {
            Token op = next();
            auto rhs = parse_pow();
            auto e = std::make_unique<ExprBinary>();
            e->pos = op.pos;
            switch (op.kind) {
                case TokenKind::Star:    e->op = BinOp::Mul; break;
                case TokenKind::Slash:   e->op = BinOp::Div; break;
                case TokenKind::Percent: e->op = BinOp::Mod; break;
                default: break;
            }
            e->lhs = std::move(lhs);
            e->rhs = std::move(rhs);
            lhs = std::move(e);
        }
        return lhs;
    }

    /**
     * ExprPow: ExprPrim ["**" ExprPow]
     * 
     * - Consome ExprPrim.
     * - Se próximo token é '**', consome e consome ExprPow
     * - Retorna ExprBinary se '**' foi lido, ou ExprPrim caso contrário.
     * Note que '**' é right-associative.
     */
    ExprPtr Parser::parse_pow() {     
        auto base = parse_primary();
        if (at(TokenKind::Pow)) {
            Token op = next();      
            auto expo = parse_pow(); 
            auto e = std::make_unique<ExprBinary>();
            e->pos = op.pos;
            e->op  = BinOp::Pow;
            e->lhs = std::move(base);
            e->rhs = std::move(expo);
            return e;
        }
        return base;
    }

    /**
     * ExprPrim: Integer | Char | String | Ident | '(' Expr ')'
     * 
     * - Se próximo token é Integer/Char/String/Ident, consome e retorna o respectivo Expr*.
     * - Se próximo token é '(', consome, consome Expr, consome ')' e retorna ExprGroup.
     * - Caso contrário, lança erro de sintaxe.
     * Note que não há suporte a expressões unárias (ex: -x, !cond) neste compilador.
     */
    ExprPtr Parser::parse_primary() {
        Token t = peek();
        switch (t.kind) {
            case TokenKind::Integer: {
                next();
                auto e = std::make_unique<ExprInteger>();
                e->pos = t.pos;
                e->digits = t.lexeme;
                return e;
            }
            case TokenKind::Char: {
                next();
                auto e = std::make_unique<ExprChar>();
                e->pos = t.pos;
                e->content = t.lexeme;
                return e;
            }
            case TokenKind::String: {
                next();
                auto e = std::make_unique<ExprString>();
                e->pos = t.pos;
                e->content = t.lexeme;
                return e;
            }
            case TokenKind::Identifier: {
                next();
                auto e = std::make_unique<ExprIdent>();
                e->pos = t.pos;
                e->name = t.lexeme;
                return e;
            }
            case TokenKind::LParen: {
                next();
                auto inner = parse_expr();
                expect(TokenKind::RParen, "esperado ')'");
                auto e = std::make_unique<ExprGroup>();
                e->pos = t.pos;
                e->inner = std::move(inner);
                return e;
            }
            default:
                syntax_error(t, "expressão primária esperada");
        }
    }
} 
