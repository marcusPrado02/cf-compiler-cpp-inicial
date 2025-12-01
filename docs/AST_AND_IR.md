# 🌳 AST & IR – Representação Intermediária da Linguagem CF

### (Arquivo: `cf/ir/ast.hpp`)

A **AST (Abstract Syntax Tree)** e a **IR (Intermediate Representation)** formam o núcleo lógico do compilador CF.  
Elas representam **estrutura, tipos e relações** do programa fonte de forma independente da sintaxe textual, servindo como base para:

- ✨ Análise semântica
- ✨ Otimizações
- ✨ Geração de código MIPS
- ✨ Mensagens de erro precisas
- ✨ Visualização e debug (via `AstDump`)

Este documento detalha cada nó, campos, responsabilidades e uso pela pipeline.

---

# 🧱 1. Tipos da Linguagem — `CfType`

```cpp
enum class CfType {
    Inteiro,
    Logico,
    Caractere,
    Desconhecido
};
```

### ✔ Descrição

| Tipo             | Tamanho | Observações                                 |
| ---------------- | ------- | ------------------------------------------- |
| **Inteiro**      | 4 bytes | Big-endian                                  |
| **Logico**       | 1 byte  | 0xFF = Verdade, 0x00 = Mentira              |
| **Caractere**    | 1 byte  | ASCII                                       |
| **Desconhecido** | —       | usado quando o parser ainda não sabe o tipo |

### ✔ Uso

Cada expressão possui um campo `CfType inferred`.  
Esse campo é preenchido pela **análise semântica**, permitindo:

- validação de operadores
- coerção e inferência
- erro semântico preciso

---

# 🧩 2. Classe Base `Expr` — Expressões

Todas as expressões da CF derivam de:

```cpp
struct Expr {
    Position pos;
    CfType inferred = CfType::Desconhecido;
    virtual ~Expr() = default;
};
```

### ✔ Campos

| Campo      | Descrição                              |
| ---------- | -------------------------------------- |
| `pos`      | posição exata no código (linha/coluna) |
| `inferred` | tipo inferido pelo semântico           |

### ✔ Subclasses de Expressões

#### **1. ExprInteger**

Literal inteiro.

```cpp
ExprInteger { int value; }
```

#### **2. ExprChar**

Literal de caractere.

```cpp
ExprChar { char content; }
```

#### **3. ExprString**

Literal string.

```cpp
ExprString { std::string content; }
```

#### **4. ExprBool**

Literal lógico (`Verdade`, `Mentira`).

```cpp
ExprBool { bool value; }
```

#### **5. ExprIdent**

Uso de variável.

```cpp
ExprIdent { std::string name; }
```

Case-insensitive (normalizado no parser/semântico).

#### **6. ExprGroup**

Expressão entre parênteses.

```cpp
ExprGroup { std::unique_ptr<Expr> inner; }
```

#### **7. ExprBinary**

Expressão binária (duas subexpressões + operador).

```cpp
ExprBinary {
    BinOp op;
    std::unique_ptr<Expr> lhs, rhs;
}
```

---

# 🔧 3. Operadores — `BinOp`

Enum completo dos operadores binários suportados pela CF:

```cpp
enum class BinOp {
    Add, Sub, Mul, Div, Mod, Pow,
    Eq, Ne, Gt, Lt, Ge, Le,
    And, Or
};
```

### ✔ Operadores e suas categorias

| Categoria       | Operadores      | Retorno esperado |
| --------------- | --------------- | ---------------- |
| **Aritméticos** | + - \* / % \*\* | Inteiro          |
| **Relacionais** | = <> < <= > >=  | Logico           |
| **Lógicos**     | & ^             | Logico           |

O parser usa `BinOp` como representação semântica de cada token.

---

# 🧱 4. Classe Base `Stmt` — Comandos

Todos os comandos derivam de:

```cpp
struct Stmt {
    Position pos;
    virtual ~Stmt() = default;
};
```

### ✔ Subclasses de Comandos

---

## **1. Declaração — `StmtDecl`**

```cpp
StmtDecl {
    CfType type;
    std::string name;
    std::unique_ptr<Expr> init; // opcional
}
```

Exemplos CF:

```
Inteiro x <- 10;
Caractere c <- 'a';
Inteiro a <- 1, b <- 2;
```

---

## **2. Atribuição — `StmtAssign`**

```cpp
StmtAssign {
    std::string name;
    std::unique_ptr<Expr> value;
}
```

Exemplo:

```
x <- x + 1;
```

---

## **3. Impressão — `StmtPrint`**

```cpp
StmtPrint {
    std::vector<std::unique_ptr<Expr>> args;
}
```

Exemplo:

```
Imprimir("Valor: " + x);
```

---

## **4. Condicional — `StmtIf`**

```cpp
StmtIf {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> then_body;
    std::unique_ptr<Stmt> else_body;
}
```

Exemplo:

```
Se x > 0 { Imprimir("ok"); }
Senao { Imprimir("err"); }
```

---

## **5. Loop Enquanto — `StmtWhile`**

```cpp
StmtWhile {
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Stmt> body;
}
```

---

## **6. Loop Para — `StmtFor`**

```cpp
StmtFor {
    std::string var;
    std::unique_ptr<Expr> begin;
    std::unique_ptr<Expr> end;
    std::unique_ptr<Expr> step;
    std::unique_ptr<Stmt> body;
}
```

Estrutura de CF:

```
Para i em (inicio, fim, passo) { ... }
```

---

## **7. Bloco — `StmtBlock`**

```cpp
StmtBlock {
    std::vector<std::unique_ptr<Stmt>> body;
}
```

Uso:

```
{
   a <- a + 1;
   Imprimir(a);
}
```

---

# 🏛️ 5. Nó Raiz — `Program`

```cpp
struct Program {
    std::vector<std::unique_ptr<Stmt>> items;
};
```

### ✔ Função

- Representa o arquivo CF completo
- É a entrada da análise semântica, otimização e geração de código

---

# 📊 Diagrama ASCII da AST

Para o programa:

```
Inteiro x <- 5;
Imprimir(x + 1);
```

Representação:

```
Program
├── StmtDecl(type=Inteiro, name=x)
│     └── init: ExprInteger(5)
└── StmtPrint
      └── args:
            └── ExprBinary(Add)
                   ├── ExprIdent(x)
                   └── ExprInteger(1)
```

---

# 🔍 Uso da AST no Compilador

| Fase          | Como usa a AST                                            |
| ------------- | --------------------------------------------------------- |
| **Semântico** | Preenche `inferred`, valida operadores, resolve variáveis |
| **Optimizer** | Simplifica nós, reduz árvores, dobra constantes           |
| **Codegen**   | Gera MIPS percorrendo recursivamente                      |
| **AstDump**   | Exibe estrutura para debug                                |

---

# 🧠 Benefícios da AST da CF

- Estrutura limpa e objetiva
- Facilita semântica e otimização
- Navegação recursiva simples
- Preserva posição para erros precisos
- Coerente com linguagens reais (C, Pascal, MiniJava etc.)

---

# 🎯 Conclusão

A AST da CF foi projetada para ser **simples, poderosa e extensível**, servindo perfeitamente como ponte entre a sintaxe textual e o código assembly.

Ela é a fundação de todas as fases seguintes — e torna o compilador modular, robusto e muito mais fácil de manter.
