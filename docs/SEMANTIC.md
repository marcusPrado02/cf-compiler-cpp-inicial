# 🧠 Analisador Semântico (`cf/semantic`)

O **Analisador Semântico** é a fase do compilador CF (Compila Fofo) responsável por garantir que o programa **faz sentido**, e não apenas que está bem formado sintaticamente.

Enquanto:

- o **Lexer** transforma texto em tokens, e
- o **Parser** transforma tokens em uma **AST** (árvore sintática),

o **Semântico**:

- valida **declarações**, **atribuições** e **usos de variáveis**;
- verifica **tipos e compatibilidade de operadores**;
- controla **escopos léxicos**;
- anota cada nó de expressão com o tipo inferido (`e.inferred`);
- prepara a AST para **otimização** e **geração de código**.

Esta documentação reflete **exatamente** a implementação em `semantics.cpp` que você forneceu, incluindo:

- suporte a **literais booleanos** (`Verdade` / `Mentira`);
- regras de promoção **Caractere → Inteiro**;
- regras específicas para o laço `Para` (início/fim aceitam `Inteiro/Caractere`, passo só `Inteiro`);
- suporte a **concatenação de strings** com operador `+`, controlado por `expr_has_string`;
- tratamento especial de strings **apenas para `Imprimir`** (tipo interno `Desconhecido`).

---

# 🧭 1. Papel do Semântico no Pipeline do Compilador CF

```text
Fonte → Lexer → Tokens
Tokens → Parser → AST Sintática
AST → [Semântico] → AST Anotada (tipos, escopos)
AST → Otimizador → AST Simplificada
AST → Codegen → Assembly (MIPS)
```

Responsabilidades do módulo `cf::Semantic`:

1. Garantir que **toda variável usada foi declarada** em algum escopo visível.
2. Verificar que **tipos combinam** em:
   - atribuições
   - inicializações
   - expressões aritméticas, relacionais e lógicas
3. Verificar que **condições de controle** (`Se`, `Enquanto`) são booleanas (`Logico`).
4. Checar regras específicas de laços `Para`.
5. Verificar que argumentos de `Imprimir` são **imprimíveis**.
6. Preencher o campo `inferred` de cada expressão (`Expr`) com um valor de `CfType`.

---

# 🧬 2. Arquitetura Geral

O módulo semântico se apoia em três pilares:

```text
semantic/
 ├── types.hpp      // enum CfType e funções auxiliares de tipos
 ├── scope.hpp      // pilha de escopos e tabela de símbolos
 └── semantics.hpp  // classe Semantic (analisador semântico em si)
```

## 2.1 Enum de tipos (`CfType`)

A linguagem CF possui 3 tipos primitivos visíveis ao usuário:

- `CfType::Inteiro` — `Inteiro` (4 bytes)
- `CfType::Logico` — `Logico` (1 byte, Verdade/Mentira)
- `CfType::Caractere` — `Caractere` (1 byte, ASCII)

E um tipo interno especial:

- `CfType::Desconhecido` — usado para:
  - strings (`ExprString`);
  - expressões tratadas exclusivamente como **texto para impressão** (concatenadas com `+`).

Funções auxiliares (em `types.hpp`):

```cpp
bool is_int(CfType t);
bool is_char(CfType t);
bool is_logical(CfType t);
CfType promote_if_needed(CfType t);   // ex.: Caractere → Inteiro para aritmética
CfType result_arith(CfType lhs, CfType rhs);
CfType result_rel(CfType lhs, CfType rhs);
CfType result_logic(CfType lhs, CfType rhs);
bool assign_compatible(CfType dest, CfType src);
```

---

## 2.2 Escopos e Tabela de Símbolos (`scope.hpp`)

`Semantic` utiliza uma pilha de escopos (`scopes_`) com operações:

- `enter()` — cria novo escopo
- `leave()` — destrói o escopo atual
- `declare(name, type)` — declara variável no escopo atual
- `find(name)` — busca variável de dentro para fora (escopo atual → global)

Regras principais:

- **não pode redeclarar a mesma variável no mesmo escopo**;
- variáveis de escopos mais internos **escondem** (sombras) variáveis de escopos externos (exceto regras especiais de `Para`).

