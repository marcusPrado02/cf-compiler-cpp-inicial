# 🧩 Módulo Parser (`cf/parser`)

O **Parser** é a segunda fase do compilador CF (Compila Fofo).  
Ele recebe a sequência de **tokens** produzidos pelo Lexer e constrói uma **Árvore Sintática Abstrata (AST)** bem formada, validando a estrutura do programa conforme a gramática da linguagem CF.

Esta documentação está **atualizada de acordo com a implementação real em C++**, incluindo:

- gramática com **lista de declarações múltiplas** (`Inteiro a, b <- 2, c;`);
- comando `Para` com **passo opcional** (`(ini, fim [, passo])`);
- nível único de **expressões lógicas** (AND/OR com mesma precedência);
- suporte explícito a **literais booleanos** (`Verdade` / `Mentira`);
- suporte a **unário negativo** (`-expr`) implementado como `0 - expr`;
- integração direta com o sistema de diagnósticos (`Diagnostic` / `CompileError`).

---

# 🧭 1. O Parser no Pipeline do Compilador CF

Visão geral do pipeline:

```text
Fonte  →  Lexer        →  Tokens
Tokens →  Parser       →  AST (árvore sintática)
AST    →  Semântico    →  AST anotada (tipos, escopos)
AST    →  Otimizador   →  AST simplificada
AST    →  Codegen      →  Assembly (MIPS/RISC-V)
```

Responsabilidades principais do Parser:

1. **Validar sintaxe**: garantir que o programa segue as regras da linguagem CF.
2. **Organizar a estrutura**: transformar uma lista linear de tokens em uma estrutura hierárquica (AST).
3. **Preservar informações de posição**: cada nó AST carrega `pos` (linha/coluna), útil para erros posteriores.
4. **Modelar precedência e associatividade**: especialmente para expressões aritméticas e lógicas.

O Parser da CF é implementado como um **parser recursivo descendente LL(1)**, ideal para fins didáticos e fácil de depurar.

---

# 🧱 2. Arquitetura da Classe `Parser`

A assinatura conceitual da classe, alinhada ao código atual, é a seguinte:

```cpp
class Parser {
public:
    explicit Parser(Lexer lex);

    Program parse_program();

private:
    // Acesso aos tokens
    const Token& peek();           // lookahead sem consumir
    Token next();                  // consome token atual e avança
    bool at(TokenKind k) const;    // verifica se token atual é do tipo k

    // Utilidades de consumo
    bool eat(TokenKind k, const char* expectMsg = nullptr);
    void expect(TokenKind k, const char* msg);

    // Mapeamento de tipos
    CfType map_type(TokenKind k);

    // Erros
    [[noreturn]] void syntax_error(const Token& got, const std::string& msg);

    // Alto nível
    Program parse_program_impl();
    std::vector<StmtPtr> parse_decl_or_stmt();
    std::vector<StmtPtr> parse_declaration();
    StmtPtr parse_statement();
    std::vector<StmtPtr> parse_block();
    StmtPtr parse_assign_tail_after_ident(Token identTok);

    // Estruturas de controle
    StmtPtr parse_if();
    StmtPtr parse_while();
    StmtPtr parse_for();
    StmtPtr parse_print();

    // Expressões (precedência)
    ExprPtr parse_expr();
    ExprPtr parse_logical();
    ExprPtr parse_rel();
    ExprPtr parse_add();
    ExprPtr parse_mul();
    ExprPtr parse_pow();
    ExprPtr parse_primary();
};
```

> **Nota:** Alguns nomes auxiliares podem variar levemente no código real (por exemplo, ausência de `parse_program_impl`), mas a estrutura conceitual é essa.

---

# 📜 3. Gramática CF Alinhada ao Parser Atual (EBNF)

Abaixo está a gramática em EBNF simplificada, adaptada exatamente ao que o parser implementa hoje.

## 3.1 Programa, Declarações e Comandos

