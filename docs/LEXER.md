# 🔤 Módulo Léxico (`cf/lexer`)

O analisador léxico (lexer / scanner) é a **primeira fase do compilador CF**, responsável por transformar o texto-fonte em uma sequência padronizada e estruturada de **tokens**, que servirão como entrada para o parser.  
Sem um lexer robusto, nenhuma etapa posterior do compilador pode operar de forma correta, pois toda a compreensão sintática e semântica depende diretamente da tokenização.

Esta versão da documentação está **atualizada** com a implementação mais recente do `Lexer`, incluindo:

- suporte a identificadores com **acentos** (bytes `>= 0x80`);
- aceitação de palavras-chave com e sem acento (`Logico` / `Lógico`, `Senao` / `Senão`);
- suporte adicional a comentários no estilo `/* ... */`;
- operadores lógicos `&`, `^` e também `|` como sinônimo de **OR**;
- comportamento real de strings e posição de coluna/linha conforme o código C++.

---

# 🧭 1. O Papel do Lexer no Pipeline do Compilador

O lexer é a ponte entre o texto bruto e a estrutura formal da linguagem:

```text
Código Fonte (string) → [ Lexer ] → Lista de Tokens → Parser (AST)
```

O lexer:

1. Lê o código caractere por caractere
2. Remove espaços, quebras e comentários
3. Identifica padrões formais da linguagem
4. Gera tokens com tipo, lexema e posição
5. Detecta e reporta erros léxicos de forma precisa
6. Entrega tokens sob demanda com **lookahead de 1 token** (`peek()` / `next()`)

Essa abordagem simplifica drasticamente o parser e garante performance e robustez.

---

# 🧪 2. Definição Formal da Linguagem Léxica (Implementação Atual)

A lexicografia da CF é formalmente definida por um conjunto de regras regulares.  
Abaixo está a visão **formal** alinhada com o que o código C++ efetivamente implementa.

## ✔ 2.1 Identificadores

Formalmente na especificação textual, temos:

```text
identifier ::= letter+
letter     ::= 'A'..'Z' | 'a'..'z'
```

Na **implementação real** do `Lexer`, isso é expandido para:

- O primeiro caractere deve ser:
  - uma letra ASCII (`isalpha(c) == true`), **ou**
  - qualquer byte com valor `>= 0x80` (tipicamente parte de um caractere acentuado em UTF‑8).
- Os próximos caracteres seguem a mesma regra: `isalpha(c) || c >= 0x80`.

Ou seja, de forma mais fiel ao código:

```text
identifier ::= id_char+
id_char    ::= ascii_letter | extended_byte
ascii_letter  ::= 'A'..'Z' | 'a'..'z'
extended_byte ::= qualquer byte com valor >= 0x80
```

⚙️ **Case-insensitive (apenas para ASCII)**  
Antes de comparar com as palavras-chave, o lexema é normalizado:

- apenas os bytes ASCII são convertidos para minúsculas (`lower()`),
- bytes `>= 0x80` são preservados como estão.

Com isso, as variáveis:

- `SOMA`, `soma`, `SoMa` → são considerados o mesmo identificador;
- `área`, `ÁREA` → continuam distintos na forma bruta de bytes, mas o código trata tudo como parte de identificador válido.

📌 **Resumo de regra prática para o usuário:**

> _Todo nome de variável deve conter apenas letras (com ou sem acento). Não há dígitos, underscores ou outros símbolos em identificadores. A linguagem não distingue maiúsculas de minúsculas na parte ASCII._

---

## ✔ 2.2 Palavras-chave

Abaixo estão todas as keywords reconhecidas pela linguagem CF, conforme a especificação original:

| TokenKind   | Lexema “oficial” |
| ----------- | ---------------- |
| KwInteiro   | Inteiro          |
| KwLogico    | Logico           |
| KwCaractere | Caractere        |
| KwEnquanto  | Enquanto         |
| KwSe        | Se               |
| KwSenao     | Senao            |
| KwPara      | Para             |
| KwImprimir  | Imprimir         |
| KwVerdade   | Verdade          |
| KwMentira   | Mentira          |
| KwEm        | em               |

