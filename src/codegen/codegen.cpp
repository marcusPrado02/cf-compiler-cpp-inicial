#include "cf/codegen/codegen.hpp"
#include <sstream>
#include <iomanip>

namespace cf
{

    // ---------- Escopos e alocação ----------

    /**
     * Entra em um novo escopo.
     */
    void Codegen::enter_scope()
    {
        scopes_.emplace_back();
    }

    /**
     * Sai do escopo atual, liberando espaço na stack.
     *
     * - Calcula o total de bytes alocados no escopo atual.
     * - Ajusta o frame_size_ e o stack pointer ($sp) para liberar esse espaço.
     * - Remove o escopo atual da pilha de escopos.
     */
    void Codegen::leave_scope()
    {
        int to_free = 0;
        for (auto const &kv : scopes_.back())
        {
            const VarInfo &v = kv.second;
            to_free += v.size + v.padding;
        }
        if (to_free > 0)
        {
            frame_size_ -= to_free;
            std::ostringstream os;
            os << "  addiu $sp, $sp, " << to_free
               << "   # leave-scope free " << to_free << "\n";
            text_ += os.str();
        }
        scopes_.pop_back();
    }

    /**
     * Procura uma variável na pilha de escopos (do topo para baixo).
     *
     * - Para cada escopo, verifica se a variável existe.
     * - Se encontrada, retorna um ponteiro para VarInfo.
     * - Se não encontrada, retorna nullptr.
     */
    VarInfo *Codegen::lookup(const std::string &name)
    {
        for (int i = (int)scopes_.size() - 1; i >= 0; --i)
        {
            auto it = scopes_[i].find(name);
            if (it != scopes_[i].end())
                return &it->second;
        }
        return nullptr;
    }

    /**
     * Aloca bytes na stack com alinhamento.
     *
     * - Calcula o alinhamento necessário com base no parâmetro align.
     * - Se o frame_size_ atual não estiver alinhado, adiciona padding.
     * - Atualiza o frame_size_ com os bytes alocados.
     * - Emite a instrução de ajuste do stack pointer ($sp).
     * - Retorna o offset relativo ao frame-pointer ($fp) para a alocação
     */
    int Codegen::alloc_bytes_aligned(int bytes, int align, int &outPad)
    {
        int mis = frame_size_ % align;
        int pad = 0;

        if (mis != 0)
        {
            pad = align - mis;
            frame_size_ += pad;
            ln("  addiu $sp, $sp, -" + std::to_string(pad) +
               "   # padding align");
        }

        frame_size_ += bytes;
        ln("  addiu $sp, $sp, -" + std::to_string(bytes) +
           "   # alloc bytes");

        outPad = pad;
        return -frame_size_; // offset relativo a $fp
    }

    /**
     * Declara uma variável no escopo atual.
     *
     * - Se o escopo estiver vazio, entra em um novo escopo.
     * - Atribui a top ao escopo atual.
     * - Calcula o tamanho e alinhamento com base no tipo (4 bytes para Inteiro, 1 byte para Caractere/Logico).
     * - Aloca os bytes necessários, alinhando se necessário.
     * - Cria um VarInfo com o tipo, offset e tamanho.
     * - Tenta inserir na tabela do escopo atual; se já existe, sobrescreve.
     * - Retorna uma referência para o VarInfo inserido.
     */
    VarInfo &Codegen::declare(const std::string &name, CfType ty)
    {
        if (scopes_.empty())
            enter_scope();
        auto &top = scopes_.back();

        int size = (ty == CfType::Inteiro) ? 4 : 1;
        int align = (ty == CfType::Inteiro) ? 4 : 1;

        int pad = 0;
        int offset = alloc_bytes_aligned(size, align, pad);

        VarInfo v{ty, offset, size, pad};
        auto [it, ok] = top.emplace(name, v);
        if (!ok)
        {
            it->second = v;
        }
        return it->second;
    }

    // ---------- Pool de strings ----------

