#include "cf/semantic/semantics.hpp"
#include <sstream>

namespace cf {

    /**
     * Analisa semanticamente o programa.
     * 
     * - Cria o escopo global.
     * - Percorre todas as declarações e comandos top-level, 
     * verificando cada um.
     * - Remove o escopo global ao final.
     */
    void Semantic::analyze(Program& p) {
        scopes_.enter(); // escopo global
        for (auto& it : p.items) {
            check_stmt(*it);
        }
        scopes_.leave();
    }

    // ------------------ Stmts ------------------

    /**
     * Verifica o tipo de comando e chama a função específica.
     * 
     * - Declaração: check_decl
     * - Atribuição: check_assign
     * - Imprimir: check_print
     * - Enquanto: check_while
     * - Se: check_if
     * - Para: check_for
     * - Bloco: entra em novo escopo, verifica cada comando, sai do escopo
     *    - Entra em um novo escopo
     *    - Verifica cada comando dentro do bloco
     *    - Sai do escopo
     * 
     */
    void Semantic::check_stmt(Stmt& s) {
        if (auto* d = dynamic_cast<StmtDecl*>(&s)) return check_decl(*d);
        if (auto* a = dynamic_cast<StmtAssign*>(&s)) return check_assign(*a);
        if (auto* pr = dynamic_cast<StmtPrint*>(&s)) return check_print(*pr);
        if (auto* w = dynamic_cast<StmtWhile*>(&s)) return check_while(*w);
        if (auto* i = dynamic_cast<StmtIf*>(&s)) return check_if(*i);
        if (auto* f = dynamic_cast<StmtFor*>(&s)) return check_for(*f);
        if (auto* b = dynamic_cast<StmtBlock*>(&s)) {
            scopes_.enter();
            for (auto& st : b->body) check_stmt(*st);
            scopes_.leave();
            return;
        }
        // default: nada
    }

    /**
     * Declara uma variável no escopo atual.
     * 
     * - Se já existe no mesmo escopo, erro.
     * - Se possuir inicialização (`init`), verifica o tipo da expressão
     *   e a compatibilidade de atribuição.
     */
    void Semantic::check_decl(StmtDecl& s) {
        if (!scopes_.declare(s.name, s.type)) {
            sem_error(s.pos, "redeclaracao de '" + s.name + "' no mesmo escopo");
        }
        // Inicialização opcional: Tipo nome <- Expr
        if (s.init) {
            CfType rhs = check_expr(*s.init);
            if (!assign_compatible(s.type, rhs)) {
                sem_error(s.pos, "inicializacao incompatível de '" + s.name + "'");
            }
        }
    }

    /**
     * Verifica a atribuição.
     * 
     * - Verifica se a variável foi declarada (scopes_.find); se não, erro.
     * - Verifica o tipo da expressão do lado direito (check_expr).
     * - Verifica se o tipo da variável é compatível com o tipo da expressão (assign_compatible);
     *   se não, erro.
     */
    void Semantic::check_assign(StmtAssign& s) {
        auto sym = scopes_.find(s.name);
        if (!sym) {
            sem_error(s.pos, "variavel '" + s.name + "' nao declarada");
        }
        CfType rhs = check_expr(*s.value);
        if (!assign_compatible(sym->type, rhs)) {
            sem_error(s.pos, "atribuição incompatível: destino " +
                std::to_string((int)sym->type) + " <- origem " + std::to_string((int)rhs));
        }
    }

    /**
     * Verifica os argumentos de Imprimir.  
     * 
     * - Cada argumento deve ser de tipo Inteiro, Logico ou Caractere.
     * - Strings são permitidas (tipo especial aqui), tratadas como Caractere* (aceito).
     * - Qualquer outro tipo, erro.
     */
    void Semantic::check_print(StmtPrint& s) {
        for (auto& e : s.args) {
            CfType t = check_expr(*e);
            switch (t) {
                case CfType::Inteiro:
                case CfType::Logico:
                case CfType::Caractere:
                    break; // ok
                default:
                    // ExprString é permitido (tipo especial aqui): tratar como Caractere* (aceito)
                    if (dynamic_cast<ExprString*>(e.get())) break;
                    sem_error(e->pos, "tipo invalido para Imprimir");
            }
        }
    }