Na **implementação atual**, o lexer aceita também **formas acentuadas** de algumas palavras:

- `Logico` **ou** `Lógico` → `KwLogico`
- `Senao` **ou** `Senão` → `KwSenao`

Isso é feito via comparação com o lexema já case‑folded em ASCII (`lower()`), mas preservando bytes acima de `0x80`.

---

## ✔ 2.3 Literais Numéricos

```text
integer ::= digit+
digit   ::= '0'..'9'
```

- Não há sinal embutido no literal:
  - `-5` é analisado como `Minus` + `Integer("5")`;
- Não há floats;
- Não há prefixos hex/bin/oct (apenas inteiros decimais).

---

## ✔ 2.4 Literais de String

Implementação em `Lexer::lex_string()`:

```text
string ::= '"' string_content* '"'
```

Onde `string_content` é qualquer sequência de caracteres (incluindo `\n`), com suporte a escapes:

- se encontrar `"` → finaliza a string;
- se encontrar `\` → consome `\` + próximo caractere literalmente (sem interpretação semântica de escape pelo lexer).

Pontos importantes:

- A string **pode conter quebras de linha** (`'\n'`), pois a função não proíbe `\n`, apenas atualiza corretamente `line`/`column` via `get()`.
- Se o fim do arquivo (`'\0'`) é atingido antes de encontrar o `"` de fechamento, é lançado erro léxico:

  > `"string não terminada"`

- Se encontramos `\` como último caractere antes de EOF, lançamos:

  > `"escape inválido no final de string"`

🔎 **Consequência prática:**

- Strings multi‑linha são aceitas pela implementação atual do CF.
- Escapes são mantidos literalmente no lexema, a interpretação (por exemplo `"\n"` virar caractere nova linha) é feita em fase posterior (semântica / codegen).

---

## ✔ 2.5 Literais de Caractere

Implementação em `Lexer::lex_char()`:

```text
char_literal ::= '\'' char_content '\''
```

Regras:

1. Lê `'` de abertura.
2. Se próximo caractere for `\0` → erro: _"caractere não terminado"_.
3. Se próximo caractere for `\`:
   - consome `\` + próximo caractere, desde que não seja `\0` ou `\n`;
   - caso contrário → erro: _"escape inválido em caractere"_.
4. Se próximo caractere não for `\`:
   - consome exatamente um caractere (qualquer byte).
5. Exige imediatamente em seguida o `'` de fechamento:
   - se não houver → erro: _"caractere deve conter exatamente 1 símbolo"_.

📌 **Resumo**:

- O conteúdo de um literal de caractere deve representar **exatamente 1 símbolo**, seja simples (`'a'`) ou escapado (`'\n'`, `'\''`, etc.).
- O lexer não impõe restrição a “apenas ASCII”: qualquer byte é aceito.

---

## ✔ 2.6 Comentários

A linguagem CF aceita oficialmente:

### 2.6.1 Comentário de linha (`$`)

```text
$ comentário até a quebra de linha
```

Implementação:

- Se o caractere atual é `$` e o próximo **não** é `$`:
  - consome tudo até `'\n'` ou EOF em `skip_spaces_and_comments()`.

---

### 2.6.2 Comentário de bloco CF (`$$ ... $$`)

```text
$$
  texto qualquer
$$
```

Implementação:

- Detectado por `skip_spaces_and_comments()` quando encontra `$$`.
- Consome até encontrar outro par `$$`.
- Se chegar em EOF sem fechar → gera `CompileError` com mensagem:

  > `"comentário de bloco '$$' não fechado"`

---

## ✔ 2.7 Operadores e símbolos

A estratégia usada é de **maximal munch**: o lexer sempre tenta ler primeiro os operadores de 2 caracteres, depois os de 1.

Tabela de operadores de **2 caracteres** reconhecidos:

| Lexema | TokenKind |
| ------ | --------- |
| `<-`   | Assign    |
| `**`   | Pow       |
| `<>`   | Ne        |
| `>=`   | Ge        |
| `<=`   | Le        |

