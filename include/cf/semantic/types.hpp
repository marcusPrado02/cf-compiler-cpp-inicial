#pragma once
#include "cf/ir/ast.hpp"

namespace cf {

    /**
     * Tipos básicos da linguagem CF.
     * 
     * - Inteiro: números inteiros (int)
     * - Logico: valores booleanos (true/false)
     * - Caractere: caracteres únicos (char)
     */
    inline bool is_int(CfType t)      { return t == CfType::Inteiro; }
    inline bool is_char(CfType t)     { return t == CfType::Caractere; }
    inline bool is_logical(CfType t)  { return t == CfType::Logico; }

    /**
     * Promoção implícita: Caractere -> Inteiro (para aritméticos e relacionais)
     */
    inline CfType promote_if_needed(CfType t) {
        if (t == CfType::Caractere) return CfType::Inteiro;
        return t;
    }

    /**
     * Resultado dos operadores binários
     */
    inline CfType result_arith(CfType lhs, CfType rhs) {
        lhs = promote_if_needed(lhs); rhs = promote_if_needed(rhs);
        if (is_int(lhs) && is_int(rhs)) return CfType::Inteiro;
        return CfType::Desconhecido;
    }

    /**
     * Resultado dos operadores relacionais (sempre Lógico se válido)
     */
    inline CfType result_rel(CfType lhs, CfType rhs) {
        lhs = promote_if_needed(lhs); rhs = promote_if_needed(rhs);
        if (is_int(lhs) && is_int(rhs)) return CfType::Logico;
        return CfType::Desconhecido;
    }

    /**
     * Resultado dos operadores lógicos (sempre Lógico se válido)
     */
    inline CfType result_logic(CfType lhs, CfType rhs) {
        if (is_logical(lhs) && is_logical(rhs)) return CfType::Logico;
        return CfType::Desconhecido;
    }

    /**
     * Compatibilidade de atribuição:
     * - exatamente igual
     * - Char -> Int permitido
     */
    inline bool assign_compatible(CfType dst, CfType src) {
        if (dst == src) return true;
        if (dst == CfType::Inteiro && src == CfType::Caractere) return true;
        return false;
    }

}
