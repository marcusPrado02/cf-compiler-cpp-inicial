#include "cf/codegen/codegen.hpp"
#include <sstream>
#include <iomanip>

namespace cf {

  // ---------- Escopos e alocação ----------

  /**
   * Entra em um novo escopo.
   */
  void Codegen::enter_scope() {
      scopes_.emplace_back();
  }

  /**
   * Sai do escopo atual.
   * 
   * - Para cada variável no escopo atual, soma seu tamanho para liberar o stack.
   * - Ajusta o frame_size_ e emite a instrução de ajuste do stack pointer (sp).
   * - Remove o escopo atual da pilha de escopos.
   */
  void Codegen::leave_scope() {
      // Ao sair, precisamos "liberar" quaisquer bytes alocados dentro do escopo.
      // Para isso, somamos os tamanhos das variáveis deste escopo e realocamos o stack.
      int to_free = 0;
      for (auto const& kv : scopes_.back()) {
          to_free += kv.second.size;
          // Inteiros foram alinhados a 4 pela alloc; bytes contam literalmente.
          // Como alinhamos sempre antes do inteiro, o somatório empilha corretamente
          // na ordem de declarações.
      }
      if (to_free > 0) {
          frame_size_ -= to_free;
          std::ostringstream os;
          os << "  addi sp, sp, " << to_free << "   # leave-scope free " << to_free << "\n";
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
  VarInfo* Codegen::lookup(const std::string& name) {
      for (int i=(int)scopes_.size()-1; i>=0; --i) {
          auto it = scopes_[i].find(name);
          if (it != scopes_[i].end()) return &it->second;
      }
      return nullptr;
  }

  /**
   * Aloca bytes na pilha, alinhando se necessário.
   * 
   * - Alinha o frame_size_ para 'align' (potência de 2) antes de alocar.
   * - Se mis não for zero, calcula o padding necessário e ajusta o stack pointer (sp).
   *    - Incrementa o frame_size_ pelo número de bytes solicitados.
   *    - Emite a instrução de ajuste do stack pointer (sp).
   * - Incrementa o frame_size_ pelo número de bytes solicitados.
   * - Emite a instrução de ajuste do stack pointer (sp).
   * - Retorna o offset negativo relativo ao frame-pointer (s0) para a nova alocação.
   */
  int Codegen::alloc_bytes_aligned(int bytes, int align) {
      int mis = frame_size_ % align;
      if (mis != 0) {
          int pad = align - mis;
          frame_size_ += pad;
          std::ostringstream os;
          os << "  addi sp, sp, -" << pad << "   # pad align " << align << "\n";
          text_ += os.str();
      }
      frame_size_ += bytes;
      std::ostringstream os;
      os << "  addi sp, sp, -" << bytes << "   # alloc " << bytes << " bytes\n";
      text_ += os.str();
      // offset de uma nova alocação é -frame_size_ relativo a s0
      return -frame_size_;
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
  VarInfo& Codegen::declare(const std::string& name, CfType ty) {
      if (scopes_.empty()) enter_scope();
      auto& top = scopes_.back();
      // tamanho/alinhamento por tipo
      int size = (ty == CfType::Inteiro) ? 4 : 1;
      int align = (ty == CfType::Inteiro) ? 4 : 1;
      int offset = alloc_bytes_aligned(size, align);
      VarInfo v{ty, offset, size};
      auto [it, ok] = top.emplace(name, v);
      if (!ok) {
          // Sombreamento no mesmo escopo (raro aqui, parser já impediria redeclaração)
          // mas vamos simplesmente sobrescrever.
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
  int Codegen::put_string(const std::string& s) {
      // Procura existente
      for (auto const& it : str_pool_) {
          if (it.value == s) return it.labelIndex;
      }
      int idx = (int)str_pool_.size();
      str_pool_.push_back(StrLit{s, idx});
      return idx;
  }

  /**
   * Emite a seção .rodata com todas as strings do pool.
   * 
   * - Se o pool estiver vazio, não faz nada.
   * - Emite a diretiva .section .rodata.
   * - Para cada string no pool, emite um rótulo L.str.<index> e a diretiva .asciz com o valor da string.
   * - Escapa aspas e barras na string para assembly.
   * - Usa \n para novas linhas.
   * - Adiciona a saída ao texto gerado.
   */
  void Codegen::emit_rodata() {
      if (str_pool_.empty()) return;
      ln(".section .rodata");
      for (auto const& it : str_pool_) {
          std::ostringstream os;
          os << "L.str." << it.labelIndex << ":\n"
            << "  .asciz \"";
          // escapar aspas e barras para assembly
          for (char c : it.value) {
              if (c == '\\\\' || c == '\"') os << '\\\\' << c;
              else if (c == '\\n') os << "\\n";
              else os << c;
          }
          os << "\"\n";
          out(os.str());
      }
  }

  // ---------- Util ----------

  /**
   * Gera um novo rótulo único baseado em 'base'.
   * 
   * - Usa um contador interno label_id_ para garantir unicidade.
   * - Incrementa o contador a cada chamada.  
   * - Retorna o rótulo no formato ".L<base>.<id>".
   */
  std::string Codegen::new_label(const std::string& base) {
      std::ostringstream os;
      os << ".L" << base << "." << (label_id_++);
      return os.str();
  }

  /**
   * Normaliza o valor em a0 para 0xFF (true) ou 0x00 (false).
   * 
   * - Usa instruções RISC-V para comparar a0 com zero.
   * - Se a0 for diferente de zero, seta a0 para 1.
   * - Negativar a0 para obter 0xFF.
   * - Aplica AND com 0xFF para garantir que o valor seja 0xFF.
   * - Adiciona a saída ao texto gerado.
   */
  void Codegen::emit_bool_normalize() {
      // Converte a0 para 0xFF/0x00:
      // a0 = (a0 != 0) ? 1 : 0;  a0 = -a0; a0 &= 0xFF
      ln("  snez a0, a0");
      ln("  neg a0, a0");
      ln("  andi a0, a0, 0xFF");
  }

  /**
   * Carrega o valor de uma variável para a0.
   * 
   * - Se a variável for Inteiro, usa lw.
   * - Se for Caractere ou Logico, usa lb.
   * - Adiciona a saída ao texto gerado.
   * - Usa o offset relativo ao frame-pointer (s0).
   */
  void Codegen::emit_load_var_to_a0(const VarInfo& v) {
      std::ostringstream os;
      if (v.type == CfType::Inteiro) {
          os << "  lw a0, " << v.offset << "(s0)\n";
      } else {
          os << "  lb a0, " << v.offset << "(s0)\n";
      }
      out(os.str());
  }

  /**
   * Armazena o valor de a0 em uma variável.
   * 
   * - Se a variável for Inteiro, usa sw.
   * - Senão 
   *    - Se for Logico, normaliza a0 para 0xFF/0x00. 
   *    - Se for Caractere, aplica AND com 0xFF para garantir 1 byte.
   *    - Usa sb para armazenar 1 byte.
   * - Adiciona a saída ao texto gerado.
   */
  void Codegen::emit_store_a0_to_var(const VarInfo& v, CfType rhsTy) {
      std::ostringstream os;
      if (v.type == CfType::Inteiro) {
          os << "  sw a0, " << v.offset << "(s0)\n";
      } else {
          // normaliza lógico/char para 1 byte
          if (v.type == CfType::Logico) {
              emit_bool_normalize();
          } else {
              ln("  andi a0, a0, 0xFF");
          }
          os << "  sb a0, " << v.offset << "(s0)\n";
      }
      out(os.str());
  }

  // ---------- Expressões ----------

  /**
   * Emite código para uma expressão agrupada (parênteses).
   * 
   * - Simplesmente emite o código da expressão interna.
   */
  void Codegen::emit_expr_group(const ExprGroup& e) {
      emit_expr(*e.inner);
  }

  /**
   * Emite código para uma expressão binária.
   * 
   * - Emite o código do lado esquerdo (lhs) para a0, move para t0.
   * - Emite o código do lado direito (rhs) para a0, move para t1.
   * - Dependendo do operador, emite a instrução apropriada:
   *   - Aritméticos (+, -, *, /, %, ^): usa add, sub, mul, div, rem, ou um loop para pow.
   *   - Relacionais (==, !=, >, <, >=, <=): usa seqz, snez, slt, xori conforme necessário.
   *   - Lógicos (&&, ||): trata os operandos como truthy, usa snez e and/xor, normaliza no fim.
   * - O resultado final fica em a0.
   */
  void Codegen::emit_expr_bin(const ExprBinary& b) {
      // lhs -> t0 ; rhs -> a0 ; guarda rhs em t1
      emit_expr(*b.lhs);
      ln("  mv t0, a0");
      emit_expr(*b.rhs);
      ln("  mv t1, a0");

      switch (b.op) {
          // Aritméticos
          case BinOp::Add: ln("  add a0, t0, t1"); break;
          case BinOp::Sub: ln("  sub a0, t0, t1"); break;
          case BinOp::Mul: ln("  mul a0, t0, t1"); break;
          case BinOp::Div: ln("  div a0, t0, t1"); break;
          case BinOp::Mod: ln("  rem a0, t0, t1"); break;
          case BinOp::Pow: {
              // a0 = t0 ** t1 (inteiros, expoente >= 0)
              std::string Lloop = new_label("pow.loop");
              std::string Lend  = new_label("pow.end");
              ln("  li a0, 1          # acc = 1");
              ln("  mv t2, t0         # base");
              ln("  mv t3, t1         # exp");
              ln(Lloop + ":");
              ln("  beqz t3, " + Lend);
              ln("  mul a0, a0, t2");
              ln("  addi t3, t3, -1");
              ln("  j " + Lloop);
              ln(Lend + ":");
              break;
          }

          // Relacionais -> 0/1
          case BinOp::Eq: ln("  sub t2, t0, t1"); ln("  seqz a0, t2"); break;
          case BinOp::Ne: ln("  sub t2, t0, t1"); ln("  snez a0, t2"); break;
          case BinOp::Lt: ln("  slt a0, t0, t1"); break;
          case BinOp::Gt: ln("  slt a0, t1, t0"); break;
          case BinOp::Le: ln("  slt a0, t1, t0"); ln("  xori a0, a0, 1"); break; // !(t1<t0)
          case BinOp::Ge: ln("  slt a0, t0, t1"); ln("  xori a0, a0, 1"); break; // !(t0<t1)

          // Lógicos (&, ^) — tratamos operands como truthy e normalizamos no fim
          case BinOp::And:
              ln("  snez t0, t0");   // t0 = 0/1
              ln("  snez t1, t1");
              ln("  and a0, t0, t1");
              emit_bool_normalize();
              break;
          case BinOp::Or:
              ln("  snez t0, t0");
              ln("  snez t1, t1");
              ln("  xor a0, t0, t1");
              emit_bool_normalize();
              break;
      }
  }

  /**
   * Emite código para uma expressão.
   * 
   * - Se for ExprInteger, carrega o valor imediato em a0.
   * - Se for ExprChar, converte o caractere para seu valor ASCII e carrega em a0.
   * - Se for ExprString, obtém o índice do pool de strings, carrega o endereço em a0, o tamanho em a1, e chama print_str.
   * - Se for ExprIdent, procura a variável e carrega seu valor em a0.
   * - Se for ExprBinary, chama emit_expr_bin.
   * - Se for ExprGroup, chama emit_expr_group.
   * - Se não reconhecido, carrega 0 em a0 como fallback.
   */
  void Codegen::emit_expr(const Expr& e) {
      if (auto* i = dynamic_cast<const ExprInteger*>(&e)) {
          // Inteiros decimais simples
          std::ostringstream os;
          os << "  li a0, " << i->digits << "\n";
          out(os.str());
          return;
      }
      if (auto* ch = dynamic_cast<const ExprChar*>(&e)) {
          // content carrega, por ex, "\\n" ou "a" — aqui só suportamos 1 byte ASCII
          unsigned char val = 0;
          if (ch->content.size() == 1) {
              val = static_cast<unsigned char>(ch->content[0]);
          } else if (ch->content.size() == 2 && ch->content[0] == '\\\\') {
              // escapes simples: \n \t \\ \' "
              switch (ch->content[1]) {
                  case 'n': val = 10; break;
                  case 't': val = 9;  break;
                  case '\\\\': val = '\\\\'; break;
                  case '\'': val = '\''; break;
                  case '\"': val = '\"'; break;
                  default: val = static_cast<unsigned char>(ch->content[1]); break;
              }
          }
          std::ostringstream os;
          os << "  li a0, " << (int)val << "\n";
          out(os.str());
          return;
      }
      if (auto* s = dynamic_cast<const ExprString*>(&e)) {
          int id = put_string(s->content);
          std::ostringstream os;
          os << "  la a0, L.str." << id << "\n"
            << "  li a1, " << (int)s->content.size() << "\n"
            << "  jal print_str\n";
          out(os.str());
          // Por convenção, após imprimir string, deixamos a0 indefinido para uso seguinte.
          return;
      }
      if (auto* id = dynamic_cast<const ExprIdent*>(&e)) {
          auto* v = lookup(id->name);
          // (semântico já garantiu existência)
          emit_load_var_to_a0(*v);
          return;
      }
      if (auto* b = dynamic_cast<const ExprBinary*>(&e)) {
          emit_expr_bin(*b);
          return;
      }
      if (auto* g = dynamic_cast<const ExprGroup*>(&e)) {
          emit_expr_group(*g);
          return;
      }
      // fallback
      ln("  li a0, 0   # <unknown expr>");
  }

  // ---------- Statements ----------

  /**
   * Emite código para uma declaração.
   */
  void Codegen::emit_decl(const StmtDecl& s) {
      // reservar espaço e registrar offset
      declare(s.name, s.type);
  }

  /**
   * Emite código para uma atribuição.
   * 
   * 
   * - Procura a variável; o semântico já garantiu que existe.
   * - Emite o código da expressão do lado direito (RHS) para a0.
   * - Armazena o valor de a0 na variável usando emit_store_a0_to_var.
   */
  void Codegen::emit_assign(const StmtAssign& s) {
      auto* v = lookup(s.name);
      // avalia RHS em a0
      emit_expr(*s.value);
      // store no offset adequado
      emit_store_a0_to_var(*v, s.value->inferred);
  }

  /**
   * Emite código para uma instrução de impressão.
   *
   * - Para cada argumento: 
   *   - Se for ExprString, emite a expressão (já chama print_str).
   *   - Se for ExprChar, emite a expressão e chama print_char.
   *   - Senão (Inteiro ou Logico), emite a expressão e chama print_int.
   */
  void Codegen::emit_print(const StmtPrint& s) {
      // Para cada arg: decide rotina
      for (auto const& e : s.args) {
          if (dynamic_cast<ExprString*>(e.get())) {
              emit_expr(*e); // já chama print_str
          } else if (dynamic_cast<ExprChar*>(e.get())) {
              emit_expr(*e); ln("  jal print_char");
          } else {
              // Inteiro ou Logico — ambos em decimal
              emit_expr(*e); ln("  jal print_int");
          }
      }
  }

  /**
   * Emite código para uma instrução if-then-else.
   * 
   * - Gera rótulos únicos para else e end.
   * - Emite o código da condição para a0, verifica se é zero (falso).
   * - Se falso, salta para o rótulo else.
   * - Emite o corpo do then dentro de um novo escopo.
   * - Após o then, salta para o rótulo end.  
   * - Emite o rótulo else e o corpo do else dentro de um novo escopo.
   * - Emite o rótulo end.
   */
  void Codegen::emit_if(const StmtIf& s) {
      std::string Lelse = new_label("else");
      std::string Lend  = new_label("endif");

      // cond: a0 != 0?
      emit_expr(*s.cond);
      ln("  beqz a0, " + Lelse);
      // then
      enter_scope();
      for (auto const& st : s.then_body) emit_stmt(*st);
      leave_scope();
      ln("  j " + Lend);
      // else
      ln(Lelse + ":");
      enter_scope();
      for (auto const& st : s.else_body) emit_stmt(*st);
      leave_scope();
      // end
      ln(Lend + ":");
  }

  /**
   * Emite código para uma instrução while.
   * 
   * - Gera rótulos únicos para a condição e o fim do laço.
   * - Emite o rótulo da condição.
   * - Emite o código da condição para a0, verifica se é zero (falso).
   * - Se falso, salta para o rótulo do fim do laço.
   * - Emite o corpo do laço dentro de um novo escopo.
   * - Após o corpo, salta de volta para o rótulo da condição.
   * - Emite o rótulo do fim do laço.
   */
  void Codegen::emit_while(const StmtWhile& s) {
      std::string Lcond = new_label("while.cond");
      std::string Lend  = new_label("while.end");
      ln(Lcond + ":");
      emit_expr(*s.cond);
      ln("  beqz a0, " + Lend);
      enter_scope();
      for (auto const& st : s.body) emit_stmt(*st);
      leave_scope();
      ln("  j " + Lcond);
      ln(Lend + ":");
  }

  /**
   * Emite código para uma instrução for.
   * 
   * - Gera rótulos únicos para a condição, o corpo, o incremento e o fim do laço.
   * - Declara a variável do laço como Inteiro no escopo atual.
   * - Emite o código da expressão begin, armazena o valor na variável do laço.
   * - Emite o código da expressão end, move o valor para t1.
   * - Se houver step, emite o código da expressão step, move o valor para t2;
   * senão, carrega 1 em t2.
   * - Emite o rótulo da condição.
   * - Carrega a variável do laço para t0, verifica se t0 > t1 (laço inclusivo i <= end).
   * - Se for maior, salta para o rótulo do fim do laço.
   * - Emite o corpo do laço dentro de um novo escopo.
   * - Emite o rótulo do incremento.
   * - Incrementa t0 por t2, armazena de volta na variável do laço.
   * - Salta de volta para o rótulo da condição.
   * - Emite o rótulo do fim do laço e sai do escopo.
   */
  void Codegen::emit_for(const StmtFor& s) {
      // Para i em (begin, end, [step]) { body }
      // i é variável local do laço (Inteiro)
      enter_scope();
      VarInfo& vi = declare(s.var, CfType::Inteiro);

      // init i <- begin
      emit_expr(*s.begin);
      emit_store_a0_to_var(vi, CfType::Inteiro);

      // compute end -> t1
      emit_expr(*s.end);
      ln("  mv t1, a0");

      // step -> t2 (default 1)
      if (s.step.has_value()) {
          emit_expr(*(*s.step));
          ln("  mv t2, a0");
      } else {
          ln("  li t2, 1");
      }

      std::string Lcond = new_label("for.cond");
      std::string Lbody = new_label("for.body");
      std::string Linc  = new_label("for.inc");
      std::string Lend  = new_label("for.end");

      ln(Lcond + ":");
      // load i -> t0 ; if (i > end) goto end  (laço inclusivo i <= end)
      emit_load_var_to_a0(vi); ln("  mv t0, a0");
      ln("  slt a0, t1, t0");   // a0 = (end < i) ? 1 : 0  => i > end
      ln("  bnez a0, " + Lend);
      // body
      ln(Lbody + ":");
      enter_scope();
      for (auto const& st : s.body) emit_stmt(*st);
      leave_scope();
      // inc
      ln(Linc + ":");
      ln("  add t0, t0, t2"); // i += step
      ln("  mv a0, t0"); emit_store_a0_to_var(vi, CfType::Inteiro);
      ln("  j " + Lcond);
      // end
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
  void Codegen::emit_block(const std::vector<StmtPtr>& body) {
      enter_scope();
      for (auto const& st : body) emit_stmt(*st);
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
  void Codegen::emit_stmt(const Stmt& s) {
      if (auto* d = dynamic_cast<const StmtDecl*>(&s)) return emit_decl(*d);
      if (auto* a = dynamic_cast<const StmtAssign*>(&s)) return emit_assign(*a);
      if (auto* p = dynamic_cast<const StmtPrint*>(&s)) return emit_print(*p);
      if (auto* i = dynamic_cast<const StmtIf*>(&s)) return emit_if(*i);
      if (auto* w = dynamic_cast<const StmtWhile*>(&s)) return emit_while(*w);
      if (auto* f = dynamic_cast<const StmtFor*>(&s)) return emit_for(*f);
      if (auto* b = dynamic_cast<const StmtBlock*>(&s)) return emit_block(b->body);
  }

  /**
   * Emite o programa completo.
   * 
   * - Emite o cabeçalho e prólogo (seção .text, rótulo _start, inicializa frame-pointer).
   * - Entra no escopo global.
   * - Emite cada instrução top-level do programa.
   * - Sai do escopo global (libera alocações).
   * - Emite o epílogo (chama exit(0)).
   * - Emite a seção .rodata com as strings do pool. 
   */
  void Codegen::emit_program(const Program& p) {
      // Header + prologo
      ln(".section .text");
      ln(".globl _start");
      ln("_start:");
      ln("  mv s0, sp             # frame-pointer = sp");
      // Corpo: cada Decl/Stmt top-level
      enter_scope();
      for (auto const& it : p.items) emit_stmt(*it);
      // Sair do escopo global (libera alocações)
      leave_scope();

      // Epílogo: exit(0)
      ln("  li a0, 0");
      ln("  li a7, 93");
      ln("  ecall");

      // Secao de strings
      emit_rodata();
  }

  /**
   * Emite o código assembly completo para o programa.
   * 
   * - Inicializa o texto, frame_size_, label_id_, escopos e pool de strings.
   * - Chama emit_program para gerar o código do programa.
   * - Retorna o texto gerado como uma string.
   */
  std::string Codegen::emit(const Program& p) {
      text_.clear();
      frame_size_ = 0;
      label_id_ = 0;
      scopes_.clear();
      str_pool_.clear();

      emit_program(p);
      return text_;
  }

}
