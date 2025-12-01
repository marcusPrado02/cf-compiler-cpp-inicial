# 🏗️ Geração de Código MIPS — Documentação Oficial

### Diretório: `cf/codegen/` — Backend MIPS do Compilador CF (Compila Fofo)

Este documento descreve em detalhes como o módulo **`Codegen`** transforma a **AST anotada semanticamente** em **código assembly MIPS** executável.

Ele está totalmente alinhado ao código atual de `codegen.cpp`, incluindo:

- uso de `_start` como _entrypoint_ (sem `main`);
- inicialização explícita do `$sp` para simuladores (cpulator / SPIM / MARS / Ripes);
- alocação de variáveis na **stack** via `frame_size_` + `VarInfo`;
- seções `.data` (pool de strings) e `.text` na **ordem correta**: `.data` primeiro, `.text` depois;
- implementação de:
  - operações aritméticas, relacionais e lógicas;
  - operador de potência `**` via loop;
  - representação de booleanos (`0x00` / `0xFF`);
  - laços `Enquanto` e `Para` (com detecção de passo negativo);
  - impressão com **concatenação de strings** (`"texto" + expr`).

> A primeira versão dessa documentação foi criada com base no design original do backend MIPS, e aqui ela é **refinada e sincronizada** com o código fonte mais recente. fileciteturn3file0

---

## 🧭 1. Visão Geral do Backend MIPS

Fluxo a partir da AST:

```text
AST (Program)
   ↓
cf::Codegen::emit(const Program&)
   ↓
string com assembly MIPS:
  .data      # pool de strings
  .text      # código principal (_start)
```

O `Codegen` segue alguns princípios de compiladores reais:

- **Avaliação de expressões por acumulador**: resultado em `$a0`.
- **Variáveis alocadas na pilha**: offsets negativos relativos a `$fp`.
- **Escopos léxicos**: cada bloco abre/fecha escopo com desalocação de memória.
- **Labels únicos**: gerados por `new_label(base)` (`L_base_N`).
- **Pool de strings**: literais armazenadas em `.data` e reutilizadas.

---

## 🧱 2. Estruturas Internas Importantes

### 2.1 `VarInfo` — Metadados de Variáveis

Cada variável CF é representada por:

```cpp
struct VarInfo {
    CfType type;   // Inteiro, Logico ou Caractere
    int offset;    // offset relativo a $fp (sempre negativo ou zero)
    int size;      // tamanho em bytes (4 para Inteiro, 1 para Logico/Caractere)
    int padding;   // bytes extras usados para alinhamento na pilha
};
```

- `offset` é utilizado em instruções como `lw` / `sw` (inteiro) e `lbu` / `sb` (1 byte).
- `padding` é usado para manter alinhamentos adequados via `alloc_bytes_aligned`.

### 2.2 Pilha de Escopos: `scopes_`

```cpp
std::vector<std::unordered_map<std::string, VarInfo>> scopes_;
```

- Cada elemento do vetor é um **escopo léxico**.
- Um escopo é um `unordered_map` nome → `VarInfo`.
- Topo da pilha = escopo atual.

Operações:

- `enter_scope()` → `scopes_.emplace_back()`
- `leave_scope()`:
  - soma `size + padding` de todas as variáveis daquele escopo;
  - move `$sp` para cima (liberando memória);
  - diminui `frame_size_`;
  - remove o escopo do vetor.

---

## 📐 3. Alocação na Pilha e Alinhamento

### 3.1 `frame_size_` e `alloc_bytes_aligned`

`frame_size_` rastreia quantos bytes já foram alocados **abaixo de `$fp`**:

- início: `frame_size_ = 0`;
- cada nova variável adiciona espaço na pilha;
- offsets são **negativos** em relação a `$fp`.

Função central:

```cpp
int Codegen::alloc_bytes_aligned(int bytes, int align, int& outPad);
```

Passos:

1. Calcula `mis = frame_size_ % align`.
2. Se `mis != 0`, gera `pad = align - mis`:
   - emite `addiu $sp, $sp, -pad   # padding align`;
   - incrementa `frame_size_` em `pad`.
3. Em seguida, aloca `bytes`:
   - emite `addiu $sp, $sp, -bytes # alloc bytes`;
   - incrementa `frame_size_` em `bytes`.
4. `outPad = pad` e retorna `-frame_size_` (offset relativo a `$fp`).