    /**
     * Adiciona uma string ao pool de strings (.rodata) se ainda não existe.
     *
     * - Para cada string no pool, verifica se já existe.
     * - Se encontrada, retorna o índice do rótulo existente.
     * - Se não encontrada, cria um novo StrLit com o valor e índice, adiciona ao pool.
     * - Retorna o índice do novo rótulo.
     */
    int Codegen::put_string(const std::string &s)
    {
        // Procura existente
        for (auto const &it : str_pool_)
        {
            if (it.value == s)
                return it.labelIndex;
        }
        int idx = (int)str_pool_.size();
        str_pool_.push_back(StrLit{s, idx});
        return idx;
    }

    /**
     * Emite a seção .rodata com as strings do pool.
     *
     * - Se o pool estiver vazio, não faz nada.
     * - Para cada string no pool, emite um rótulo e a diretiva .asciiz com o conteúdo escapado.
     * - Adiciona tudo ao buffer de dados (data_), não ao texto principal (text_).
     * - Emite a seção .rodata no formato apropriado.
     */
    void Codegen::emit_rodata()
    {
        if (str_pool_.empty())
            return;

        std::ostringstream all;
        all << ".data\n";

        for (auto const &it : str_pool_)
        {
            all << "L.str." << it.labelIndex << ":\n"
                << "  .asciiz \"";

            const std::string &s = it.value;

            for (std::size_t i = 0; i < s.size(); ++i)
            {
                char c = s[i];

                // Trata sequências de escape vindas do código fonte: \n, \t, \",
                if (c == '\\' && i + 1 < s.size())
                {
                    char n = s[i + 1];
                    switch (n)
                    {
                    case 'n':
                        all << "\\n"; // quebra de linha
                        ++i;          // pula o 'n'
                        break;
                    case 't':
                        all << "\\t"; // tab
                        ++i;
                        break;
                    case '\\':
                        all << "\\\\"; // barra invertida
                        ++i;
                        break;
                    case '\"':
                        all << "\\\""; // aspas
                        ++i;
                        break;
                    default:
                        // escape desconhecido: só imprime os dois como vieram
                        all << '\\' << n;
                        ++i;
                        break;
                    }
                }
                else
                {
                    // precisa escapar aspas e barra "soltas"
                    if (c == '\"' || c == '\\')
                        all << '\\' << c;
                    else
                        all << c;
                }
            }

            all << "\"\n";
        }

        data_ += all.str();
    }

    // ---------- Util ----------

    /**
     * Gera um novo rótulo único baseado em um nome base.
     *
     * - Concatena o base com um contador único.
     * - Incrementa o contador para a próxima chamada.
     * - Retorna o rótulo gerado.
     */
    std::string Codegen::new_label(const std::string &base)
    {
        std::ostringstream os;
        os << "L_" << base << "_" << (label_id_++);
        return os.str();
    }

    /**
     * Normaliza o valor em $a0 para booleano 0xFF/0x00.
     *
     * - Se $a0 for diferente de zero, define $a0 como 0xFF (verdadeiro).
     * - Se $a0 for zero, define $a0 como 0x00 (falso).
     * - Adiciona a saída ao texto gerado.
     */
    void Codegen::emit_bool_normalize()
    {
        // supomos $a0 = 0 ou !=0
        // $t0 = ($a0 != 0) ? 1 : 0
        ln("  sltu $t0, $zero, $a0");
        // $a0 = -$t0 -> 0 ou 0xFFFFFFFF
        ln("  subu $a0, $zero, $t0");
        // mantém só o byte baixo: 0x00 ou 0xFF
        ln("  andi $a0, $a0, 0x00FF");
    }

