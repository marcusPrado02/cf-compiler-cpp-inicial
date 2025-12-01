#include "cf/lexer/lexer.hpp"
#include "cf/common/diagnostic.hpp"
#include <cctype>
#include <stdexcept>

namespace cf
{

    /**
     * Converte uma string para minúsculas.
     */
    static std::string lower(std::string s)
    {
        for (auto &c : s)
            c = std::tolower((unsigned char)c);
        return s;
    }

    /**
     * Converte uma string para maiúsculas.
     */
    static std::string upper(std::string s)
    {
        for (auto &c : s)
            c = std::toupper((unsigned char)c);
        return s;
    }

    /**
     * Construtor do analisador léxico.
     */
    Lexer::Lexer(std::string source) : src_(std::move(source))
    {
        current_ = lex_token(); // prime current_ para peek()
    }

    /**
     * Retorna o token atual sem avançar. (Lookahead)
     */
    const Token &Lexer::peek() { return current_; }

    /**
     * Retorna o token atual e avança para o próximo.
     * Se já estiver no fim do arquivo, retorna End repetidamente.
     * (Consome um token)
     */
    Token Lexer::next()
    {
        // ---------------------------------------
        // IGNORAR COMENTÁRIOS /* ... */
        // ---------------------------------------
        if (peekc() == '/' && peekc(1) == '*')
        {
            get();
            get(); // consome /*

            while (true)
            {
                if (peekc() == '\0')
                    break; // EOF sem fechar comentário
                if (peekc() == '*' && peekc(1) == '/')
                {
                    get();
                    get(); // consome */
                    break;
                }
                get(); // avança dentro do comentário
            }
            return next(); // volta ao fluxo normal
        }
        Token out = current_;
        if (out.kind != TokenKind::End)
        {
            current_ = lex_token();
        }
        return out;
    }

    /**
     * Cria um token com o lexema e tipo fornecidos, na posição atual. (factory)
     */
    Token Lexer::make(TokenKind k, std::string lx)
    {
        return Token{k, std::move(lx), pos_};
    }

    /**
     * Retorna o caractere na posição atual + off, ou '\0' se no fim do arquivo.
     * (lookahead de caracteres brutos do fonte)
     *
     * off = 0 => caractere atual
     * off = 1 => próximo caractere
     * off = 2 => próximo próximo caractere
     * etc.
     */
    char Lexer::peekc(std::size_t off) const
    {
        return (i_ + off < src_.size()) ? src_[i_ + off] : '\0';
    }

    /**
     * "Deslê" um caractere, voltando uma posição. (não retrocede linha)
     *
     * Se já estiver no início do arquivo, não faz nada.
     */
    void Lexer::unread()
    {
        if (i_ == 0)
            return;
        --i_;
        if (pos_.column > 1)
            --pos_.column;
    }

    /**
     * Lê o caractere na posição atual e avança uma posição.
     * Retorna '\0' se no fim do arquivo.
     * (consome um caractere)
     *
     * - Se o estado _i estiver no fim do arquivo, retorna '\0'.
     * - Se o caractere lido for '\n', incrementa a linha e zera a coluna.
     * - Caso contrário, apenas incrementa a coluna.
     */
    char Lexer::get()
    {
        if (i_ >= src_.size())
            return '\0';
        char c = src_[i_++];
        if (c == '\n')
        {
            pos_.line++;
            pos_.column = 1;
        }
        else
        {
            pos_.column++;
        }
        return c;
    }

    /**
     * Se o caractere atual for igual ao fornecido, consome-o e retorna true.
     * Caso contrário, não consome nada e retorna false.
     */
    bool Lexer::match(char c)
    {
        if (peekc() == c)
        {
            get();
            return true;
        }
        return false;
    }

    /**
     * Retorna true se a sequência de caracteres a partir da posição atual
     * começar com a string fornecida (s) a partir do caractere atual (_i_).
     */
    bool Lexer::starts_with(const char *s) const
    {
        std::size_t j = 0;
        while (s[j])
        {
            if (peekc(j) != s[j])
                return false;
            ++j;
        }
        return true;
    }