### 3.2 `declare(name, ty)` — Reservando Espaço para Variáveis

```cpp
VarInfo& Codegen::declare(const std::string& name, CfType ty)
{
    if (scopes_.empty())
        enter_scope();

    auto& top = scopes_.back();
    int size  = (ty == CfType::Inteiro) ? 4 : 1;
    int align = (ty == CfType::Inteiro) ? 4 : 1;

    int pad = 0;
    int offset = alloc_bytes_aligned(size, align, pad);

    VarInfo v{ty, offset, size, pad};
    auto [it, ok] = top.emplace(name, v);
    if (!ok) {
        // Se reusar nome no mesmo escopo, sobrescreve VarInfo
        it->second = v;
    }
    return it->second;
}
```

- Inteiros são alinhados em 4 bytes.
- Lógicos/Caracteres são de 1 byte (sem alinhamento extra).
- Em `leave_scope()`, todo o espaço (`size + padding`) é liberado com um único `addiu $sp, $sp, to_free`.

---

## 📦 4. Pool de Strings e Seção `.data`

### 4.1 Pool de Strings: `put_string`

```cpp
int Codegen::put_string(const std::string& s);
```

- Se a string já está no pool, reutiliza o mesmo índice.
- Caso contrário, insere `StrLit{s, idx}` em `str_pool_`.
- Retorna um índice `idx` que será usado em labels `L.str.idx`.

### 4.2 Emissão de `.data`: `emit_rodata()`

Apesar do nome, o código usa **`.data`** (não `.rodata`):

```asm
.data
L.str.0:
  .asciiz "texto...\n"
L.str.1:
  .asciiz "outro"
```

Escapes suportados:

- `\n` → `\\n` na string da diretiva
- `\t` → `\\t`
- `\\` → `\\\\`
- `\"` → `\\\"`

Qualquer outro escape desconhecido é emitido como veio (ex.: `\x`).

O buffer construído é guardado em `data_` e **concatenado antes de `.text`** na saída final.

---

## 🔤 5. Normalização de Booleanos

Na linguagem CF:

- `Verdade` é representado como `0xFF` (byte cheio).
- `Mentira` como `0x00`.

### 5.1 Função `emit_bool_normalize()`

```cpp
void Codegen::emit_bool_normalize()
{
    ln("  sltu $t0, $zero, $a0");
    ln("  subu $a0, $zero, $t0");
    ln("  andi $a0, $a0, 0x00FF");
}
```

Interpretação:

1. `sltu $t0, $zero, $a0` → `$t0 = 1` se `$a0 != 0`, senão 0.
2. `subu $a0, $zero, $t0`:
   - se `$t0 = 1` → `$a0 = 0xFFFFFFFF`;
   - se `$t0 = 0` → `$a0 = 0x00000000`.
3. `andi $a0, $a0, 0x00FF` → mantém apenas o byte menos significativo:
   - `0x00000000` → `0x00`;
   - `0x000000FF` → `0xFF`.

Essa rotina é usada:

- em operações lógicas (`And`, `Or`);
- ao armazenar variáveis `Logico` com `emit_store_a0_to_var`.

---

## 🧮 6. Geração de Expressões

### 6.1 `emit_expr(const Expr& e)` — Entrada Única

`emit_expr` é o **ponto central** para gerar código de expressões. Ela:

- verifica o tipo concreto da expressão via `dynamic_cast`;
- gera MIPS específico;
- garante que o **resultado final** esteja em `$a0`.

Casos suportados (alinhados ao código atual):

#### 6.1.1 Literais Inteiros (`ExprInteger`)

```cpp
li $a0, <valor>
```

Ex.:

```asm
  li $a0, 42
```

#### 6.1.2 Literais de Caractere (`ExprChar`)

Trata escapes simples (`\n`, `\t`, `\\`, `\'`, `\"`) e converte para valor ASCII:

```asm
  li $a0, <codigo_ascii>
```

Ex.:

- `'A'` → `li $a0, 65`
- `'\n'` → `li $a0, 10`

#### 6.1.3 Strings (`ExprString`)

Strings não são valores numéricos; elas são **impressas diretamente**:

```asm
  la $a0, L.str.N
  li $v0, 4      # print_string
  syscall
```

Depois disso, `$a0` fica “sem significado específico”, pois o objetivo era o efeito colateral de impressão.

