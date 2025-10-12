#pragma once
#include <string>
#include <vector>
#include "cf/lexer/token.hpp"

namespace cf {

class Lexer {
public:
    explicit Lexer(std::string source);
    Token next();           // retorna atual e avança
    const Token& peek();    // lookahead atual
    bool eof() const { return current_.kind == TokenKind::End; }
private:
    // Núcleo do lexer
    Token lex_token();
    void skip_spaces_and_comments();
    Token lex_identifier_or_keyword();
    Token lex_number();
    Token lex_string();
    Token lex_char();

    // utilitários
    Token make(TokenKind k, std::string lx);
    bool match(char c);
    bool starts_with(const char* s) const;
    char get();
    char peekc(std::size_t off = 0) const;
    void unread();
    std::string current_line_text() const;

    /**
     * fonte original (string imutável)
     */
    std::string src_;
    /**
     * índice do caractere atual no fonte
     */
    std::size_t i_{0};
    /**
     * posição atual (linha/coluna)
     */
    Position pos_{};
    /**
     * token atual (lookahead de tokens)
     */
    Token current_{TokenKind::Invalid, "", {}};
};

}