---

## 2.3 Classe `Semantic`

A interface pública essencial:

```cpp
class Semantic {
public:
    void analyze(Program& p);

private:
    ScopeStack scopes_;

    void check_stmt(Stmt& s);
    void check_decl(StmtDecl& s);
    void check_assign(StmtAssign& s);
    void check_print(StmtPrint& s);
    void check_if(StmtIf& s);
    void check_while(StmtWhile& s);
    void check_for(StmtFor& s);

    CfType check_expr(Expr& e);
    CfType check_binary(ExprBinary& e);
    CfType check_group(ExprGroup& e);

    bool expr_has_string(Expr& e);

    [[noreturn]] void sem_error(const Position& pos, const std::string& msg);
};
```

---

# ⚙️ 3. Fluxo Principal — `analyze(Program& p)`

```cpp
void Semantic::analyze(Program& p) {
    scopes_.enter();       // escopo global
    for (auto& it : p.items) {
        check_stmt(*it);   // verifica declarações e comandos top-level
    }
    scopes_.leave();       // fecha escopo global
}
```

Ou seja:

- cria o **escopo global**;
- percorre todos os `Stmt` de topo (`p.items`);
- para cada statement, delega para `check_stmt` (despacho por tipo concreto);
- ao final, a AST está **anotada com tipos** e pronta para otimizador / codegen.

---

# 🧩 4. Verificação de Comandos (`check_stmt`)

```cpp
void Semantic::check_stmt(Stmt& s) {
    if (auto* d  = dynamic_cast<StmtDecl*>(&s))   return check_decl(*d);
    if (auto* a  = dynamic_cast<StmtAssign*>(&s)) return check_assign(*a);
    if (auto* pr = dynamic_cast<StmtPrint*>(&s))  return check_print(*pr);
    if (auto* w  = dynamic_cast<StmtWhile*>(&s))  return check_while(*w);
    if (auto* i  = dynamic_cast<StmtIf*>(&s))     return check_if(*i);
    if (auto* f  = dynamic_cast<StmtFor*>(&s))    return check_for(*f);
    if (auto* b  = dynamic_cast<StmtBlock*>(&s)) {
        scopes_.enter();
        for (auto& st : b->body)
            check_stmt(*st);
        scopes_.leave();
        return;
    }
    // Outros tipos de statements (se existirem futuramente) podem ser ignorados ou tratados aqui.
}
```

- `StmtBlock` representa um bloco `{ ... }` no nível semântico.
- Cada bloco abre um **novo escopo**, garantindo que variáveis declaradas dentro morram ao fim do bloco.

---

# 🧾 5. Declarações e Atribuições

## 5.1 Declarações — `check_decl(StmtDecl&)`

```cpp
void Semantic::check_decl(StmtDecl& s) {
    if (!scopes_.declare(s.name, s.type)) {
        sem_error(s.pos, "redeclaracao de '" + s.name + "' no mesmo escopo");
    }

    if (s.init) {
        CfType rhs = check_expr(*s.init);
        if (!assign_compatible(s.type, rhs)) {
            sem_error(s.pos, "inicializacao incompatível de '" + s.name + "'");
        }
    }
}
```

Regras:

1. variável não pode ser redeclarada **no mesmo escopo**;
2. se há inicialização (`Tipo nome <- Expr`), a expressão do lado direito (`Expr`) deve ser **compatível** com o tipo declarado do lado esquerdo, segundo `assign_compatible`.

### 5.1.1 Regras de `assign_compatible`

Conceitualmente:

- `dest == src` → permitido;
- `Inteiro <- Caractere` → permitido (promoção de `char` para `int`);
- qualquer outra combinação → erro.

Exemplos válidos:

```cf
Inteiro x <- 10;
Inteiro y <- 'A';       // ok: Caractere → Inteiro
Caractere c <- 'Z';
Logico b <- Verdade;
```

Exemplos inválidos:

```cf
Caractere c <- 300;     // int grande → char, proibido
Logico b <- 10;         // inteiro não é lógico
Inteiro x <- "oi";      // string não é atribuível
```