#### 6.1.4 Identificadores (`ExprIdent`)

Carregam o valor da variável associada:

```cpp
emit_load_var_to_a0(*v);
```

Que gera:

- Inteiro: `lw $a0, offset($fp)`
- Lógico/Caractere: `lbu $a0, offset($fp)` (carrega 1 byte sem sinal).

#### 6.1.5 Booleanos (`ExprBool`)

```asm
  li $a0, 0xFF   # Verdade
  li $a0, 0x00   # Mentira
```

#### 6.1.6 Expressões Binárias (`ExprBinary`)

Delegadas para:

```cpp
emit_expr_bin(*b);
```

#### 6.1.7 Expressões Agrupadas (`ExprGroup`)

```cpp
emit_expr(*e.inner);
```

Apenas passa adiante, respeitando a precedência da gramática.

---

### 6.2 `emit_expr_bin(const ExprBinary& b)` — Operações Binárias

Sequência padrão:

1. Avalia LHS em `$a0`.
2. Empilha `$a0` (LHS) na stack:

   ```asm
   addiu $sp, $sp, -4
   sw   $a0, 0($sp)
   ```

3. Avalia RHS em `$a0`.
4. Move RHS → `$t1`, recupera LHS → `$t0`:

   ```asm
   move $t1, $a0
   lw   $t0, 0($sp)
   addiu $sp, $sp, 4
   ```

5. Executa o operador, deixando o resultado final em `$a0`.

#### 6.2.1 Aritméticos (`+`, `-`, `*`, `/`, `%`)

```asm
add $a0, $t0, $t1    # BinOp::Add
sub $a0, $t0, $t1    # BinOp::Sub
mul $a0, $t0, $t1    # BinOp::Mul

div $t0, $t1         # BinOp::Div
mflo $a0             # quociente

div $t0, $t1         # BinOp::Mod
mfhi $a0             # resto
```

#### 6.2.2 Potência (`**`, `BinOp::Pow`)

Implementada com loop:

```asm
  li   $a0, 1        # acc = 1
  move $t2, $t0      # base
  move $t3, $t1      # exp
L_pow_loop:
  beq  $t3, $zero, L_pow_end
  mul  $a0, $a0, $t2
  addi $t3, $t3, -1
  j    L_pow_loop
L_pow_end:
```

- Assume expoente inteiro não negativo.
- Resultado final em `$a0`.

#### 6.2.3 Relacionais (`=`, `<>`, `<`, `>`, `<=`, `>=`)

Exemplo `Eq` (`==`):

```asm
  subu $t2, $t0, $t1
  sltu $a0, $zero, $t2   # a0 = (t2 != 0)
  xori $a0, $a0, 1       # inverte → (t2 == 0)
```

Outros:

- `Ne` (`<>`): `subu`, `sltu` direto (sem inverter).
- `Lt` (`<`): `slt $a0, $t0, $t1`
- `Gt` (`>`): `slt $a0, $t1, $t0`
- `Le` (`<=`): `slt $a0, $t1, $t0` seguido de `xori` (negação de `>`).
- `Ge` (`>=`): `slt $a0, $t0, $t1` seguido de `xori` (negação de `<`).

> O resultado preliminar é `0` ou `1`; caso seja atribuído a um `Logico`, ele será normalizado para `0xFF/0x00` por `emit_store_a0_to_var`.

#### 6.2.4 Lógicos (`And`, `Or`)

Operadores lógicos recebem quaisquer inteiros como **truthy** (≠0):

```asm
sltu $t0, $zero, $t0    # t0 = (lhs != 0) ? 1 : 0
sltu $t1, $zero, $t1    # t1 = (rhs != 0) ? 1 : 0
and  $a0, $t0, $t1      # And
# ou
or   $a0, $t0, $t1      # Or
```

Em seguida, `emit_bool_normalize()` garante que o resultado seja `0xFF` ou `0x00`.

---

## 🧾 7. Statements (Declarações, Atribuições, Controle de Fluxo)

### 7.1 `emit_decl(const StmtDecl& s)`

1. `VarInfo& v = declare(s.name, s.type);`
2. Se houver inicialização (`s.init`):
   - `emit_expr(*s.init);` → resultado em `$a0`;
   - `emit_store_a0_to_var(v, s.init->inferred);`.