```ebnf
program        ::= { decl_or_stmt } EOF ;

decl_or_stmt   ::= declaration
                 | statement ;

declaration    ::= type ident_decl_list ';' ;

type           ::= KwInteiro
                 | KwLogico
                 | KwCaractere ;

ident_decl_list ::= ident_decl { ',' ident_decl } ;

ident_decl     ::= Identifier ['<-' expr] ;

statement      ::= assign_stmt
                 | print_stmt
                 | if_stmt
                 | while_stmt
                 | for_stmt ;

assign_stmt    ::= Identifier '<-' expr ';' ;

print_stmt     ::= KwImprimir '(' [ expr { ',' expr } ] ')' ';' ;

if_stmt        ::= KwSe expr block [ KwSenao block ] ;

while_stmt     ::= KwEnquanto expr block ;

for_stmt       ::= KwPara Identifier KwEm '(' expr ',' expr [ ',' expr ] ')' block ;

block          ::= '{' { decl_or_stmt } '}' ;
```

Pontos importantes refletindo o código real:

- **Declarações múltiplas**: `ident_decl_list` permite declarar várias variáveis do mesmo tipo em uma única linha, com inicialização opcional para cada uma.
- **Passo opcional em `Para`**: a terceira expressão dentro do parênteses é opcional; se ausente, o valor padrão (tipicamente assumido na análise semântica/codegen) é `1` ou `-1` a depender da lógica adotada.
- **Blocos** (`block`) aparecem como corpos de `Se`, `Enquanto` e `Para` — não existem blocos “soltos” como comandos top-level; eles sempre vêm diretamente após essas palavras-chave de controle.

---

## 3.2 Expressões e Precedência

A hierarquia implementada no parser é:

1. **Lógico** (AND/OR com mesma precedência)
2. **Relacional** (`=`, `<>`, `<`, `<=`, `>`, `>=`)
3. **Aditivo** (`+`, `-`)
4. **Multiplicativo** (`*`, `/`, `%`)
5. **Potência** (`**`, associativo à direita)
6. **Primário** (literais, identificadores, parênteses, booleanos, unário `-`)

A gramática:

```ebnf
expr          ::= logical ;

logical       ::= rel { ( '&' | '|' | '^' ) rel } ;

rel           ::= add [ rel_op add ] ;

rel_op        ::= '=' | '<>' | '<' | '<=' | '>' | '>=' ;

add           ::= mul { ( '+' | '-' ) mul } ;

mul           ::= pow { ( '*' | '/' | '%' ) pow } ;

pow           ::= primary [ '**' pow ] ;

primary       ::= Integer
                | String
                | Char
                | Identifier
                | KwVerdade        // literal booleano true
                | KwMentira        // literal booleano false
                | '(' expr ')'
                | '-' primary ;    // unário negativo (atual)
```

### Observações importantes

1. **Nível único para operadores lógicos**  
   Diferentemente de uma gramática com `logical_and` e `logical_or` separados, o parser atual trata `AND` e `OR` (tokens `TokenKind::And` e `TokenKind::Or`, mapeados a `&`, `^` e `|`) no mesmo nível de precedência, associados à esquerda.

   Exemplos:

   ```cf
   a & b | c   -- parseado como ( (a & b) | c )
   a | b & c   -- parseado como ( (a | b) & c )
   ```

   Não há diferenciação de precedência entre AND e OR; isso é uma decisão de design da linguagem nesta implementação.

2. **Operadores relacionais**  
   `parse_rel()` aceita **no máximo um** operador relacional por expressão, garantindo que construções como `a < b < c` não sejam aceitas diretamente.

   Ex.:

   ```cf
   a < b       -- ok
   a = b       -- ok
   a <> b      -- ok
   a < b < c   -- erro sintático
   ```

3. **Expoente (`**`) associativo à direita\*\*

   A função `parse_pow()` implementa:

   ```cpp
   base = parse_primary();
   if (at(TokenKind::Pow)) {
       op = next();
       expo = parse_pow();
       // constrói base ** expo
   }
   ```

   Assim:

   ```cf
   a ** b ** c   -- parseado como a ** (b ** c)
   ```

4. **Unário `-` implementado em `parse_primary()`**

   O código especial para `Minus` em `parse_primary()` trata expressões como `-x` ou `-42`:

   ```cpp
   case TokenKind::Minus: {
       Token minusTok = next();     // consome '-'
       auto zero = std::make_unique<ExprInteger>();
       zero->digits = "0";
       auto rhs = parse_primary();  // expressão logo após o '-'
       auto e = std::make_unique<ExprBinary>();
       e->op = BinOp::Sub;
       e->lhs = std::move(zero);
       e->rhs = std::move(rhs);
       return e;
   }
   ```

   Em outras palavras, o unário negativo é **rebaixado** para uma operação binária `0 - expr`, o que simplifica o código de geração de código e evita criar um novo tipo de nó unário no AST.

   Exemplos:

   ```cf
   -x        -- vira (0 - x)
   -42       -- vira (0 - 42)
   -(a + b)  -- vira (0 - (a + b))
   ```