    /**
     * Retorna o texto da linha atual (onde está o índice i_).
     *
     * - Atribui start como o início da linha (ou 0 se for a primeira linha).
     * - Enquanto start > 0 e o caractere anterior não for '\n', decrementa start.
     * - Atribui end como o índice atual i_.
     * - Enquanto end < tamanho da fonte e o caractere em end não for '\n', incrementa end.
     * - Retorna a substring do fonte entre start e end (Representa a linha atual).
     */
    std::string Lexer::current_line_text() const
    {
        std::size_t start = i_;
        while (start > 0 && src_[start - 1] != '\n')
            --start;
        std::size_t end = i_;
        while (end < src_.size() && src_[end] != '\n')
            ++end;
        return src_.substr(start, end - start);
    }

    /**
     * Ignora espaços em branco e comentários.
     *
     * - Enquanto o caractere atual for espaço em branco, consome-o.
     * - Se o caractere atual for '$':
     *   - Se o próximo caractere for '$', consome ambos e ignora até encontrar outro '$$' ou o fim do arquivo.
     *   - Caso contrário, ignora até o final da linha ou o fim do arquivo.
     * - Repete o processo até que o caractere atual não seja espaço em branco ou '$'.
     * - Se um comentário de bloco '$$' não for fechado.
     */
    void Lexer::skip_spaces_and_comments()
    {
        for (;;)
        {

            // 1) Espaços em branco
            while (std::isspace((unsigned char)peekc()))
                get();

            // 2) Comentários iniciados por '$'
            if (peekc() == '$')
            {
                if (peekc(1) == '$')
                {
                    // Comentário de bloco: $$ ... $$
                    get();
                    get(); // "$$"
                    while (true)
                    {
                        if (peekc() == '\0')
                        {
                            Diagnostic d;
                            d.phase = "lexical";
                            d.pos = pos_;
                            d.message = "comentário de bloco '$$' não fechado";
                            d.line_text = current_line_text();
                            throw CompileError(d);
                        }
                        if (peekc() == '$' && peekc(1) == '$')
                        {
                            get();
                            get(); // fecha "$$"
                            break;
                        }
                        get();
                    }
                    continue;
                }
                else
                {
                    // Comentário de linha: $ ... \n
                    while (peekc() != '\n' && peekc() != '\0')
                        get();
                    continue;
                }
            }
            // Nada mais para pular
            break;
        }
    }

    /**
     * Lê um identificador(apenas letras) ou palavra-chave.
     *
     * - Se o caractere atual não for uma letra, lança um erro de compilação.
     * - Enquanto o caractere atual for uma letra, consome-o e adiciona ao lexema.
     * - Converte o lexema para minúsculas para comparação.
     * - Compara o lexema com as palavras-chave conhecidas:
     *   - Se corresponder a uma palavra-chave, retorna um token dessa palavra-chave.
     *   - Caso contrário, retorna um token de identificador com o lexema original.
     */
    Token Lexer::lex_identifier_or_keyword()
    {
        std::string lx;

        // 1) Primeiro caractere deve ser "letra" no sentido mais amplo:
        //    - isalpha ASCII
        //    - OU qualquer byte >= 128 (parte de caractere acentuado em UTF-8)
        unsigned char c0 = static_cast<unsigned char>(peekc());
        if (!std::isalpha(c0) && c0 < 0x80)
        {
            char bad = get();
            Diagnostic d;
            d.phase = "lexical";
            d.pos = pos_;
            d.message = std::string("símbolo inválido '") + bad + "'";
            d.line_text = current_line_text();
            throw CompileError(d);
        }

        // 2) Consumir sequência de "letras" (ASCII ou bytes UTF-8 >= 128)
        while (true)
        {
            unsigned char c = static_cast<unsigned char>(peekc());
            if (c == '\0')
                break;
            if (std::isalpha(c) || c >= 0x80)
            {
                lx.push_back(get());
            }
            else
            {
                break;
            }
        }

        // 3) Case-folding só para ASCII; bytes >=128 ficam como estão.
        std::string low = lower(lx);

        // 4) Palavras-chave (com e sem acento)
        if (low == "inteiro")
            return make(TokenKind::KwInteiro, lx);

        // aceita "Logico" e "Lógico"
        if (low == "logico" || low == "lógico")
            return make(TokenKind::KwLogico, lx);

        if (low == "caractere")
            return make(TokenKind::KwCaractere, lx);
        if (low == "enquanto")
            return make(TokenKind::KwEnquanto, lx);
        if (low == "se")
            return make(TokenKind::KwSe, lx);

        // aceita "Senao" e "Senão"
        if (low == "senao" || low == "senão")
            return make(TokenKind::KwSenao, lx);

        if (low == "para")
            return make(TokenKind::KwPara, lx);
        if (low == "imprimir")
            return make(TokenKind::KwImprimir, lx);
        if (low == "verdade")
            return make(TokenKind::KwVerdade, lx);
        if (low == "mentira")
            return make(TokenKind::KwMentira, lx);
        if (low == "em")
            return make(TokenKind::KwEm, lx);

        // 5) Caso contrário, é identificador (permitindo acentos, se quiser usar)
        return make(TokenKind::Identifier, lx);
    }