    /**
     * Carrega o valor de uma variável em $a0.
     *
     * - Se a variável for Inteiro, usa lw.
     * - Se for Caractere ou Logico, usa lb.
     * - Adiciona a saída ao texto gerado.
     * - Usa o offset relativo ao frame-pointer (s0).
     */
    void Codegen::emit_load_var_to_a0(const VarInfo &v)
    {
        std::ostringstream os;
        if (v.type == CfType::Inteiro)
        {
            os << "  lw $a0, " << v.offset << "($fp)\n";
        }
        else
        {
            // lógico/caractere: 1 byte, sem sinal
            os << "  lbu $a0, " << v.offset << "($fp)\n";
        }
        out(os.str());
    }

    /**
     * Armazena o valor de $a0 em uma variável.
     *
     * - Se a variável for Inteiro, usa sw.
     * - Se for Caractere ou Logico, normaliza $a0 para 0x00/0xFF se for Logico, e usa sb.
     * - Adiciona a saída ao texto gerado.
     * - Usa o offset relativo ao frame-pointer (s0).
     * - O parâmetro rhsTy indica o tipo da expressão do lado direito (RHS) para tratamento especial de booleanos.
     */
    void Codegen::emit_store_a0_to_var(const VarInfo &v, CfType rhsTy)
    {
        std::ostringstream os;
        if (v.type == CfType::Inteiro)
        {
            os << "  sw $a0, " << v.offset << "($fp)\n";
        }
        else
        {
            if (v.type == CfType::Logico)
            {
                emit_bool_normalize();
            }
            else
            {
                ln("  andi $a0, $a0, 0x00FF");
            }
            os << "  sb $a0, " << v.offset << "($fp)\n";
        }
        out(os.str());
    }

    // ---------- Expressões ----------

    /**
     * Emite código para uma expressão agrupada (parênteses).
     *
     * - Simplesmente emite o código da expressão interna.
     */
    void Codegen::emit_expr_group(const ExprGroup &e)
    {
        emit_expr(*e.inner);
    }

