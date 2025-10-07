#pragma once
#include <string>
#include <vector>
#include "cf/lexer/token.hpp"

namespace cf {

class Lexer {
public:
    explicit Lexer(std::string source);
    Token next();
    const Token& peek();
    bool eof() const { return current_.kind == TokenKind::End; }
private:
    Token lex_token();
    void skip_spaces_and_comments();
    Token lex_identifier_or_keyword();
    Token lex_number();
    Token lex_string();
    Token lex_char();
    Token make(TokenKind k, std::string lx);
    bool match(char c);
    bool match2(char a, char b);
    char get();
    char peekc(std::size_t off = 0) const;
    void newline();
    std::string src_;
    std::size_t i_{0};
    Position pos_{};
    Token current_{TokenKind::Invalid, "", {}};
};

}