    /**
     * Lê um número inteiro (sequência de dígitos).
     *
     * - Enquanto o caractere atual for um dígito, consome-o e adiciona ao lexema.
     * - Retorna um token do tipo Integer com o lexema lido.
     */
    Token Lexer::lex_number()
    {
        std::string lx;
        while (std::isdigit((unsigned char)peekc()))
            lx.push_back(get());
        return make(TokenKind::Integer, lx);
    }

    /**
     * Lê uma string entre aspas duplas, suportando escapes.
     *
     * - Consome o caractere de abertura '"'.
     * - Enquanto o próximo caractere não for o caractere de fechamento '"' ou o fim do arquivo:
     *   - Se o caractere atual for '\0', lança um erro de compilação por string não terminada.
     *   - Se o caractere atual for '"'", consome-o e encerra a leitura da string.
     *   - Se o caractere atual for '\\', consome-o e o próximo caractere (escape).
     *   - Caso contrário, consome o caractere atual e o adiciona ao lexema.
     * - Consome o caractere de fechamento '"'.
     * - Retorna um token do tipo String com o lexema lido.
     */
    Token Lexer::lex_string()
    {
        std::string lx;
        get(); // abre "
        while (true)
        {
            char c = peekc();
            if (c == '\0')
            {
                Diagnostic d;
                d.phase = "lexical";
                d.pos = pos_;
                d.message = "string não terminada";
                d.line_text = current_line_text();
                throw CompileError(d);
            }
            if (c == '"')
            {
                get();
                break;
            }
            if (c == '\\')
            {
                lx.push_back(get()); // '\\'
                char n = peekc();
                if (n == '\0')
                {
                    Diagnostic d;
                    d.phase = "lexical";
                    d.pos = pos_;
                    d.message = "escape inválido no final de string";
                    d.line_text = current_line_text();
                    throw CompileError(d);
                }
                lx.push_back(get()); // caractere escapado
            }
            else
            {
                lx.push_back(get());
            }
        }
        return make(TokenKind::String, lx);
    }

    /**
     * Lê um caractere entre aspas simples, suportando escapes.
     *
     * - Consome o caractere de abertura '\''.
     * - Se o próximo caractere for '\0', lança um erro de compilação
     * por caractere não terminado.
     * - Se o próximo caractere for '\\', consome-o e o próximo caractere
     * (escape) e o adiciona ao conteúdo.
     * - Caso contrário, consome o próximo caractere e o adiciona ao conteúdo.
     * - Se o próximo caractere não for o caractere de fechamento '\'',
     * lança um erro de compilação por caractere inválido.
     * - Consome o caractere de fechamento '\''.
     * - Retorna um token do tipo Char com o conteúdo lido.
     */
    Token Lexer::lex_char()
    {
        std::string content;
        get(); // abre '
        char c = peekc();
        if (c == '\0')
        {
            Diagnostic d;
            d.phase = "lexical";
            d.pos = pos_;
            d.message = "caractere não terminado";
            d.line_text = current_line_text();
            throw CompileError(d);
        }
        if (c == '\\')
        {                             // escape
            content.push_back(get()); // '\\'
            char e = peekc();
            if (e == '\0' || e == '\n')
            {
                Diagnostic d;
                d.phase = "lexical";
                d.pos = pos_;
                d.message = "escape inválido em caractere";
                d.line_text = current_line_text();
                throw CompileError(d);
            }
            content.push_back(get());
        }
        else
        {
            content.push_back(get());
        }
        if (peekc() != '\'')
        {
            Diagnostic d;
            d.phase = "lexical";
            d.pos = pos_;
            d.message = "caractere deve conter exatamente 1 símbolo";
            d.line_text = current_line_text();
            throw CompileError(d);
        }
        get(); // fecha '
        return make(TokenKind::Char, content);
    }

