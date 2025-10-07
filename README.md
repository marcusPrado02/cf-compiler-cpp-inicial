# CF (Compila Fofo) — Compilador em C++ (esqueleto inicial)

Este repositório contém o **setup inicial** de um compilador modular para a linguagem **CF**,
escrito em **C++20** com **CMake**. O objetivo é evoluir a partir deste esqueleto até um
compilador completo: léxico → sintático → semântico → IR/AST → geração de Assembly (RISC‑V)
→ execução em assembler online (ex.: Ripes / simuladores NASM).

## Como construir

```bash
# Requisitos: CMake ≥ 3.20, um compilador C++20 (g++/clang++/MSVC), make/ninja

# Clonar (ou descompactar o zip), entrar na pasta
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build

# Rodar binário CLI
./build/apps/cf -h
```

### Testes
```bash
ctest --test-dir build -V
```

## Estrutura
```
.
├── apps/                # executáveis (CLI)
├── docs/                # gramática, notas de design, decisões
├── include/cf/          # headers públicos
├── src/                 # implementação
│   ├── lexer/           # analisador léxico
│   ├── parser/          # analisador sintático
│   ├── semantic/        # verificação semântica
│   ├── ir/              # AST/IR e visitas
│   ├── codegen/         # geração de código (ex.: RISC-V)
│   └── driver/          # orquestração do pipeline
├── tests/               # testes unitários simples (assert)
└── examples/            # programas CF de exemplo
```

## Próximos passos sugeridos
1. Completar a especificação formal da gramática em `docs/GRAMMAR.md`.
2. Implementar o autômato do léxico com estados e tabela de transições.
3. Implementar parser LL(1) recursivo (ou LR(1) usando um gerador próprio simples).
4. Construir AST e verificador de tipos/escopo.
5. Implementar codegen p/ RISC‑V (ou x86/NASM) e validar em simulador online.
6. Adicionar `doctest` ou GoogleTest como dependência (opcional).

---
**Licença:** MIT