    /**
     * Emite código para uma expressão binária.
     *
     * - O resultado final da expressão fica em $a0.
     * - Suporta apenas tipos Inteiro para operações aritméticas e relacionais.
     * - Para operadores lógicos, normaliza o resultado para 0xFF/0x00.
     * - Lança erro se o operador não for suportado.
     * - Suporta o operador de potência (Pow) com loop.
     * - Não trata concatenação de strings aqui; isso é feito em outro lugar.
     * - Assume que as expressões LHS e RHS já foram verificadas semanticamente.
     * - Não faz verificação de tipos; assume que os tipos são compatíveis para o operador.
     * - Não lida com estouro de inteiros ou erros de divisão por zero.
     * - Usa registradores temporários $t0, $t1, $t2, $t3 conforme necessário.
     * - Empilha o valor de LHS na stack para preservar durante a avaliação de RHS.
     * - Desempilha o valor de LHS após avaliar RHS.
     * - O código gerado é adicionado ao buffer de texto principal (text_).
     */
    void Codegen::emit_expr_bin(const ExprBinary &b)
    {
        // 1) avalia LHS em $a0
        emit_expr(*b.lhs);

        // 2) empilha LHS para não ser destruído por chamadas recursivas
        ln("  addiu $sp, $sp, -4      # push lhs");
        ln("  sw $a0, 0($sp)");

        // 3) avalia RHS em $a0
        emit_expr(*b.rhs);

        // 4) carrega RHS em $t1 e recupera LHS em $t0
        ln("  move $t1, $a0          # rhs");
        ln("  lw $t0, 0($sp)         # lhs");
        ln("  addiu $sp, $sp, 4      # pop lhs");

        // 5) agora $t0 = lhs, $t1 = rhs, pode aplicar o operador
        switch (b.op)
        {
        // Aritméticos
        case BinOp::Add:
            ln("  add $a0, $t0, $t1");
            break;
        case BinOp::Sub:
            ln("  sub $a0, $t0, $t1");
            break;
        case BinOp::Mul:
            ln("  mul $a0, $t0, $t1");
            break;
        case BinOp::Div:
            // $a0 = $t0 / $t1 (quociente inteiro)
            ln("  div $t0, $t1"); // LO = quociente, HI = resto
            ln("  mflo $a0");     // $a0 = quociente
            break;
        case BinOp::Mod:
            ln("  div $t0, $t1"); // quociente em LO, resto em HI
            ln("  mfhi $a0");     // $a0 = resto
            break;

        case BinOp::Pow:
        {
            // $a0 = $t0 ** $t1 (inteiros, expoente >= 0)
            std::string Lloop = new_label("pow.loop");
            std::string Lend = new_label("pow.end");
            ln("  li $a0, 1         # acc = 1");
            ln("  move $t2, $t0      # base");
            ln("  move $t3, $t1      # exp");
            ln(Lloop + ":");
            ln("  beq $t3, $zero, " + Lend);
            ln("  mul $a0, $a0, $t2");
            ln("  addi $t3, $t3, -1");
            ln("  j " + Lloop);
            ln(Lend + ":");
            break;
        }

        // Relacionais -> 0/1
        case BinOp::Eq:
            ln("  subu $t2, $t0, $t1");
            ln("  sltu $a0, $zero, $t2"); // $a0 = ($t2 != 0)
            ln("  xori $a0, $a0, 1");     // inverte -> ==
            break;
        case BinOp::Ne:
            ln("  subu $t2, $t0, $t1");
            ln("  sltu $a0, $zero, $t2"); // !=
            break;
        case BinOp::Lt:
            ln("  slt $a0, $t0, $t1");
            break;
        case BinOp::Gt:
            ln("  slt $a0, $t1, $t0");
            break;
        case BinOp::Le:
            ln("  slt $a0, $t1, $t0"); // $t1 < $t0
            ln("  xori $a0, $a0, 1");  // !($t1 < $t0)
            break;
        case BinOp::Ge:
            ln("  slt $a0, $t0, $t1"); // $t0 < $t1
            ln("  xori $a0, $a0, 1");  // !($t0 < $t1)
            break;

        // Lógicos (&, ^) — tratamos operands como truthy e normalizamos no fim
        case BinOp::And:
            ln("  sltu $t0, $zero, $t0");
            ln("  sltu $t1, $zero, $t1");
            ln("  and $a0, $t0, $t1");
            emit_bool_normalize();
            break;
        case BinOp::Or:
            ln("  sltu $t0, $zero, $t0");
            ln("  sltu $t1, $zero, $t1");
            ln("  or $a0, $t0, $t1");
            emit_bool_normalize();
            break;
        }
    }