Depois, os operadores e símbolos de **1 caractere**:

| Categoria     | Lexemas reconhecidos    | Detalhes                                      |
| ------------- | ----------------------- | --------------------------------------------- |
| Aritméticos   | `+`, `-`, `*`, `/`, `%` | Soma, subtração, multiplicação, divisão, mod  |
| Relacionais   | `=`, `<`, `>`           | Igual, menor, maior                           |
| Lógicos       | `&`, `^`, `\|`          | AND lógico, OR lógico (tanto `^` quanto `\|`) |
| Delimitadores | `{`, `}`, `(`, `)`      | Blocos e agrupamento                          |
| Separadores   | `,`, `;`                | Lista de variáveis, fim de comando            |

Na implementação atual:

- `&` → `TokenKind::And`
- `^` → `TokenKind::Or`
- `|` → `TokenKind::Or`

📌 **Nota sobre a especificação original:**

> O enunciado da disciplina define apenas `&` e `^` como operadores lógicos AND / OR.  
> A presença de `|` como OR lógico é um **superset** da especificação, oferecido pelo compilador CF como conveniência.

---

# 📦 3. Estruturas Centrais do Módulo Léxico

## ✔ 3.1 `Token`

```cpp
struct Token {
    TokenKind kind;
    std::string lexeme;
    Position pos;
};
```

- `kind` → tipo semântico do token (palavra-chave, identificador, inteiro, operador, etc.).
- `lexeme` → texto literal lido do fonte (por exemplo `"Inteiro"`, `"soma"`, `"**"`).
- `pos` → posição (linha, coluna) **do início** do token.

---

## ✔ 3.2 `Position`

Representa a posição **humana** no arquivo-fonte:

- `line` (1-based)
- `column` (1-based)

Atualizada pela função `get()`:

- Ao ler `'\n'`:
  - `line++`
  - `column = 1`
- Caso contrário:
  - `column++`

Isso permite produzir diagnósticos precisos, exibindo:

- linha com o erro (`current_line_text()`),
- seta de indicação (`^`) no lugar correto.

---

# 🔍 4. Arquitetura Interna da Classe `Lexer`

A classe `Lexer` possui, em alto nível:

- `std::string src_` → texto-fonte completo;
- `std::size_t i_` → índice atual dentro de `src_`;
- `Position pos_` → (linha, coluna) do caractere atual;
- `Token current_` → token de lookahead já calculado.

### 4.1 Ciclo de vida básico

1. **Construção**:

   ```cpp
   Lexer::Lexer(std::string source) : src_(std::move(source)) {
       current_ = lex_token(); // já pré-carrega o primeiro token
   }
   ```

2. **Leitura de tokens**:

   - `peek()` → retorna `const Token&` para o token atual, sem consumir;
   - `next()` → devolve o token atual e já gera o próximo (via `lex_token()`).

3. **Internamente**, `lex_token()` é responsável por:

   - pular espaços e comentários (`skip_spaces_and_comments()`),
   - ler o próximo lexema conforme as regras (identificador, número, string, operador, etc.),
   - criar o `Token` com a posição correta via `make()`.

---

## ✔ 4.2 Funções utilitárias de caractere

### `peekc(std::size_t off = 0)`

Olha o caractere na posição `i_ + off` sem consumir:

- se passar do fim de `src_`, retorna `'\0'`.

Isso permite lookahead arbitrário no nível de **caractere bruto**.

---

### `get()`

Consome 1 caractere:

- se `i_` está no fim, retorna `'\0'`;
- avança `i_` em 1;
- atualiza `pos_.line` e `pos_.column`.

Todos os consumidores de caracteres **devem** usar `get()` para manter posição consistente.

---

### `unread()`

“Deslê” um caractere (volta 1 posição em `i_` e na coluna):

- usado com parcimônia;
- não volta linha (não deve ser usado para desfazer um `'\n'`).

---

## ✔ 4.3 Funções de alto nível

### `skip_spaces_and_comments()`

Responsável por pular:

- espaços em branco (`isspace`),
- comentários de linha: `$ ... \n`,
- comentários de bloco CF: `$$ ... $$`.

Repete esse processo até o primeiro caractere **não ignorável**.

---

### `lex_identifier_or_keyword()`

Fluxo:

1. Valida primeiro caractere: se não for letra ASCII nem byte `>= 0x80`, lança erro de símbolo inválido.
2. Lê todos os caracteres “de letra” (ASCII ou bytes >= 128).
3. Gera `low = lower(lx)` (somente ASCII minúsculo).
4. Compara `low` com o conjunto de palavras-chave (`inteiro`, `logico`, `lógico`, `senao`, `senão`, etc.).
5. Se encontrar correspondência → retorna token de keyword.
6. Caso contrário → `TokenKind::Identifier`.

---

### `lex_number()`

Lê sequência de dígitos `[0-9]+` e retorna `TokenKind::Integer`.  
Não verifica overflow no léxico (isso fica a cargo das fases posteriores, se desejado).

---

### `lex_string()`

Implementação já detalhada na seção 2.4:

- consome `"` de abertura;
- lê até encontrar `"` de fechamento, permitindo `\` + caractere;
- lança erros em caso de EOF sem fechamento ou escape truncado;
- retorna `TokenKind::String`.

---

### `lex_char()`

Implementação já detalhada na seção 2.5:

- exige exatamente 1 símbolo (com ou sem escape) entre aspas simples;
- valida fechamento correto;
- em caso de violação, lança erro de caractere inválido.

---

### `lex_token()`

Responsável por produzir **o próximo token de alto nível**:

1. Chama `skip_spaces_and_comments()`.
2. Se `peekc()` é `'\0'`, retorna `TokenKind::End`.
3. Verifica operadores de 2 caracteres usando `starts_with()`:
   - `<-`, `**`, `<>`, `>=`, `<=`.
4. Caso contrário, faz um `switch` em `peekc()` para operadores/símbolos de 1 caractere.
5. Se o caractere for:
   - `"`, chama `lex_string()`.
   - `'`, chama `lex_char()`.
   - letra ou byte >= 128, chama `lex_identifier_or_keyword()`.
   - dígito, chama `lex_number()`.
6. Se nada se encaixar, consome o caractere e gera erro léxico:

   > `"símbolo inválido '<c>'"`

---

### `next()` e suporte a comentários `/* ... */`

A função `next()` faz:

1. Antes de devolver o token atual, faz uma verificação especial:

   ```cpp
   if (peekc() == '/' && peekc(1) == '*') {
       // consome o bloco /* ... */ inteiro
       // e chama next() recursivamente
   }
   ```

2. Se não for comentário `/* ... */`, devolve `current_` e gera `current_ = lex_token()`.

Com isso, o `Lexer`:

- já suporta o fluxo tradicional `while (tok.kind != End) { ... tok = lexer.next(); }`;
- não “vaza” tokens `/` e `*` quando estão formando um comentário de bloco estilo C.

---

# ✨ 5. Detecção e Relato de Erros Léxicos

A infraestrutura de erros léxicos utiliza:

- `Diagnostic`
- `Position`
- `CompileError`

Cada erro monta um `Diagnostic` com:

- `phase = "lexical"`;
- `pos` (linha, coluna);
- `message` descritiva;
- `line_text` com o conteúdo da linha atual (`current_line_text()`).

Exemplos de mensagens:

- Comentário de bloco `$$` não fechado:

  ```text
  [lexical] linha 4, coluna 1: comentário de bloco '$$' não fechado
  4 | $$ teste
            ^
  ```

- Símbolo inválido:

  ```text
  [lexical] linha 3, coluna 10: símbolo inválido '@'
  3 |   x <- 10 @ 2;
                   ^
  ```

- Literal de caractere malformado:

  ```text
  [lexical] linha 7, coluna 12: caractere deve conter exatamente 1 símbolo
  7 |   c <- 'ab';
                 ^
  ```

Essas mensagens são fundamentais para a experiência do usuário com o compilador CF, permitindo correção rápida.