---

## 5.2 Atribuições — `check_assign(StmtAssign&)`

```cpp
void Semantic::check_assign(StmtAssign& s) {
    auto sym = scopes_.find(s.name);
    if (!sym) {
        sem_error(s.pos, "variavel '" + s.name + "' nao declarada");
    }
    CfType rhs = check_expr(*s.value);
    if (!assign_compatible(sym->type, rhs)) {
        sem_error(s.pos,
          "atribuição incompatível: destino " +
          std::to_string((int)sym->type) + " <- origem " + std::to_string((int)rhs));
    }
}
```

Regras:

1. variável do lado esquerdo deve existir (`scopes_.find`);
2. expressão do lado direito deve ser de tipo **compatível** com o tipo da variável;
3. caso contrário, é emitido erro detalhado com os códigos numéricos dos tipos (útil para depuração).

---

# 📣 6. Comando `Imprimir` — `check_print(StmtPrint&)`

```cpp
void Semantic::check_print(StmtPrint& s) {
    for (auto& e : s.args) {
        CfType t = check_expr(*e);

        // 1) Tipos "normais" imprimíveis
        if (t == CfType::Inteiro ||
            t == CfType::Logico  ||
            t == CfType::Caractere) {
            continue; // ok
        }

        // 2) Literal string direto
        if (dynamic_cast<ExprString*>(e.get())) {
            continue; // ok
        }

        // 3) Expressão que contém strings (ex: "Fatorial = " + r + "\n")
        if (expr_has_string(*e)) {
            continue; // ok (texto composto para impressão)
        }

        // 4) Qualquer outro caso: erro
        sem_error(e->pos, "tipo invalido para Imprimir");
    }
}
```

A lógica é **bem específica**:

1. São imprimíveis diretamente:
   - `Inteiro`
   - `Logico`
   - `Caractere`
2. **Strings literais** (`ExprString`) são sempre aceitas.
3. **Expressões que contenham strings** (como `"Area = " + a + "\n"`) também são aceitas, via `expr_has_string`.
4. Qualquer outra coisa (por exemplo expressão puramente numérica que resultou em `Desconhecido`, ou alguma estrutura futura não suportada) → erro.

### 6.1 `expr_has_string(Expr& e)`

```cpp
bool Semantic::expr_has_string(Expr& e) {
    if (dynamic_cast<ExprString*>(&e))
        return true;

    if (auto* g = dynamic_cast<ExprGroup*>(&e))
        return expr_has_string(*g->inner);

    if (auto* b = dynamic_cast<ExprBinary*>(&e))
        return expr_has_string(*b->lhs) || expr_has_string(*b->rhs);

    return false;
}
```

- Retorna `true` se a expressão é:
  - um literal string, ou
  - um grupo contendo string, ou
  - uma expressão binária contendo string em qualquer subárvore.

Esse utilitário é crucial para permitir **concatenação textual** com o operador `+`.

---

# 🔁 7. Estruturas de Controle

## 7.1 `check_if(StmtIf&)` — `Se` / `Senao`

```cpp
void Semantic::check_if(StmtIf& s) {
    CfType c = check_expr(*s.cond);
    if (!is_logical(c))
        sem_error(s.cond->pos, "condicao de 'Se' deve ser Logico");

    scopes_.enter();
    for (auto& st : s.then_body)
        check_stmt(*st);
    scopes_.leave();

    scopes_.enter();
    for (auto& st : s.else_body)
        check_stmt(*st);
    scopes_.leave();
}
```

Regras:

- a condição (`s.cond`) deve ser do tipo **Logico**;
- o corpo `then` e o corpo `else` possuem **escopos próprios** (variáveis declaradas dentro deles não vazam).

---

## 7.2 `check_while(StmtWhile&)` — `Enquanto`

```cpp
void Semantic::check_while(StmtWhile& s) {
    CfType c = check_expr(*s.cond);
    if (!is_logical(c))
        sem_error(s.cond->pos, "condicao de 'Enquanto' deve ser Logico");

    scopes_.enter();
    for (auto& st : s.body)
        check_stmt(*st);
    scopes_.leave();
}
```