### 7.2 `emit_store_a0_to_var(const VarInfo& v, CfType rhsTy)`

- Inteiro:

  ```asm
  sw $a0, offset($fp)
  ```

- Caractere:

  ```asm
  andi $a0, $a0, 0x00FF
  sb   $a0, offset($fp)
  ```

- Lógico:

  ```asm
  # normaliza para 0xFF/0x00
  emit_bool_normalize();
  sb $a0, offset($fp)
  ```

### 7.3 `emit_assign(const StmtAssign& s)`

1. Localiza a variável (`lookup`).
2. `emit_expr(*s.value);`
3. `emit_store_a0_to_var(*v, s.value->inferred);`

### 7.4 `emit_print(const StmtPrint& s)`

Para cada argumento `e`:

- Se **contém strings** (ver `expr_has_string`), usa `emit_print_concat(e)`.
- Caso contrário, usa `emit_print_expr(e)`.

#### 7.4.1 `expr_has_string(const Expr& e)`

- `ExprString` → true.
- `ExprGroup` → recursão em `inner`.
- `ExprBinary` → recursão em `lhs` e `rhs`.
- Outros → false.

#### 7.4.2 `emit_print_expr(const Expr& e)`

- Se `ExprString` puro:
  - apenas chama `emit_expr(e)` (que já faz syscall 4).
- Senão:
  - `emit_expr(e);` → resultado em `$a0`.
  - Se `e.inferred == CfType::Caractere`:
    - `li $v0, 11` (print_char).
  - Caso contrário (`Inteiro`/`Logico`):
    - `li $v0, 1` (print_int).
  - `syscall`.

#### 7.4.3 `emit_print_concat(const Expr& e)`

Trata `+` como **concatenação textual** quando houver strings:

- Se for `ExprBinary` com `op == BinOp::Add` e qualquer lado tiver string:
  - faz recursão em `lhs` e depois em `rhs` (ordem esquerda→direita);
  - **não** gera instrução de soma aritmética.
- Caso contrário:
  - delega a `emit_print_expr(e)`.

Exemplo CF:

```cf
Imprimir("Area = " + a + "\n");
```

Gera, na prática, algo como:

```asm
# "Area = "
la $a0, L.str.0
li $v0, 4
syscall

# a (inteiro)
... avalia a em $a0 ...
li $v0, 1
syscall

# "\n"
la $a0, L.str.1
li $v0, 4
syscall
```

---

### 7.5 `emit_if(const StmtIf& s)` — `Se` / `Senao`

1. Gera labels: `Lelse = new_label("else")`, `Lend = new_label("endif")`.
2. Avalia condição em `$a0`.
3. `beq $a0, $zero, Lelse` → se cond == 0 → vai pro else.
4. Then:
   - `enter_scope()`;
   - emite `then_body`;
   - `leave_scope()`;
   - `j Lend`.
5. Else:
   - label `Lelse:`;
   - `enter_scope()`;
   - emite `else_body`;
   - `leave_scope()`.
6. Label `Lend:` fecha o comando.

---

### 7.6 `emit_while(const StmtWhile& s)` — `Enquanto`

1. Labels: `Lcond = new_label("while.cond")`, `Lend = new_label("while.end")`.
2. `Lcond:` avalia condição em `$a0`.
3. `beq $a0, $zero, Lend` → se falsa, sai do loop.
4. Corpo:
   - `enter_scope()`, emite body, `leave_scope()`.
5. `j Lcond`.
6. `Lend:` encerra o laço.

---

### 7.7 `emit_for(const StmtFor& s)` — `Para i em (begin, end, [step])`

Semântica alvo:

- `i` é declarado como `Inteiro` e local ao laço.
- `begin`, `end` e `step` são expressões arbitrárias, mas:
  - `begin` e `end` são recomputados conforme necessário;
  - `step` é recomputado a cada iteração (se presente).
- Há uma otimização: se `step` for **literal inteiro negativo**, o laço é tratado como **decrescente** (`i >= end`).

Passos:

1. `enter_scope()`.
2. `VarInfo& vi = declare(s.var, CfType::Inteiro);`
3. Inicialização:

   ```cpp
   emit_expr(*s.begin);
   emit_store_a0_to_var(vi, CfType::Inteiro);
   ```

