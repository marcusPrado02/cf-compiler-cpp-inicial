# ✅ README.md — Compilador CF (Compila Fofo)

### 🧱 Projeto acadêmico completo — Lexer → Parser → Semântico → Codegen MIPS

Este repositório contém a **implementação completa** do compilador da linguagem **CF — Compila Fofo**, escrito em **C++20**, com arquitetura modular, documentação detalhada e geração final de **Assembly MIPS**, compatível com:

- **MARS**
- **SPIM**
- **Ripes**
- **CPulator (MIPS)**
- Qualquer montador MIPS educacional

O compilador implementa **todas as fases clássicas**:

```
Código-fonte CF
   ↓
Léxico (Tokens)
   ↓
Parser (AST)
   ↓
Semântico (tipos/escopos)
   ↓
Codegen MIPS (.data + .text)
   ↓
Execução em simulador MIPS
```

---

# 🚀 Como compilar o projeto

### Pré-requisitos

- **Linux/macOS/WSL**
- **CMake ≥ 3.20**
- **g++ ou clang++ com suporte a C++20**

### 💻 Compilando

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

### 🆘 Rodando o compilador

```bash
./build/apps/cf -h
```

---

# ▶️ Como compilar um programa CF para Assembly MIPS

### 📌 Modo manual (recomendado para iniciantes)

```bash
./build/apps/cf examples/demo.cf > demo.asm
```

Agora abra `demo.asm` em MARS/SPIM ou rode:

```bash
spim -file demo.asm
```

---

# ⚡ Script automático para gerar `.asm` de **todos** os exemplos

O repositório contém scripts em `scripts/`.

Exemplo:

```bash
./scripts/generate_all_examples.sh
```

Cada arquivo `.cf` da pasta **examples/** gera automaticamente o correspondente `.asm`.

---

# 📂 Estrutura do Projeto

```
.
├── apps/                  # CLI do compilador (binário "cf")
│
├── include/cf/            # Headers do compilador
│   ├── lexer/             # Tokens, posição, lexer
│   ├── parser/            # AST + parser recursivo LL
│   ├── semantic/          # Tabela de símbolos, tipos, verificação
│   ├── ir/                # Nós da AST e visitors
│   └── codegen/           # Backend MIPS
│
├── src/
│   ├── lexer/             # Implementação do analisador léxico
│   ├── parser/            # Implementação do parser
│   ├── semantic/          # Analisador semântico completo
│   ├── ir/                # AST
│   ├── codegen/           # Emissão de assembly MIPS
│   └── driver/            # Orquestra pipeline → emit.asm
│
├── examples/              # Programas CF prontos (IF, WHILE, PARA, ...)
│
├── docs/
│   ├── LEXER_UPDATED.md
│   ├── PARSER_UPDATED.md
│   ├── SEMANTIC_UPDATED.md
│   └── CODEGEN_MIPS_UPDATED.md
│
├── tests/                 # Testes (podem ser integrados via ctest)
│
└── scripts/               # Scripts utilitários
```

---

# 📘 Especificação da Linguagem CF (resumo)

### ✔️ Tipos

- **Inteiro** (4 bytes)
- **Logico** (1 byte → Verdade=0xFF, Mentira=0x00)
- **Caractere** (1 byte ASCII)

### ✔️ Estruturas

- **Enquanto**
- **Se / Senao**
- **Para i em (inicio, fim, passo)**
- **Imprimir(...)**

### ✔️ Operadores

- Aritméticos: `+ - * / % **`
- Relacionais: `= <> < > <= >=`
- Lógicos: `& ^`
- Atribuição: `<-`

### ✔️ Identificadores

Somente letras, case-insensitive  
`idade`, `Idade`, `IDADE` → **mesma variável**

### ✔️ Comentários

- linha: `$ ...`
- bloco: `$$ ... $$`

---

# 🧠 Componentes do Compilador

As documentações detalhadas estão em **/docs**:

- **Lexer** → tabela de tokens + palavras-chave + suporte UTF-8 básico
- **Parser LL(1)** → AST completa, unário `-` convertido para `0 - expr`
- **Semântico** → escopos aninhados, promoção Char→Int, concatenação de strings
- **Codegen MIPS** → geração de `.data` + `.text`, booleanos 0xFF/0x00, laços, if/else

---

# 🔧 Execução em Simuladores MIPS

O assembly gerado é compatível com:

- **MARS** → recomendado
- **SPIM**
- **Ripes (modo MIPS)**
- **CPulator** (modo MIPS User Mode)

Exemplo no MARS:

```
Assemble → Run
```

Exemplo via CLI do SPIM:

```bash
spim -file demo.asm
```

---

# 🧪 Testes

Rodar testes:

```bash
ctest --test-dir build -V
```

---

# 🎓 Exemplos CF

Todos no diretório `examples/`.

Exemplo:

```cf
Inteiro s <- 0;

Enquanto s < 10
{
    Imprimir("s = " + s + "\n");
    s <- s + 1;
}
```

Compile:

```bash
./build/apps/cf examples/loop.cf > loop.asm
```

---
