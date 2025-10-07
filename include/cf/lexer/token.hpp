#pragma once
#include <string>
#include <variant>
#include "cf/common/position.hpp"

namespace cf {

enum class TokenKind {
    End, Invalid,
    // Keywords
    KwInteiro, KwLogico, KwCaractere, KwEnquanto, KwSe, KwSenao, KwPara, KwImprimir,
    KwVerdade, KwMentira,
    // Symbols
    LBrace, RBrace, LParen, RParen, Semicolon, Comma, Assign, // '<-'
    // Operators
    Plus, Minus, Star, Slash, Percent, Pow, // '**' as Pow
    Eq, Ne, Gt, Lt, Ge, Le, And, Or, // '&' '^'
    // Literals / id
    Identifier, Integer, Char, String
};

struct Token {
    TokenKind kind{TokenKind::Invalid};
    std::string lexeme{};
    Position pos{};
};

const char* to_string(TokenKind k);

}