Regras:

- a condição do `Enquanto` deve ser **Logico**;
- o corpo do loop tem escopo próprio (variáveis internas ao loop).

---

## 7.3 `check_for(StmtFor&)` — `Para`

Implementação:

```cpp
void Semantic::check_for(StmtFor& s) {
    // Escopo do 'for': variável de controle local do loop (Inteiro)
    scopes_.enter();
    if (!scopes_.declare(s.var, CfType::Inteiro)) {
        sem_error(s.pos,
          "variavel de controle '" + s.var + "' já existe no escopo do 'Para'");
    }

    CfType tb = check_expr(*s.begin);
    CfType te = check_expr(*s.end);
    if (!(is_int(promote_if_needed(tb)) && is_int(promote_if_needed(te)))) {
        sem_error(s.pos,
          "inicio/fim do 'Para' devem ser Inteiro/Caractere (promovidos a Inteiro)");
    }

    if (s.step.has_value()) {
        CfType ts = check_expr(**s.step);
        // Para o passo, exigimos estritamente tipo Inteiro
        if (!is_int(ts)) {
            sem_error((**s.step).pos, "passo do 'Para' deve ser Inteiro");
        }
    }

    for (auto& st : s.body)
        check_stmt(*st);
    scopes_.leave();
}
```

Regras detalhadas:

1. O `Para` **abre um novo escopo**.
2. A variável de controle (`s.var`) é sempre declarada como **Inteiro**.
   - Se já existir variável com o mesmo nome **no escopo do `Para`**, erro:
     > `variavel de controle 'i' já existe no escopo do 'Para'`
3. **Início** (`s.begin`) e **fim** (`s.end`) devem ser `Inteiro` ou `Caractere`:
   - ambos passam por `promote_if_needed` e depois são checados com `is_int`.
   - `Caractere` é promovido a `Inteiro` para aritmética de laço.
4. **Passo** (`s.step`), se presente:
   - deve ser estritamente de tipo **Inteiro** (`is_int(ts)`).
   - não é aceita promoção de `Caractere` aqui.
5. O corpo do `Para` é verificado normalmente (`check_stmt`) dentro desse escopo.

Exemplos válidos:

```cf
Para i em (0, 10, 1) {
    Imprimir(i + "\n");
}

Para j em ('A', 'Z', 1) {   // início/fim chars, promovidos para Inteiro
    Imprimir(j + "\n");
}

Para k em (10, 0, -2) {
    Imprimir(k + "\n");
}
```

Exemplos inválidos:

```cf
Para i em (0, 10, 'a') { }    // passo char → erro: passo deve ser Inteiro
Para i em (Verdade, 10, 1) { } // início lógico → erro: inicio/fim devem ser Inteiro/Caractere
```

---

# 🔣 8. Verificação de Expressões

## 8.1 `check_expr(Expr&)`

```cpp
CfType Semantic::check_expr(Expr& e) {
    if (auto* i = dynamic_cast<ExprInteger*>(&e)) {
        e.inferred = CfType::Inteiro;
        return e.inferred;
    }
    if (auto* c = dynamic_cast<ExprChar*>(&e)) {
        e.inferred = CfType::Caractere;
        return e.inferred;
    }
    if (auto* s = dynamic_cast<ExprString*>(&e)) {
        // Strings só são válidas como argumento de Imprimir; não participam de operações
        e.inferred = CfType::Desconhecido;
        return e.inferred;
    }
    if (auto* id = dynamic_cast<ExprIdent*>(&e)) {
        auto sym = scopes_.find(id->name);
        if (!sym)
            sem_error(e.pos, "variavel '" + id->name + "' nao declarada");
        e.inferred = sym->type;
        return e.inferred;
    }
    if (auto* b = dynamic_cast<ExprBinary*>(&e)) {
        return check_binary(*b);
    }
    if (auto* g = dynamic_cast<ExprGroup*>(&e)) {
        return check_group(*g);
    }
    if (auto* b = dynamic_cast<ExprBool*>(&e)) {
        e.inferred = CfType::Logico;
        return e.inferred;
    }

    // fallback
    e.inferred = CfType::Desconhecido;
    return e.inferred;
}
```

