#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>
#include "cf/ir/ast.hpp"

namespace cf
{

    // Info de variável alocada na pilha
    struct VarInfo
    {
        CfType type{CfType::Desconhecido};
        int offset{};  // offset relativo ao frame-pointer (s0) — negativo
        int size{};    // 4 (Inteiro) ou 1 (Caractere/Logico)
        int padding{}; // bytes de padding para alinhamento
    };

    class Codegen
    {
    public:
        // Gera assembly RISC-V (RV32IM) para o programa
        std::string emit(const Program &p);

    private:
        // ---- Infra de emissão ----
        void out(const std::string &s) { text_ += s; }
        void ln(const std::string &s)
        {
            text_ += s;
            text_ += "\n";
        }

        // ---- Escopos/stack-frame ----
        void enter_scope();
        void leave_scope();
        VarInfo *lookup(const std::string &name);
        // declara no escopo atual: retorna VarInfo com offset calculado
        VarInfo &declare(const std::string &name, CfType ty);
        // util de alocação (alinha para 4 quando necessário)
        int alloc_bytes_aligned(int bytes, int align, int &outPad);

        // ---- Emissão por nó ----
        void emit_program(const Program &p);
        void emit_stmt(const Stmt &s);
        void emit_block(const std::vector<StmtPtr> &body);
        void emit_decl(const StmtDecl &s);
        void emit_assign(const StmtAssign &s);
        void emit_print(const StmtPrint &s);
        void emit_if(const StmtIf &s);
        void emit_while(const StmtWhile &s);
        void emit_for(const StmtFor &s);

        // ---- Expressões -> resultado em a0 ----
        void emit_expr(const Expr &e);
        void emit_expr_bin(const ExprBinary &e);
        void emit_expr_group(const ExprGroup &e);

        // ---- Helpers (labels, bool normalize, loads/stores, strings) ----
        std::string new_label(const std::string &base);
        void emit_bool_normalize(); // a0: qualquer -> 0xFF/0x00
        void emit_load_var_to_a0(const VarInfo &v);
        void emit_store_a0_to_var(const VarInfo &v, CfType rhsTy);
        // pool de strings (.rodata)
        int put_string(const std::string &s);
        void emit_rodata();
        bool expr_has_string(const Expr &e) const;
        void emit_print_expr(const Expr &e);
        void emit_print_concat(const Expr &e);

    private:
        // saída
        std::string text_;
        std::string data_;

        // frame-pointer base e alocação dinâmica
        int frame_size_{0}; // bytes alocados desde a entrada do _start
        int label_id_{0};

        // pilha de escopos de variáveis (name -> VarInfo)
        std::vector<std::unordered_map<std::string, VarInfo>> scopes_{};

        // tabela de strings para .rodata
        struct StrLit
        {
            std::string value;
            int labelIndex;
        };
        std::vector<StrLit> str_pool_;
    };

}