---

# 🧩 4. Funções Utilitárias de Parsing

## 4.1 `eat(TokenKind k, const char* expectMsg)`

- Tenta consumir um token de tipo `k`.
- Se o token atual é `k`:
  - consome via `next()`
  - retorna `true`.
- Se não é `k`:
  - se `expectMsg != nullptr`, lança `syntax_error(...)` com a mensagem dada;
  - caso contrário, retorna `false`.

Uso típico:

```cpp
if (!eat(TokenKind::Comma, nullptr)) {
    // não tinha vírgula, segue o fluxo
}
```

ou

```cpp
eat(TokenKind::Comma, "esperado ',' entre argumentos");
```

---

## 4.2 `expect(TokenKind k, const char* msg)`

- Exige que o token atual seja do tipo `k`.
- Se for, consome.
- Se não for, chama `syntax_error(...)` com a mensagem `msg`.

Uso:

```cpp
expect(TokenKind::Semicolon, "esperado ';' após declaração");
```

---

## 4.3 `map_type(TokenKind k)`

Mapeia keywords de tipo para o enum `CfType`:

- `KwInteiro` → `CfType::Inteiro`
- `KwLogico` → `CfType::Logico`
- `KwCaractere` → `CfType::Caractere`
- Outro token → `CfType::Desconhecido`

Essa função é usada em `parse_declaration()` para construir nós `StmtDecl` com o tipo correto.

---

## 4.4 `syntax_error(const Token& got, const std::string& msg)`

Monta um `Diagnostic` com:

- `phase = "syntax"`
- `pos = got.pos`
- `message = msg + " (encontrado: <tipo> \"<lexema>\")"`

e lança `CompileError`.

Exemplo de mensagem gerada:

```text
[syntax] linha 3, coluna 10: esperado ';' após atribuição (encontrado: Identifier "x")
```

---

# 🧱 5. Parsing de Programa e Blocos

## 5.1 `parse_program()`

Fluxo:

```cpp
Program Parser::parse_program() {
    Program p;
    while (!at(TokenKind::End)) {
        auto stmts = parse_decl_or_stmt();
        for (auto& s : stmts) {
            p.items.push_back(std::move(s));
        }
    }
    return p;
}
```

Ou seja:

- Lê **até EOF** (`TokenKind::End`).
- Cada chamada a `parse_decl_or_stmt()` pode retornar **1 ou N** statements (no caso de múltiplas declarações na mesma linha).

---

## 5.2 `parse_decl_or_stmt()`

Decisão de alto nível:

- Se o token atual é `KwInteiro`, `KwLogico` ou `KwCaractere` → **declaração(ões)**.
- Caso contrário → exatamente **um comando** (`statement`).

---

## 5.3 `parse_declaration()`

Gramática efetiva:

```ebnf
declaration ::= type ident_decl_list ';' ;
ident_decl_list ::= ident_decl { ',' ident_decl } ;
ident_decl ::= Identifier ['<-' expr] ;
```

Exemplos aceitos:

```cf
Inteiro x;
Logico ativo <- Verdade;
Caractere c <- 'a';
Inteiro a, b <- 2, c;
```

Cada variável é traduzida em um nó `StmtDecl` separado dentro do vetor retornado.

---

## 5.4 `parse_statement()`

Reconhece, na ordem:

1. `KwEnquanto` → `parse_while()`
2. `KwSe` → `parse_if()`
3. `KwPara` → `parse_for()`
4. `KwImprimir` → `parse_print()`
5. `Identifier` → `parse_assign_tail_after_ident(id)`

Caso nenhum destes se aplique, gera erro:

```text
comando inválido
```

> **Importante:** um bloco (`{ ... }`) **não** é reconhecido como comando isolado em nível top-level; ele sempre aparece como corpo de `Se`, `Enquanto` ou `Para`.

---

## 5.5 `parse_block()`

Bloco sintático:

```ebnf
block ::= '{' { decl_or_stmt } '}' ;
```

