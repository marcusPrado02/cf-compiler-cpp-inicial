#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

echo "============================================================"
echo "   DEMO COMPLETA DO COMPILADOR CF"
echo "   (build + testes + léxico + AST + semântica + codegen)"
echo "============================================================"
echo

# 1) Build
bash scripts/build.sh
echo

# 2) Testes
bash scripts/test.sh || true
echo

# 3) Léxico
bash scripts/demo_lexer.sh
echo

# 4) AST
bash scripts/demo_ast.sh
echo

# 5) Semântica (OK + erro)
bash scripts/demo_semantic.sh
echo

# 6) Codegen
bash scripts/demo_codegen.sh
echo

echo "============================================================"
echo "DEMO COMPLETA FINALIZADA."
echo "Agora você pode abrir o arquivo examples/codegen_demo.s"
echo "no Ripes (ou outro simulador RISC-V) e executar."
echo "============================================================"