    /**
     * Verifica o comando Se.
     * 
     * - Verifica a expressão condicional (check_expr); deve ser Logico, senão erro.
     * - Entra em novo escopo, verifica cada comando do corpo "então", sai do escopo.
     * - Entra em novo escopo, verifica cada comando do corpo "senão", sai do escopo.
     * - Se a condição for verdadeira, executa o corpo "então"; caso contrário, executa o corpo "senão".
     */
    void Semantic::check_if(StmtIf& s) {
        CfType c = check_expr(*s.cond);
        if (!is_logical(c)) sem_error(s.cond->pos, "condicao de 'Se' deve ser Logico");
        scopes_.enter();
        for (auto& st : s.then_body) check_stmt(*st);
        scopes_.leave();
        scopes_.enter();
        for (auto& st : s.else_body) check_stmt(*st);
        scopes_.leave();
    }

    /**
     * Verifica o comando Enquanto.
     * 
     * - Verifica a expressão condicional (check_expr); deve ser Logico, senão erro.
     * - Entra em novo escopo, verifica cada comando do corpo, sai do escopo.
     * - Enquanto a condição for verdadeira, executa o corpo do loop.
     */
    void Semantic::check_while(StmtWhile& s) {
        CfType c = check_expr(*s.cond);
        if (!is_logical(c)) sem_error(s.cond->pos, "condicao de 'Enquanto' deve ser Logico");
        scopes_.enter();
        for (auto& st : s.body) check_stmt(*st);
        scopes_.leave();
    }

    /**
     * Verifica o comando Para.
     * 
     * - Entra em novo escopo.
     * - Se a variável de controle já existe no mesmo escopo do 'for', erro.
     * - Declara a variável de controle como Inteiro.
     * - Verifica a expressão de início (check_expr); deve ser Inteiro ou Caractere (promovido a Inteiro), senão erro.
     * - Verifica a expressão de fim (check_expr); deve ser Inteiro ou Caractere (promovido a Inteiro), senão erro.
     * - Se no passo (step) for fornecido:
     *   - Verifica a expressão do passo (check_expr); deve ser Inteiro ou Caractere (promovido a Inteiro), senão erro.
     * - Verifica cada comando do corpo (check_stmt).
     * - Sai do escopo.
     */
    void Semantic::check_for(StmtFor& s) {
        // Escopo do 'for': variável de controle local do loop (Inteiro)
        scopes_.enter();
        if (!scopes_.declare(s.var, CfType::Inteiro)) {
            // Se já existia no mesmo escopo do 'for' (raro), erro
            sem_error(s.pos, "variavel de controle '" + s.var + "' já existe no escopo do 'Para'");
        }
        CfType tb = check_expr(*s.begin);
        CfType te = check_expr(*s.end);
        if (!(is_int(promote_if_needed(tb)) && is_int(promote_if_needed(te)))) {
            sem_error(s.pos, "inicio/fim do 'Para' devem ser Inteiro/Caractere (promovidos a Inteiro)");
        }
        if (s.step.has_value()) {
            CfType ts = check_expr(**s.step);
            // Para o passo, exigimos estritamente tipo Inteiro (sem promoção de Caractere)
            if (!is_int(ts)) {
                sem_error((**s.step).pos, "passo do 'Para' deve ser Inteiro");
            }
        }
        for (auto& st : s.body) check_stmt(*st);
        scopes_.leave();
    }

    // ------------------ Exprs ------------------