Resumo por tipo de expressão:

- `ExprInteger` → `Inteiro`
- `ExprChar` → `Caractere`
- `ExprString` → `Desconhecido` (apenas para `Imprimir`)
- `ExprIdent` → tipo consultado em `scopes_.find` (erro se não declarada)
- `ExprBinary` → delega a `check_binary`
- `ExprGroup` → delega a `check_group`
- `ExprBool` (`Verdade` / `Mentira`) → `Logico`
- qualquer outro tipo não previsto → `Desconhecido`

---

## 8.2 Expressões Binárias — `check_binary(ExprBinary&)`

```cpp
CfType Semantic::check_binary(ExprBinary& e) {
    CfType lt = check_expr(*e.lhs);
    CfType rt = check_expr(*e.rhs);

    switch (e.op) {
    case BinOp::Add: {
        // 🔹 Concatenação de strings
        if (expr_has_string(*e.lhs) || expr_has_string(*e.rhs)) {
            e.inferred = CfType::Desconhecido;
            return e.inferred;
        }

        // Caso contrário, aritmética normal
        CfType r = result_arith(lt, rt);
        if (r == CfType::Desconhecido) {
            sem_error(e.pos, "operacao aritmetica requer Inteiro (Char promove a Inteiro)");
        }
        e.inferred = r;
        return r;
    }

    // Aritméticos puros
    case BinOp::Sub:
    case BinOp::Mul:
    case BinOp::Div:
    case BinOp::Mod:
    case BinOp::Pow: {
        CfType r = result_arith(lt, rt);
        if (r == CfType::Desconhecido) {
            sem_error(e.pos, "operacao aritmetica requer Inteiro (Char promove a Inteiro)");
        }
        e.inferred = r;
        return r;
    }

    // Relacionais
    case BinOp::Eq:
    case BinOp::Ne:
    case BinOp::Gt:
    case BinOp::Lt:
    case BinOp::Ge:
    case BinOp::Le: {
        CfType r = result_rel(lt, rt);
        if (r == CfType::Desconhecido) {
            sem_error(e.pos, "operacao relacional requer Inteiro/Caractere (promovidos)");
        }
        e.inferred = r;
        return r;
    }

    // Lógicos
    case BinOp::And:
    case BinOp::Or: {
        CfType r = result_logic(lt, rt);
        if (r == CfType::Desconhecido) {
            sem_error(e.pos, "operacao logica requer Logico");
        }
        e.inferred = r;
        return r;
    }
    }

    e.inferred = CfType::Desconhecido;
    return e.inferred;
}
```

### 8.2.1 Regras por categoria

#### ➤ Aritméticos (`+`, `-`, `*`, `/`, `%`, `**`)

- Operandos devem ser **numéricos**: `Inteiro` ou `Caractere`.
- `Caractere` é promovido a `Inteiro` para a operação.
- Resultado sempre `Inteiro`.
- Erro se qualquer operando for:
  - `Logico`
  - `Desconhecido` (exceto no caso especial de `+` com strings)
  - ou combinação incompatível segundo `result_arith`.

Mensagem de erro:

> `operacao aritmetica requer Inteiro (Char promove a Inteiro)`

#### ➤ Relacionais (`=`, `<>`, `<`, `>`, `<=`, `>=`)

- Operandos devem ser comparáveis, normalmente:
  - `Inteiro` com `Inteiro`
  - `Caractere` com `Caractere`
  - `Inteiro` com `Caractere` (com promoção)
- Resultado sempre `Logico`.
- Erro para combinações inválidas (por exemplo, `Logico` com `Inteiro`).

Mensagem de erro:

> `operacao relacional requer Inteiro/Caractere (promovidos)`

#### ➤ Lógicos (`And`, `Or` = `&`, `|`, `^`)

- Ambos os operandos devem ser `Logico`.
- Resultado é `Logico`.

Mensagem de erro:

> `operacao logica requer Logico`

#### ➤ Caso especial: `+` com strings

Se **qualquer lado** da soma contém string (`expr_has_string(lhs/rhs) == true`):