4. Detecção de laço descendente:

   ```cpp
   bool descending = false;
   if (s.step.has_value()) {
       if (auto* lit = dynamic_cast<const ExprInteger*>(s.step->get())) {
           int val = std::stoi(lit->digits);
           if (val < 0) descending = true;
       }
   }
   ```

5. Geração de labels: `Lcond`, `Lbody`, `Linc`, `Lend`.

6. Condição (`Lcond:`):

   ```asm
   Lcond:
     # carrega i
     ... emit_load_var_to_a0(vi) ...
     move $t0, $a0

     # recomputa end
     ... emit_expr(*s.end) ...
     move $t1, $a0
   ```

   - Se **não descendente**: laço `while (i <= end)`:

     ```asm
     slt  $a0, $t1, $t0       # end < i ?
     bne  $a0, $zero, Lend    # se end < i → sai
     ```

   - Se **descendente**: laço `while (i >= end)`:

     ```asm
     slt  $a0, $t0, $t1       # i < end ?
     bne  $a0, $zero, Lend    # se i < end → sai
     ```

7. Corpo (`Lbody:`):

   ```asm
   Lbody:
     enter_scope()
       ... corpo do Para ...
     leave_scope()
   ```

8. Incremento (`Linc:`):

   ```asm
   Linc:
     # i atual → $t0
     emit_load_var_to_a0(vi)
     move $t0, $a0

     # step → $t2 (se não houver, step = 1)
     if (s.step.has_value()) {
         emit_expr(*(*s.step));
         move $t2, $a0;
     } else {
         li $t2, 1;
     }

     add  $t0, $t0, $t2      # i = i + step
     move $a0, $t0
     emit_store_a0_to_var(vi, CfType::Inteiro)

     j Lcond
   ```

9. Fim do laço:

   ```asm
   Lend:
     leave_scope()
   ```

---

## 🧱 8. Programas Top-Level e `_start`

### 8.1 `emit_program(const Program& p)`

1. Emite cabeçalho de código:

   ```asm
   .text
   .globl _start
   _start:
   ```

2. Inicializa stack e frame pointer:

   ```asm
   li   $sp, 0x7fffeffc    # stack alto (bom para simuladores)
   move $fp, $sp           # frame-pointer = $sp
   ```

3. `enter_scope()` do escopo global.
4. Emite todos os statements top-level (`emit_stmt(*it)`).
5. `leave_scope()` libera variáveis globais (implementadas como locais de `_start`).
6. Epílogo:

   ```asm
   li $v0, 10   # syscall 10 = exit
   syscall
   ```

> Não há `main` em C; `_start` é o ponto de entrada direto, o que torna o assembly mais simples e adequado a simuladores educacionais.

### 8.2 `emit(const Program& p)` — Função Final

```cpp
std::string Codegen::emit(const Program& p)
{
    text_.clear();
    data_.clear();
    frame_size_ = 0;
    label_id_   = 0;
    scopes_.clear();
    str_pool_.clear();

    emit_program(p);  // preenche text_ e str_pool_
    emit_rodata();    // preenche data_ com .data e strings

    std::string finalAsm;
    finalAsm.reserve(data_.size() + text_.size());
    finalAsm += data_;   // primeiro .data
    finalAsm += text_;   // depois .text
    return finalAsm;
}
```

- `data_` e `text_` são buffers separados.
- Ordem final: `.data` (strings) antes de `.text`.
- Adequado para assemblers/simuladores MIPS que aceitam múltiplas seções.

---

## 🎯 9. Conclusão

O módulo **`cf::Codegen`**:

- implementa um backend MIPS **didático e profissional**, com:
  - alocação de variáveis na stack;
  - tratamento consistente de booleanos (0x00/0xFF);
  - expressões avaliada em `$a0`;
  - suporte a `**`, `%`, `&&`, `||`, comparações e unário `-` (já convertido em `0 - expr` pelo parser);
  - laços `Enquanto` e `Para` com detecção de passo negativo;
  - impressão com concatenação textual de forma incremental;
- produz código compatível com **SPIM, MARS, Ripes, cpulator**;
- foi projetado para ser legível, modular e fácil de estender (por exemplo, para funções futuras, arrays, etc.).

Esta documentação pode ser usada como **referência oficial do backend MIPS** do compilador CF, tanto para fins acadêmicos (disciplinas de compiladores / arquitetura) quanto para apresentação em portfólio e projetos open-source de ensino de compiladores.