---

# 📘 6. Exemplo Real (Anotado)

Código CF:

```cf
Inteiro soma <- 0;
Para i em (1, 5, 1) {
    soma <- soma + i;
}
Imprimir("Resultado = " + soma + "\n");
```

Sequência de tokens produzidos (conceitual):

```text
KwInteiro     "Inteiro"      pos=1:1
Identifier    "soma"         pos=1:9
Assign        "<-"           pos=1:14
Integer       "0"            pos=1:17
Semicolon     ";"            pos=1:18

KwPara        "Para"         pos=2:1
Identifier    "i"            pos=2:6
KwEm          "em"           pos=2:8
LParen        "("            pos=2:11
Integer       "1"            pos=2:12
Comma         ","            pos=2:13
Integer       "5"            pos=2:15
Comma         ","            pos=2:16
Integer       "1"            pos=2:18
RParen        ")"            pos=2:19
LBrace        "{"            pos=2:21

Identifier    "soma"         pos=3:5
Assign        "<-"           pos=3:10
Identifier    "soma"         pos=3:13
Plus          "+"            pos=3:18
Identifier    "i"            pos=3:20
Semicolon     ";"            pos=3:21

RBrace        "}"            pos=4:1

KwImprimir    "Imprimir"     pos=5:1
LParen        "("            pos=5:9
String        "Resultado = " pos=5:10
Plus          "+"            pos=5:25
Identifier    "soma"         pos=5:27
Plus          "+"            pos=5:32
String        "\n"           pos=5:36
RParen        ")"            pos=5:40
Semicolon     ";"            pos=5:41

End           ""             pos=6:1
```

---

# 🧠 7. Decisões de Design e Racional Técnico

1. **Case-insensitive apenas para ASCII**  
   Simplifica a implementação e respeita a ideia do enunciado (“não faz distinção entre caixa alta e baixa”), sem acoplar o compilador a detalhes completos de Unicode.

2. **Suporte a bytes `>= 0x80` em identificadores**  
   Permite escrever nomes com acento (ex.: `área`, `índice`) sem que o lexer quebre, aproximando a linguagem de um uso mais natural em português.

3. **Maximal munch para operadores**  
   Evita ambiguidades como:

   - ler `<` e `-` separadamente em vez de `<-`;
   - ler `*` e `*` separadamente em vez de `**`.

4. **Comentários `$$ ... $$` e `/* ... */`**  
   Mantém compatibilidade com o enunciado (via `$$ ... $$`) e, ao mesmo tempo, oferece uma sintaxe mais familiar para quem vem do C/C++ (`/* ... */`), sem impactar a gramática sintática.

5. **Strings com suporte a múltiplas linhas**  
   A implementação permite strings multi‑linha, o que é útil em alguns cenários (mensagens longas, banners ASCII, etc.).  
   A interpretação final desses textos é responsabilidade de fases posteriores.

6. **Erros ricos em contexto**  
   Ao carregar:

   - a linha completa (`current_line_text()`),
   - a posição exata (`line`, `column`),
   - e mensagens claras e em português,

   o módulo léxico cumpre um papel pedagógico importante, tanto para o usuário quanto para a disciplina de compiladores.

---

# 🎯 8. Conclusão

O módulo **Léxico (`cf/lexer`)** do compilador CF foi projetado com:

- **rigor técnico** (baseado em expressões regulares e autômatos);
- **implementação moderna em C++**, com foco em clareza e robustez;
- **mensagens de erro pedagógicas**, em português, pensadas para uso em ambiente acadêmico;
- **extensibilidade**, permitindo ajustes simples nas regras lexicais;
- **aderência à especificação CF**, com algumas extensões práticas (acentos, `|`, `/* ... */`).

Ele transforma o texto-fonte em uma forma estruturada e confiável, servindo como base sólida para:

- o **analisador sintático** (parser, construção de AST),
- o **analisador semântico** (tipos, escopos, declarações),
- e o **gerador de código** (Assembly MIPS/RISC‑V, conforme o restante do projeto).