    /**
     * Emite código para uma expressão.
     *
     * - Se for ExprInteger, carrega o valor imediato em $a0.
     * - Se for ExprChar, converte o caractere para seu valor ASCII e carrega em $a0.
     * - Se for ExprString, obtém o índice do pool de strings, carrega o endereço em $a0, o tamanho em a1, e chama print_str.
     * - Se for ExprIdent, procura a variável e carrega seu valor em $a0.
     * - Se for ExprBinary, chama emit_expr_bin.
     * - Se for ExprGroup, chama emit_expr_group.
     * - Se não reconhecido, carrega 0 em $a0 como fallback.
     */
    void Codegen::emit_expr(const Expr &e)
    {
        if (auto *i = dynamic_cast<const ExprInteger *>(&e))
        {
            // Inteiros decimais simples
            std::ostringstream os;
            os << "  li $a0, " << i->digits << "\n";
            out(os.str());
            return;
        }
        if (auto *ch = dynamic_cast<const ExprChar *>(&e))
        {
            // content carrega, por ex, "\\n" ou "a" — aqui só suportamos 1 byte ASCII
            unsigned char val = 0;
            if (ch->content.size() == 1)
            {
                val = static_cast<unsigned char>(ch->content[0]);
            }
            else if (ch->content.size() == 2 && ch->content[0] == '\\\\')
            {
                // escapes simples: \n \t \\ \' "
                switch (ch->content[1])
                {
                case 'n':
                    val = 10;
                    break;
                case 't':
                    val = 9;
                    break;
                case '\\\\':
                    val = '\\\\';
                    break;
                case '\'':
                    val = '\'';
                    break;
                case '\"':
                    val = '\"';
                    break;
                default:
                    val = static_cast<unsigned char>(ch->content[1]);
                    break;
                }
            }
            std::ostringstream os;
            os << "  li $a0, " << (int)val << "\n";
            out(os.str());
            return;
        }
        if (auto *s = dynamic_cast<const ExprString *>(&e))
        {
            int id = put_string(s->content);
            std::ostringstream os;
            os << "  la $a0, L.str." << id << "\n"
               << "  li $v0, 4\n" // print_string
               << "  syscall\n";
            out(os.str());
            // Por convenção, após imprimir string, deixamos $a0 indefinido para uso seguinte.
            return;
        }
        if (auto *id = dynamic_cast<const ExprIdent *>(&e))
        {
            auto *v = lookup(id->name);
            // (semântico já garantiu existência)
            emit_load_var_to_a0(*v);
            return;
        }
        if (auto *b = dynamic_cast<const ExprBinary *>(&e))
        {
            emit_expr_bin(*b);
            return;
        }
        if (auto *g = dynamic_cast<const ExprGroup *>(&e))
        {
            emit_expr_group(*g);
            return;
        }
        if (auto *b = dynamic_cast<const ExprBool *>(&e))
        {
            // Verdade = 0xFF, Mentira = 0x00
            int val = b->value ? 0xFF : 0x00;
            std::ostringstream os;
            os << "  li $a0, " << val << "\n";
            out(os.str());
            return;
        }
        // fallback
        ln("  li $a0, 0   # <unknown expr>");
    }

    // ---------- Statements ----------

    /**
     * Emite código para uma declaração.
     *
     * - Declara a variável no escopo atual.
     * - Se houver inicialização (Tipo nome <- Expr):
     *   - Emite o código da expressão em $a0.
     *   - Armazena o valor de $a0 na variável usando emit_store_a0_to_var.
     */
    void Codegen::emit_decl(const StmtDecl &s)
    {
        // reservar espaço e registrar offset
        VarInfo &v = declare(s.name, s.type);

        // se houver inicialização (Tipo nome <- Expr),
        // avalia expressão em $a0 e armazena no slot da variável
        if (s.init)
        {
            emit_expr(*s.init); // resultado em $a0
            emit_store_a0_to_var(v, s.init->inferred);
        }
    }

    /**
     * Emite código para uma atribuição.
     *
     *
     * - Procura a variável; o semântico já garantiu que existe.
     * - Emite o código da expressão do lado direito (RHS) para $a0.
     * - Armazena o valor de $a0 na variável usando emit_store_a0_to_var.
     */
    void Codegen::emit_assign(const StmtAssign &s)
    {
        auto *v = lookup(s.name);
        // avalia RHS em $a0
        emit_expr(*s.value);
        // store no offset adequado
        emit_store_a0_to_var(*v, s.value->inferred);
    }

    /**
     * Emite código para uma instrução de impressão.
     *
     * - Para cada argumento na lista:
     *   - Se a expressão (ou subexpressões) contiver string, trata '+' como concatenação, chamando emit_print_concat.
     *   - Caso contrário, emite a expressão normalmente com emit_print_expr.
     */
    void Codegen::emit_print(const StmtPrint &s)
    {
        for (auto const &e : s.args)
        {
            // Se a expressão (ou subexpressões) contiver string,
            // tratamos '+' como concatenação, NÃO como soma numérica.
            if (expr_has_string(*e))
            {
                emit_print_concat(*e);
            }
            else
            {
                // Sem strings: imprime normalmente (número, char, etc.)
                emit_print_expr(*e);
            }
        }
    }