- a expressão **não** é tratada como aritmética, mas como **concatenação textual**;
- o tipo inferido é `CfType::Desconhecido`;
- essas expressões são aceitas como argumentos de `Imprimir`, mas não podem participar de aritmética/relacionais numéricos.

Exemplos:

```cf
Imprimir("Area = " + a + "\n");   // ok: concatenação textual

Inteiro x <- "Area = " + a;       // erro semântico na atribuição:
                                  // tipo do RHS é Desconhecido, não compatível com Inteiro
```

---

## 8.3 Grupos — `check_group(ExprGroup&)`

```cpp
CfType Semantic::check_group(ExprGroup& e) {
    e.inferred = check_expr(*e.inner);
    return e.inferred;
}
```

- Apenas delega a verificação para a expressão interna;
- o grupo **não altera** o tipo da expressão, apenas sua associação.

---

# 🚨 9. Erros Semânticos — `sem_error`

```cpp
[[noreturn]] void Semantic::sem_error(const Position& pos, const std::string& msg) {
    Diagnostic d;
    d.phase = "semantic";
    d.pos = pos;
    d.message = msg;
    throw CompileError(d);
}
```

- `phase = "semantic"` identifica claramente a fase do erro.
- A posição (`pos`) vem dos nós da AST e permite mostrar linha/coluna corretas.
- A mensagem é sempre em português, com foco didático:
  - `"variavel 'x' nao declarada"`
  - `"inicializacao incompatível de 'y'"`
  - `"condicao de 'Se' deve ser Logico"`
  - etc.

Em conjunto com o sistema de `Diagnostic` e `current_line_text()` (do lexer), o compilador CF produz mensagens amigáveis e precisas para o usuário.

---

# 🧪 10. Exemplo Comentado

Considere:

```cf
Inteiro n <- 5;
Inteiro i;
Logico cond <- Verdade;

Para i em (0, n, 1) {
    Se i < 3 {
        Imprimir("i = " + i + "\n");
    }
    Senao {
        Imprimir("fim\n");
    }
}

Imprimir("n = " + n + "\n");
```

Análise semântica:

1. `Inteiro n <- 5;`
   - declara `n` como `Inteiro`
   - inicializa com literal inteiro → ok
2. `Inteiro i;`
   - declara `i` como `Inteiro`
3. `Logico cond <- Verdade;`
   - declara `cond` como `Logico`
   - inicializa com literal booleano → ok
4. `Para i em (0, n, 1) { ... }`
   - entra em escopo do `Para`
   - declara nova `i` (variável de controle, Inteiro) no escopo do loop  
     (esconde a `i` externa, o que é permitido aqui)
   - `0` → `Inteiro` → ok como início
   - `n` → `Inteiro` → ok como fim
   - `1` → `Inteiro` → ok como passo
5. `Se i < 3 { ... }`
   - `i < 3` → relacional int-int → tipo `Logico` → ok para condição
6. `Imprimir("i = " + i + "\n");`
   - expressão `"i = " + i + "\n"` contém string → `expr_has_string == true`
   - tipo inferido `Desconhecido`, mas aceita por `check_print` → ok
7. `Imprimir("n = " + n + "\n");`
   - mesma lógica de concatenação textual → ok

Nenhum erro é reportado; a AST é anotada com todos os tipos necessários para geração de código.

---

# 🎯 11. Conclusão

O módulo **Semântico (`cf/semantic`)** do compilador CF:

- implementa um **checador de tipos e escopos** robusto e pedagógico;
- garante:
  - variáveis declaradas antes do uso;
  - tipos compatíveis em atribuições e operações;
  - condições booleanas em `Se` e `Enquanto`;
  - regras específicas e bem definidas para laços `Para`;
  - uso seguro e controlado de **strings** e **concatenação textual**;
- anota a AST com `CfType` em `Expr::inferred`, preparando o terreno para:
  - **otimizações** (constant folding, simplificações);
  - **geração de código** MIPS/RISC-V correta e eficiente.

Essa documentação, alinhada ao código C++ atual, pode ser usada como **referência oficial** do módulo semântico, tanto em um contexto acadêmico quanto em um projeto open-source de ensino de compiladores.