    /**
     * Lê o próximo token do fonte, ignorando espaços e comentários.
     * Retorna um token do tipo apropriado.
     *
     * - Ignora espaços em branco e comentários.
     * - Se o caractere atual for '\0', retorna um token End.
     * - Verifica operadores de 2 caracteres primeiro (como '<-', '**', '<>', etc.).
     * - Verifica símbolos de 1 caractere (como '{', '}', '(', etc.).
     * - Se o caractere atual for '"', chama lex_string() para ler uma string.
     * - Se o caractere atual for '\'', chama lex_char() para ler um caractere.
     * - Se o caractere atual for uma letra, chama lex_identifier_or_keyword().
     * - Se o caractere atual for um dígito, chama lex_number().
     * - Se o caractere atual não corresponder a nenhum caso conhecido, lança um erro
     * de compilação.
     */
    Token Lexer::lex_token()
    {
        skip_spaces_and_comments();
        char c = peekc();
        if (c == '\0')
            return make(TokenKind::End, "");

        // operadores de 2 chars primeiro
        if (starts_with("<-"))
        {
            get();
            get();
            return make(TokenKind::Assign, "<-");
        }
        if (starts_with("**"))
        {
            get();
            get();
            return make(TokenKind::Pow, "**");
        }
        if (starts_with("<>"))
        {
            get();
            get();
            return make(TokenKind::Ne, "<>");
        }
        if (starts_with(">="))
        {
            get();
            get();
            return make(TokenKind::Ge, ">=");
        }
        if (starts_with("<="))
        {
            get();
            get();
            return make(TokenKind::Le, "<=");
        }

        switch (c)
        {
        case '{':
            get();
            return make(TokenKind::LBrace, "{");
        case '}':
            get();
            return make(TokenKind::RBrace, "}");
        case '(':
            get();
            return make(TokenKind::LParen, "(");
        case ')':
            get();
            return make(TokenKind::RParen, ")");
        case ';':
            get();
            return make(TokenKind::Semicolon, ";");
        case ',':
            get();
            return make(TokenKind::Comma, ",");
        case '+':
            get();
            return make(TokenKind::Plus, "+");
        case '-':
            get();
            return make(TokenKind::Minus, "-");
        case '*':
            get();
            return make(TokenKind::Star, "*");
        case '/':
            get();
            return make(TokenKind::Slash, "/");
        case '%':
            get();
            return make(TokenKind::Percent, "%");
        case '=':
            get();
            return make(TokenKind::Eq, "=");
        case '>':
            get();
            return make(TokenKind::Gt, ">");
        case '<':
            get();
            return make(TokenKind::Lt, "<");
        case '&':
            get();
            return make(TokenKind::And, "&");
        case '|':
            get();
            return make(TokenKind::Or, "|");
        case '^':
            get();
            return make(TokenKind::Or, "^");
        case '"':
            return lex_string();
        case '\'':
            return lex_char();
        default:
            if (std::isalpha((unsigned char)c))
                return lex_identifier_or_keyword();
            if (std::isdigit((unsigned char)c))
                return lex_number();
            // símbolo desconhecido => erro
            {
                char bad = get();
                Diagnostic d;
                d.phase = "lexical";
                d.pos = pos_;
                d.message = std::string("símbolo inválido '") + bad + "'";
                d.line_text = current_line_text();
                throw CompileError(d);
            }
        }
    }

}