    /**
     * Emite código para uma instrução if-then-else.
     *
     * - Gera rótulos únicos para else e end.
     * - Emite o código da condição para $a0, verifica se é zero (falso).
     * - Se falso, salta para o rótulo else.
     * - Emite o corpo do then dentro de um novo escopo.
     * - Após o then, salta para o rótulo end.
     * - Emite o rótulo else e o corpo do else dentro de um novo escopo.
     * - Emite o rótulo end.
     */
    void Codegen::emit_if(const StmtIf &s)
    {
        std::string Lelse = new_label("else");
        std::string Lend = new_label("endif");

        // cond: $a0 != 0?
        emit_expr(*s.cond);
        ln("  beq $a0, $zero, " + Lelse);
        // then
        enter_scope();
        for (auto const &st : s.then_body)
            emit_stmt(*st);
        leave_scope();
        ln("  j " + Lend);
        // else
        ln(Lelse + ":");
        enter_scope();
        for (auto const &st : s.else_body)
            emit_stmt(*st);
        leave_scope();
        // end
        ln(Lend + ":");
    }

    /**
     * Emite código para uma instrução while.
     *
     * - Gera rótulos únicos para a condição e o fim do laço.
     * - Emite o rótulo da condição.
     * - Emite o código da condição para $a0, verifica se é zero (falso).
     * - Se falso, salta para o rótulo do fim do laço.
     * - Emite o corpo do laço dentro de um novo escopo.
     * - Após o corpo, salta de volta para o rótulo da condição.
     * - Emite o rótulo do fim do laço.
     */
    void Codegen::emit_while(const StmtWhile &s)
    {
        std::string Lcond = new_label("while.cond");
        std::string Lend = new_label("while.end");
        ln(Lcond + ":");
        emit_expr(*s.cond);
        ln("  beq $a0, $zero, " + Lend);
        enter_scope();
        for (auto const &st : s.body)
            emit_stmt(*st);
        leave_scope();
        ln("  j " + Lcond);
        ln(Lend + ":");
    }

    /**
     * Emite código para uma instrução for.
     *
     * - Gera rótulos únicos para condição, corpo, incremento e fim.
     * - Declara a variável de iteração no escopo atual.
     * - Inicializa a variável com o valor de begin.
     * - Emite o rótulo da condição:
     *   - Carrega a variável de iteração em $t0.
     *   - Recalcula o valor de end em $t1.
     *   - Se o step for positivo (ou padrão), verifica se i <= end; se negativo, verifica se i >= end.
     *   - Se a condição falhar, salta para o rótulo do fim.
     * - Emite o rótulo do corpo:
     *   - Entra em um novo escopo e emite o corpo do laço.
     *   - Sai do escopo.
     * - Emite o rótulo do incremento:
     *   - Carrega a variável de iteração em $t0.
     *   - Recalcula o valor do step em $t2 (ou usa 1 se não fornecido).
     *   - Atualiza a variável de iteração com i + step.
     *   - Salta de volta para o rótulo da condição.
     * - Emite o rótulo do fim e sai do escopo.
     */
    void Codegen::emit_for(const StmtFor &s)
    {
        // Para i em (begin, end, [step]) { body }
        enter_scope();
        VarInfo &vi = declare(s.var, CfType::Inteiro);

        // init i <- begin
        emit_expr(*s.begin);
        emit_store_a0_to_var(vi, CfType::Inteiro);

        // --- NOVO: detectar se o step é negativo constante ---
        bool descending = false;
        if (s.step.has_value())
        {
            if (auto *lit = dynamic_cast<const ExprInteger *>(s.step->get()))
            {
                // aqui assumo que 'digits' guarda o número com sinal, ex: "-1"
                int val = std::stoi(lit->digits);
                if (val < 0)
                {
                    descending = true;
                }
            }
        }
        // -----------------------------------------------------

        std::string Lcond = new_label("for.cond");
        std::string Lbody = new_label("for.body");
        std::string Linc = new_label("for.inc");
        std::string Lend = new_label("for.end");

        // ---------- condição ----------
        ln(Lcond + ":");

        // carrega i -> $t0
        emit_load_var_to_a0(vi);
        ln("  move $t0, $a0");

        // recomputa end -> $t1
        emit_expr(*s.end);
        ln("  move $t1, $a0");

        if (!descending)
        {
            // passo > 0 (ou default): while (i <= end)
            ln("  slt $a0, $t1, $t0");       // end < i ?
            ln("  bne $a0, $zero, " + Lend); // se i > end, sai
        }
        else
        {
            // passo negativo: while (i >= end)
            ln("  slt $a0, $t0, $t1");       // i < end ?
            ln("  bne $a0, $zero, " + Lend); // se i < end, sai
        }

        // ---------- corpo ----------
        ln(Lbody + ":");
        enter_scope();
        for (auto const &st : s.body)
            emit_stmt(*st);
        leave_scope();

        // ---------- incremento ----------
        ln(Linc + ":");

        // i atual -> $t0
        emit_load_var_to_a0(vi);
        ln("  move $t0, $a0");

        // recomputa step -> $t2 (default 1)
        if (s.step.has_value())
        {
            emit_expr(*(*s.step));
            ln("  move $t2, $a0");
        }
        else
        {
            ln("  li $t2, 1");
        }

        // i = i + step
        ln("  add $t0, $t0, $t2");
        ln("  move $a0, $t0");
        emit_store_a0_to_var(vi, CfType::Inteiro);

        ln("  j " + Lcond);

        // ---------- fim ----------
        ln(Lend + ":");
        leave_scope();
    }