    /**
     * Verifica o tipo da expressão.
     * 
     * - ExprInteger: tipo Inteiro
     * - ExprChar: tipo Caractere
     * - ExprString: tipo Desconhecido (válido apenas como argumento de Imprimir)
     * - ExprIdent: procura na tabela de símbolos (scopes_.find); se não existe, erro; tipo é o da variável
     * - ExprBinary: check_binary
     * - ExprGroup: check_group
     * - Qualquer outro tipo: Desconhecido
     */
    CfType Semantic::check_expr(Expr& e) {
        if (auto* i = dynamic_cast<ExprInteger*>(&e)) {
            e.inferred = CfType::Inteiro;
            return e.inferred;
        }
        if (auto* c = dynamic_cast<ExprChar*>(&e)) {
            e.inferred = CfType::Caractere;
            return e.inferred;
        }
        if (auto* s = dynamic_cast<ExprString*>(&e)) {
            // Strings só são válidas como argumento de Imprimir; não participam de operações
            e.inferred = CfType::Desconhecido;
            return e.inferred;
        }
        if (auto* id = dynamic_cast<ExprIdent*>(&e)) {
            auto sym = scopes_.find(id->name);
            if (!sym) sem_error(e.pos, "variavel '" + id->name + "' nao declarada");
            e.inferred = sym->type;
            return e.inferred;
        }
        if (auto* b = dynamic_cast<ExprBinary*>(&e)) {
            return check_binary(*b);
        }
        if (auto* g = dynamic_cast<ExprGroup*>(&e)) {
            return check_group(*g);
        }
        if (auto* b = dynamic_cast<ExprBool*>(&e)) {
            e.inferred = CfType::Logico;
            return e.inferred;
        }

        // fallback
        e.inferred = CfType::Desconhecido;
        return e.inferred;
    }

    /**
     * Verifica o tipo da expressão binária.
     * 
     * - Verifica o tipo do lado esquerdo (check_expr).
     * - Verifica o tipo do lado direito (check_expr).
     * - Dependendo do operador:
     *   - Aritméticos (+, -, *, /, %, ^): usa result_arith; se Desconhecido, erro.
     *   - Relacionais (==, !=, >, <, >=, <=): usa result_rel; se Desconhecido, erro.
     *   - Lógicos (&&, ||): usa result_logic; se Desconhecido, erro.
     * - Define e retorna o tipo inferido da expressão (e.inferred).
     */
    CfType Semantic::check_binary(ExprBinary& e) {
        CfType lt = check_expr(*e.lhs);
        CfType rt = check_expr(*e.rhs);

        switch (e.op) {
            // Aritméticos
            case BinOp::Add: case BinOp::Sub: case BinOp::Mul:
            case BinOp::Div: case BinOp::Mod: case BinOp::Pow: {
                CfType r = result_arith(lt, rt);
                if (r == CfType::Desconhecido) {
                    sem_error(e.pos, "operacao aritmetica requer Inteiro (Char promove a Inteiro)");
                }
                e.inferred = r; return r;
            }
            // Relacionais
            case BinOp::Eq: case BinOp::Ne: case BinOp::Gt:
            case BinOp::Lt: case BinOp::Ge: case BinOp::Le: {
                CfType r = result_rel(lt, rt);
                if (r == CfType::Desconhecido) {
                    sem_error(e.pos, "operacao relacional requer Inteiro/Caractere (promovidos)");
                }
                e.inferred = r; return r;
            }
            // Lógicos
            case BinOp::And: case BinOp::Or: {
                CfType r = result_logic(lt, rt);
                if (r == CfType::Desconhecido) {
                    sem_error(e.pos, "operacao logica requer Logico");
                }
                e.inferred = r; return r;
            }
        }
        e.inferred = CfType::Desconhecido;
        return e.inferred;
    }

    /**
     * Verifica o grupo de expressão (parênteses).
     * 
     * - Verifica o tipo da expressão interna (check_expr).
     * - Define e retorna o tipo inferido da expressão (e.inferred).
     */
    CfType Semantic::check_group(ExprGroup& e) {
        e.inferred = check_expr(*e.inner);
        return e.inferred;
    }

    // ------------------ util ------------------

    /**
     * Lança um erro de semântica com a posição e mensagem fornecidas.
     * 
     * - Cria um Diagnostic com a fase "semantic", posição e mensagem.
     * - Lança CompileError com o Diagnostic.
     */
    [[noreturn]] void Semantic::sem_error(const Position& pos, const std::string& msg) {
        Diagnostic d;
        d.phase = "semantic";
        d.pos = pos;
        d.message = msg;
        throw CompileError(d);
    }

} 
