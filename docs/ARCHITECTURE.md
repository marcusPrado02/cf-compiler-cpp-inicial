# 🚀 Compilador CF (Compila Fofo)

### Arquitetura Completa e Guia Definitivo

Este documento descreve **toda a arquitetura do compilador da linguagem CF (Compila Fofo)**, implementado em C++ moderno, seguindo princípios de **modularização, clareza, testabilidade, extensibilidade e separação de responsabilidades**, como esperado em um compilador profissional.

A linguagem CF é definida no enunciado oficial do professor Felipe Belém e este compilador implementa **todas as suas regras léxicas, sintáticas, semânticas, de geração de código e otimização**.

---

# 📦 Visão Geral da Arquitetura

O compilador é organizado em **7 grandes camadas**, formando um pipeline clássico:

```
Fonte CF
   ↓
[Léxico] → Tokens
   ↓
[Sintático] → AST
   ↓
[Semântico] → AST anotada + Tabela de Símbolos
   ↓
[Otimizações] → AST otimizada
   ↓
[Codegen] → MIPS Assembly
   ↓
Execução no simulador (Ripes / SPIM / MARS)
```

---

# 🧩 1. Common (`cf/common`)

Contém utilitários essenciais compartilhados por todas as outras camadas.

### ✔ `Position`

Representa uma posição exata no código-fonte: linha, coluna e arquivo.

### ✔ `Diagnostic`

Estrutura completa de erro:

- fase (léxico, sintático, semântico, codegen)
- mensagem detalhada
- posição
- linha de código causadora
- sugestão de correção

### ✔ `CompileError`

Exceção lançada em qualquer fase do compilador.

---

# 🔤 2. Léxico (`cf/lexer`) — Scanner / Tokenizador

O analisador léxico transforma texto CF em **tokens**.

### Tokens reconhecidos:

- **Palavras-chave:** Inteiro, Logico, Caractere, Enquanto, Se, Senao, Para, Imprimir, Verdade, Mentira
- **Operadores aritméticos:** + - / % \* \*\*
- **Operadores lógicos:** = <> < <= > >= & ^
- **Símbolos:** ; { } ( ) , "<string>"
- **Comentários:**
  - `$ …` (linha)
  - `$$ … $$` (bloco)

### Estruturas principais:

- `TokenKind`
- `Token`
- `Lexer` (responsável por pular espaços/comentários e reconhecer padrões)

---

# 🌳 3. AST / IR (`cf/ir`)

A árvore sintática abstrata representa toda a estrutura lógica do programa CF.

### ✔ Tipos (`CfType`)

- Inteiro (4 bytes, big-endian)
- Logico (1 byte: 0xFF/0x00)
- Caractere (1 byte ASCII)
- Desconhecido

### ✔ Expressões

- IntExpr, CharExpr, StringExpr, BoolExpr
- IdentExpr
- BinaryExpr, UnaryExpr, ParenExpr

### ✔ Comandos

- Declaração (`VarDeclStmt`)
- Atribuição (`AssignStmt`)
- `PrintStmt`
- `IfStmt`, `WhileStmt`, `ForStmt`
- `BlockStmt`

---

# 📘 4. Parser (`cf/parser`)

Implementa parsing **LL recursivo descendente**, incluindo precedência de operadores.

## Gramática Simplificada

```
program → stmt_list EOF
stmt_list → stmt stmt_list | ε
stmt → decl_stmt | assign_stmt | print_stmt | if_stmt | while_stmt | for_stmt | block
decl_stmt → type ident_decl_list ";"
ident_decl_list → IDENT "<-" expr | IDENT "<-" expr "," ident_decl_list
assign_stmt → IDENT "<-" expr ";"
print_stmt → "Imprimir" "(" expr ")" ";"
if_stmt → "Se" expr block ("Senao" block)?
while_stmt → "Enquanto" expr block
for_stmt → "Para" IDENT "em" "(" expr "," expr "," expr ")" block
block → "{" stmt_list "}"
expr → or_expr
or_expr → and_expr ("^" and_expr)*
and_expr → cmp_expr ("&" cmp_expr)*
cmp_expr → add_expr (( "=" | "<>" | "<" | "<=" | ">" | ">=" ) add_expr)*
add_expr → mul_expr (("+" | "-") mul_expr)*
mul_expr → pow_expr (("*" | "/" | "%") pow_expr)*
pow_expr → unary ("**" unary)*
unary → primary | "-" unary | "+" unary
primary → INT | CHAR | STRING | IDENT | "Verdade" | "Mentira" | "(" expr ")"
```

---

# 🧠 5. Analisador Semântico (`cf/semantic`)

Realiza validação de:

- declarações
- uso de variáveis
- tipos
- compatibilidade entre operações
- controle de fluxo

### ✔ Tabela de Símbolos

Baseada em pilha (`ScopeStack`).  
Cada `{ ... }` cria um novo escopo.

### ✔ Regras semânticas principais

- variável deve ser declarada antes do uso
- não pode haver dupla declaração no mesmo escopo
- operadores aritméticos aceitam apenas inteiros
- operadores lógicos produzem/consomem lógicos
- condições de `Se`, `Enquanto`, `Para` devem ser lógicas
- `Para` só funciona com inteiros

---

# ⚙️ 6. Otimizador (`cf/opt`)

Aplica:

### ✔ Constant Folding

- `2 + 3 → 5`
- `8 * 0 → 0`

### ✔ Simplificações Algébricas

- `x + 0 → x`
- `x * 1 → x`
- `x * 0 → 0`

---

# 🏗️ 7. Codegen MIPS (`cf/codegen`)

Gera assembly MIPS executável.

### ✔ `VarInfo`

- tipo
- tamanho
- offset na pilha
- uso no frame

### ✔ Estrutura de frame

```
| variáveis |
-------------
| ret addr  |
| old $fp   |
-------------
      $fp →
```

### ✔ Tradução de operações

- aritmética: add, sub, mul, div, rem
- exponenciação: loop ou função auxiliar
- lógicos: bitwise
- comparação: gera 0xFF ou 0x00
- impressão: syscalls 1, 4, 11

---

# 🖥️ 8. Driver / CLI (`cf/driver`)

### Comando principal:

```
./cf arquivo.cf -o saida.asm
```

### Flags:

```
--tokens
--ast
--semantic
--emit-asm
```

---

# 🧪 Testes e Exemplos

Inclui exemplos do enunciado:

- cálculo de primo
- somatório
- expressões complexas
- loops e condicionais

---

# 🛠️ Como Executar

```
cmake -S . -B build
cmake --build build
./build/apps/cf examples/programa.cf > programa.asm
```

Rodar `programa.asm` no MARS/Ripes/SPIM.

---

# 🔍 Troubleshooting

| Erro                   | Causa             | Solução                |
| ---------------------- | ----------------- | ---------------------- |
| Token inesperado       | Erro de sintaxe   | Rever gramática        |
| Variável não declarada | Semântico         | Declarar antes de usar |
| Tipos incompatíveis    | Mistura inválida  | Ajustar expressão      |
| $$ não fechado         | Comentário aberto | Fechar corretamente    |

---

# 🎯 Conclusão

Este documento fornece:

- visão profunda da linguagem CF
- descrição completa da arquitetura
- gramática
- regras semânticas
- modelo de geração de código
- fluxograma do compilador

Pronto para apresentação e publicação no GitHub.