Fluxo:

1. Exige `'{'`.
2. Enquanto não encontrar `'}'`:
   - chama `parse_decl_or_stmt()` para permitir **declarações misturadas com comandos** dentro do bloco.
3. Exige `'}'`.

Se EOF chega antes de `'}'`, o parser lança erro de sintaxe:

```text
EOF dentro de bloco — '}' esperado
```

---

# 🔁 6. Estruturas de Controle

## 6.1 `parse_if()` — Comando `Se` / `Senao`

Gramática:

```ebnf
if_stmt ::= KwSe expr block [ KwSenao block ] ;
```

Construção de AST:

```cpp
auto s = std::make_unique<StmtIf>();
s->cond      = cond;
s->then_body = then_body;
s->else_body = else_body; // pode ser vazio
```

Exemplo CF:

```cf
Se idade >= 18 {
    Imprimir("Maior\n");
}
Senao {
    Imprimir("Menor\n");
}
```

---

## 6.2 `parse_while()` — Comando `Enquanto`

Gramática:

```ebnf
while_stmt ::= KwEnquanto expr block ;
```

Exemplo CF:

```cf
Enquanto s < 100 {
    s <- s + i;
    i <- i + 1;
}
```

---

## 6.3 `parse_for()` — Comando `Para` com Passo Opcional

Gramática efetiva (implementação):

```ebnf
for_stmt ::= KwPara Identifier KwEm '(' expr ',' expr [ ',' expr ] ')' block ;
```

Onde:

- `Identifier` → variável de controle do laço.
- Primeiro `expr` → valor inicial.
- Segundo `expr` → valor final.
- Terceiro `expr` (opcional) → passo (incremento/decremento).

Exemplos CF válidos:

```cf
Para i em (0, 10, 1) {
    Imprimir(i + "\n");
}

Para i em (10, 0, -2) {
    Imprimir(i + "\n");
}

Para k em (1, 5) {      // passo padrão tratado depois (ex.: 1)
    Imprimir(k + "\n");
}
```

O parser:

1. Lê `Para` e o identificador.
2. Exige `KwEm`.
3. Exige `'('`.
4. Lê `begin = expr`.
5. Exige `','`.
6. Lê `end = expr`.
7. Se houver `','`:
   - lê `step = expr` e guarda em `s->step` (como `std::optional<ExprPtr>`).
8. Exige `')'`.
9. Lê `body = block`.
10. Monta `StmtFor` com esses campos.

---

## 6.4 `parse_print()` — Comando `Imprimir`

Gramática:

```ebnf
print_stmt ::= KwImprimir '(' [ expr { ',' expr } ] ')' ';' ;
```

Permite:

- zero argumentos: `Imprimir();`
- um argumento: `Imprimir("Oi\n");`
- múltiplos argumentos (concatenados na geração de código):

```cf
Imprimir("Area = ", a, "\n");
```

No AST, `StmtPrint` contém um vetor `args` com as expressões na ordem.

---

# 🧮 7. Parsing de Expressões em Detalhe

## 7.1 `parse_expr()`

Função de entrada para expressões:

```cpp
ExprPtr Parser::parse_expr() {
    return parse_logical();
}
```

Tudo passa pelo nível lógico.

---

## 7.2 `parse_logical()` — nível lógico único (AND/OR)

```cpp
auto lhs = parse_rel();
while (at(TokenKind::And) || at(TokenKind::Or)) {
    Token op = next();
    auto rhs = parse_rel();
    auto e   = std::make_unique<ExprBinary>();
    e->op    = (op.kind == TokenKind::And) ? BinOp::And : BinOp::Or;
    e->lhs   = std::move(lhs);
    e->rhs   = std::move(rhs);
    lhs      = std::move(e);
}
return lhs;
```

Tokens lógicos vêm do Lexer:

- `&` → `TokenKind::And`
- `^` → `TokenKind::Or`
- `|` → `TokenKind::Or`

Associatividade à esquerda:

```cf
a & b & c   -- ( (a & b) & c )
a | b | c   -- ( (a | b) | c )
a & b | c   -- ( (a & b) | c )
```

---

## 7.3 `parse_rel()` — Operadores relacionais

Aceita no máximo um operador relacional:

