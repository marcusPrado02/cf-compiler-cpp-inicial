\
#include "cf/lexer/lexer.hpp"
#include "cf/common/diagnostic.hpp"
#include <cctype>
#include <stdexcept>

namespace cf {

static std::string lower(std::string s){ for(auto& c:s) c = std::tolower((unsigned char)c); return s; }

Lexer::Lexer(std::string source) : src_(std::move(source)) {
    current_ = lex_token(); // prime 'current_' for peek()
}

const Token& Lexer::peek() {
    return current_;
}

Token Lexer::next() {
    Token out = current_;
    if (out.kind != TokenKind::End) {
        current_ = lex_token();
    }
    return out;
}

Token Lexer::make(TokenKind k, std::string lx){
    return Token{k, std::move(lx), pos_};
}

char Lexer::peekc(std::size_t off) const {
    return (i_+off < src_.size()) ? src_[i_+off] : '\0';
}

char Lexer::get(){
    if (i_ >= src_.size()) return '\0';
    char c = src_[i_++];
    if (c == '\n') { pos_.line++; pos_.column = 1; }
    else { pos_.column++; }
    return c;
}

bool Lexer::match(char c){
    if (peekc() == c){ get(); return true; } return false;
}
bool Lexer::match2(char a, char b){ return peekc()==a && peekc(1)==b; }

void Lexer::skip_spaces_and_comments(){
    for(;;){
        while (std::isspace((unsigned char)peekc())) get();
        if (peekc() == '$'){
            // single line OR block
            if (match2('$','$')){ // starts with "$$"
                get(); get(); // consume "$$"
                // consume until "$$"
                while (!(match2('$','$'))){
                    if (peekc() == '\0') break;
                    get();
                }
                if (match2('$','$')){ get(); get(); }
                continue;
            } else {
                // line comment: consume until newline or end
                while (peekc() != '\n' && peekc() != '\0') get();
                continue;
            }
        }
        break;
    }
}

Token Lexer::lex_identifier_or_keyword(){
    std::string lx;
    // identifiers: ONLY letters
    if (!std::isalpha((unsigned char)peekc())){
        return make(TokenKind::Invalid, std::string(1, get()));
    }
    while (std::isalpha((unsigned char)peekc())){
        lx.push_back(get());
    }
    std::string low = lower(lx);
    if (low == "inteiro") return make(TokenKind::KwInteiro, lx);
    if (low == "logico") return make(TokenKind::KwLogico, lx);
    if (low == "caractere") return make(TokenKind::KwCaractere, lx);
    if (low == "enquanto") return make(TokenKind::KwEnquanto, lx);
    if (low == "se") return make(TokenKind::KwSe, lx);
    if (low == "senao" || low == "senão") return make(TokenKind::KwSenao, lx);
    if (low == "para") return make(TokenKind::KwPara, lx);
    if (low == "imprimir") return make(TokenKind::KwImprimir, lx);
    if (low == "verdade") return make(TokenKind::KwVerdade, lx);
    if (low == "mentira") return make(TokenKind::KwMentira, lx);
    return make(TokenKind::Identifier, lx);
}

Token Lexer::lex_number(){
    std::string lx;
    while (std::isdigit((unsigned char)peekc())) lx.push_back(get());
    return make(TokenKind::Integer, lx);
}

Token Lexer::lex_string(){
    std::string lx; get(); // consume opening "
    while (peekc() && peekc()!='"'){
        if (peekc()=='\\'){ lx.push_back(get()); if (peekc()) lx.push_back(get()); }
        else lx.push_back(get());
    }
    if (peekc()=='"'){ get(); }
    return make(TokenKind::String, lx);
}

Token Lexer::lex_char(){
    std::string lx; get(); // consume opening '
    char c = get();
    if (c=='\\'){ lx.push_back(c); lx.push_back(get()); }
    else lx.push_back(c);
    if (peekc()=='\'') get();
    return make(TokenKind::Char, lx);
}

Token Lexer::lex_token(){
    skip_spaces_and_comments();
    char c = peekc();
    if (c == '\0') return make(TokenKind::End, "");

    // punctuation & multi-char ops first
    if (match2('<','-')){ get(); get(); return make(TokenKind::Assign, "<-"); }
    if (match2('*','*')){ get(); get(); return make(TokenKind::Pow, "**"); }
    if (match2('<','>')){ get(); get(); return make(TokenKind::Ne, "<>"); }
    if (match2('>','=')){ get(); get(); return make(TokenKind::Ge, ">="); }
    if (match2('<','=')){ get(); get(); return make(TokenKind::Le, "<="); }

    switch (c){
        case '{': get(); return make(TokenKind::LBrace, "{");
        case '}': get(); return make(TokenKind::RBrace, "}");
        case '(': get(); return make(TokenKind::LParen, "(");
        case ')': get(); return make(TokenKind::RParen, ")");
        case ';': get(); return make(TokenKind::Semicolon, ";");
        case ',': get(); return make(TokenKind::Comma, ",");
        case '+': get(); return make(TokenKind::Plus, "+");
        case '-': get(); return make(TokenKind::Minus, "-");
        case '*': get(); return make(TokenKind::Star, "*");
        case '/': get(); return make(TokenKind::Slash, "/");
        case '%': get(); return make(TokenKind::Percent, "%");
        case '=': get(); return make(TokenKind::Eq, "=");
        case '>': get(); return make(TokenKind::Gt, ">");
        case '<': get(); return make(TokenKind::Lt, "<");
        case '&': get(); return make(TokenKind::And, "&");
        case '^': get(); return make(TokenKind::Or, "^");
        case '"': return lex_string();
        case '\'': return lex_char();
        default:
            if (std::isalpha((unsigned char)c)) return lex_identifier_or_keyword();
            if (std::isdigit((unsigned char)c)) return lex_number();
            // unknown
            get();
            return make(TokenKind::Invalid, std::string(1,c));
    }
}

}
