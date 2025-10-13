#include "cf/lexer/token.hpp"

namespace cf {

    /**
     * Converte um TokenKind em sua representação string para depuração.
     */
    const char* to_string(TokenKind k) {
        switch (k) {
            case TokenKind::End: return "End";
            case TokenKind::Invalid: return "Invalid";
            case TokenKind::KwInteiro: return "Inteiro";
            case TokenKind::KwLogico: return "Logico";
            case TokenKind::KwCaractere: return "Caractere";
            case TokenKind::KwEnquanto: return "Enquanto";
            case TokenKind::KwSe: return "Se";
            case TokenKind::KwSenao: return "Senao";
            case TokenKind::KwPara: return "Para";
            case TokenKind::KwImprimir: return "Imprimir";
            case TokenKind::KwVerdade: return "Verdade";
            case TokenKind::KwMentira: return "Mentira";
            case TokenKind::KwEm: return "Em";
            case TokenKind::LBrace: return "{";
            case TokenKind::RBrace: return "}";
            case TokenKind::LParen: return "(";
            case TokenKind::RParen: return ")";
            case TokenKind::Semicolon: return ";";
            case TokenKind::Comma: return ",";
            case TokenKind::Assign: return "<-";
            case TokenKind::Plus: return "+";
            case TokenKind::Minus: return "-";
            case TokenKind::Star: return "*";
            case TokenKind::Slash: return "/";
            case TokenKind::Percent: return "%";
            case TokenKind::Pow: return "**";
            case TokenKind::Eq: return "=";
            case TokenKind::Ne: return "<>";
            case TokenKind::Gt: return ">";
            case TokenKind::Lt: return "<";
            case TokenKind::Ge: return ">=";
            case TokenKind::Le: return "<=";
            case TokenKind::And: return "&";
            case TokenKind::Or: return "^";
            case TokenKind::Identifier: return "Identifier";
            case TokenKind::Integer: return "Integer";
            case TokenKind::Char: return "Char";
            case TokenKind::String: return "String";
        }
        return "TokenKind?";
    }

}