```cpp
auto lhs = parse_add();
if (at(TokenKind::Eq) || at(TokenKind::Ne) ||
    at(TokenKind::Gt) || at(TokenKind::Lt) ||
    at(TokenKind::Ge) || at(TokenKind::Le)) {
    Token op = next();
    auto rhs = parse_add();
    // constrói ExprBinary(BinOp::Eq/Ne/Gt/Lt/Ge/Le)
    ...
    return e;
}
return lhs;
```

Sintaxe CF típica:

```cf
a = b
a <> b
x >= 10
idade < 18
```

---

## 7.4 `parse_add()` — `+` e `-`

```cpp
auto lhs = parse_mul();
while (at(TokenKind::Plus) || at(TokenKind::Minus)) {
    Token op = next();
    auto rhs = parse_mul();
    // ExprBinary(BinOp::Add ou BinOp::Sub)
    ...
}
```

Associatividade à esquerda:

```cf
a - b - c   -- ( (a - b) - c )
a + b + c   -- ( (a + b) + c )
```

---

## 7.5 `parse_mul()` — `*`, `/`, `%`

Similar a `parse_add()`, com operadores:

- `*` → `BinOp::Mul`
- `/` → `BinOp::Div`
- `%` → `BinOp::Mod`

```cf
a * b / c   -- ( (a * b) / c )
```

---

## 7.6 `parse_pow()` — `**` (potência right-associative)

Regra:

```ebnf
pow ::= primary [ '**' pow ] ;
```

Isto faz a associatividade ser à **direita**:

```cf
2 ** 3 ** 2   -- 2 ** (3 ** 2)
```

---

## 7.7 `parse_primary()` — literais, identificadores, booleanos, parênteses e unário `-`

Casos suportados:

- `Integer` → `ExprInteger`
- `Char` → `ExprChar`
- `String` → `ExprString`
- `Identifier` → `ExprIdent`
- `KwVerdade` → `ExprBool(value = true)`
- `KwMentira` → `ExprBool(value = false)`
- `'(' expr ')'` → `ExprGroup`
- `'-' primary` → convertido em `ExprBinary(0 - primary)`

Caso nenhum desses padrões se aplique, o parser chama:

```cpp
syntax_error(t, "expressão primária esperada");
```

Exemplos CF válidos:

```cf
42
'a'
"teste"
nome
Verdade
Mentira
(x + 1)
-10
-(a + b)
```

---

# 🧠 8. Tratamento Profissional de Erros de Sintaxe

O módulo `parser` integra-se ao sistema de diagnósticos comum do compilador.

Em qualquer inconsistência, o parser lança `CompileError` com um `Diagnostic` contendo:

- `phase = "syntax"`
- `pos` (linha/coluna)
- `message` explicando o erro e incluindo o token encontrado

Exemplos de situações detectadas:

1. **Falta de ponto e vírgula**

```cf
x <- 10
y <- 20;
```

Mensagem típica:

```text
[syntax] linha 2, coluna 1: esperado ';' após atribuição (encontrado: Identifier "y")
```

2. **Bloco não fechado**

```cf
Se x > 0 {
    Imprimir("ok");
```

Erro:

```text
[syntax] linha N, coluna C: EOF dentro de bloco — '}' esperado (encontrado: End "")
```

3. **Expressão primária ausente**

```cf
Inteiro x <- ;
```

Erro:

```text
[syntax] linha N, coluna C: expressão primária esperada (encontrado: Semicolon ";")
```

Esse nível de detalhe torna o compilador CF muito agradável de usar e ideal para fins acadêmicos.

---

# 🎯 9. Conclusão e Próximos Passos

O módulo **Parser (`cf/parser`)**:

- implementa um **parser recursivo descendente LL(1)**, claro e didático;
- está completamente alinhado à **gramática CF** atualmente implementada;
- suporta:
  - múltiplas declarações na mesma linha;
  - laços `Para` com passo opcional;
  - expressões lógicas, relacionais, aritméticas e de potência;
  - unário negativo, booleanos (`Verdade` / `Mentira`) e strings;
- integra-se com o sistema de erros para diagnósticos precisos;
- entrega uma **AST limpa e bem estruturada** para as fases semântica, de otimização e de geração de código.

Essa documentação reflete fielmente o código C++ atual do parser e pode ser usada como referência oficial do módulo tanto em contexto acadêmico quanto em um projeto open-source de ensino de compiladores.