    /**
     * Emite código para um bloco de instruções.
     *
     * - Entra em um novo escopo.
     * - Emite cada instrução do corpo.
     * - Sai do escopo.
     */
    void Codegen::emit_block(const std::vector<StmtPtr> &body)
    {
        enter_scope();
        for (auto const &st : body)
            emit_stmt(*st);
        leave_scope();
    }

    /**
     * Emite código para uma instrução.
     *
     * - Se for StmtDecl, chama emit_decl.
     * - Se for StmtAssign, chama emit_assign.
     * - Se for StmtPrint, chama emit_print.
     * - Se for StmtIf, chama emit_if.
     * - Se for StmtWhile, chama emit_while.
     * - Se for StmtFor, chama emit_for.
     * - Se for StmtBlock, chama emit_block.
     */
    void Codegen::emit_stmt(const Stmt &s)
    {
        if (auto *d = dynamic_cast<const StmtDecl *>(&s))
            return emit_decl(*d);
        if (auto *a = dynamic_cast<const StmtAssign *>(&s))
            return emit_assign(*a);
        if (auto *p = dynamic_cast<const StmtPrint *>(&s))
            return emit_print(*p);
        if (auto *i = dynamic_cast<const StmtIf *>(&s))
            return emit_if(*i);
        if (auto *w = dynamic_cast<const StmtWhile *>(&s))
            return emit_while(*w);
        if (auto *f = dynamic_cast<const StmtFor *>(&s))
            return emit_for(*f);
        if (auto *b = dynamic_cast<const StmtBlock *>(&s))
            return emit_block(b->body);
    }

    /**
     * Emite o código assembly para o programa principal.
     *
     * - Emite o cabeçalho da seção de código (.text) e o rótulo main.
     * - Inicializa o frame-pointer ($fp) com o stack-pointer ($sp).
     * - Entra em um novo escopo.
     * - Emite cada instrução do programa.
     * - Sai do escopo.
     * - Emite o epílogo para sair do programa (syscall exit).
     * - Emite a seção de dados (.rodata) com as strings do pool.
     */
    void Codegen::emit_program(const Program &p)
    {
        // Cabeçalho da seção de código
        ln(".text");
        ln(".globl _start"); // <- torna _start símbolo global (entrypoint)
        ln("_start:");

        // Inicializa explicitamente o stack pointer num endereço alto e alinhado
        // (bom para simuladores tipo cpulator / SPIM)
        ln("  li $sp, 0x7fffeffc    # inicializa stack pointer (simulador)");
        ln("  move $fp, $sp        # frame-pointer = $sp");

        enter_scope();
        for (auto const &it : p.items)
            emit_stmt(*it);
        leave_scope();

        // Epílogo: exit(0)
        ln("  li $v0, 10"); // syscall 10 = exit
        ln("  syscall");
    }

    /**
     * Emite o código assembly completo para o programa.
     *
     * - Limpa os buffers de texto e dados, reseta frame_size_ e label_id_.
     * - Emite o código do programa principal (preenchendo text_ e str_pool_).
     * - Emite a seção .rodata com as strings do pool (preenchendo data_).
     * - Concatena data_ e text_ na ordem correta (.data primeiro) e retorna como string.
     */
    std::string Codegen::emit(const Program &p)
    {
        text_.clear();
        data_.clear(); // <-- limpar também o buffer de dados
        frame_size_ = 0;
        label_id_ = 0;
        scopes_.clear();
        str_pool_.clear();

        // Gera apenas o código (.text) e preenche str_pool_
        emit_program(p);

        // Agora, com o pool de strings preenchido, geramos a .data
        emit_rodata();

        // Monta a saída final: primeiro .data, depois .text
        std::string finalAsm;
        finalAsm.reserve(data_.size() + text_.size());
        finalAsm += data_;
        finalAsm += text_;
        return finalAsm;
    }

    /**
     * Verifica se uma expressão contém alguma string literal.
     *
     * - Se a expressão for ExprString, retorna true.
     * - Se for ExprGroup, verifica a expressão interna.
     * - Se for ExprBinary, verifica ambos os lados (lhs e rhs).
     * - Caso contrário, retorna false.
     */
    bool Codegen::expr_has_string(const Expr &e) const
    {
        if (dynamic_cast<const ExprString *>(&e))
            return true;

        if (auto *g = dynamic_cast<const ExprGroup *>(&e))
            return expr_has_string(*g->inner);

        if (auto *b = dynamic_cast<const ExprBinary *>(&e))
            return expr_has_string(*b->lhs) || expr_has_string(*b->rhs);

        return false;
    }

    /**
     * Emite código para imprimir uma expressão simples.
     *
     * - Se a expressão for uma string literal, reaproveita emit_expr, que já faz syscall 4.
     * - Caso contrário, emite a expressão em $a0.
     *   - Se o tipo inferido for Caractere, usa print_char (syscall 11).
     *   - Caso contrário (Inteiro ou Logico), usa print_int (syscall 1).
     */
    void Codegen::emit_print_expr(const Expr &e)
    {
        // String literal pura: reaproveita emit_expr, que já faz syscall 4
        if (dynamic_cast<const ExprString *>(&e))
        {
            emit_expr(e); // la $a0, L.str.X; li $v0, 4; syscall
            return;
        }

        // Caso geral: numérico / lógico / caractere
        emit_expr(e); // resultado em $a0

        if (e.inferred == CfType::Caractere)
        {
            ln("  li $v0, 11"); // print_char
        }
        else
        {
            ln("  li $v0, 1"); // print_int (Inteiro ou Logico)
        }
        ln("  syscall");
    }

    /**
     * Emite código para imprimir uma expressão com concatenação.
     *
     * - Se a expressão for uma concatenação com '+', e algum dos lados for string,
     *   quebra a expressão em partes e emite cada parte separadamente.
     * - Caso contrário, emite a expressão como uma única unidade.
     *
     */
    void Codegen::emit_print_concat(const Expr &e)
    {
        // Se for concatenação com '+' e tiver string em algum lado,
        // não avaliamos como soma: quebramos em partes.
        if (auto *b = dynamic_cast<const ExprBinary *>(&e))
        {
            if (b->op == BinOp::Add &&
                (expr_has_string(*b->lhs) || expr_has_string(*b->rhs)))
            {
                emit_print_concat(*b->lhs);
                emit_print_concat(*b->rhs);
                return;
            }
        }

        // Caso contrário, imprime como expressão simples
        emit_print_expr(e);
    }

}
