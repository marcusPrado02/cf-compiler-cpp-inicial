#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

echo "============================================================"
echo "   RODANDO TODOS OS EXEMPLOS DA LINGUAGEM CF"
echo "============================================================"

cmake -S . -B build >/dev/null
cmake --build build >/dev/null

echo
bash scripts/ex_basics.sh

echo
bash scripts/ex_branching.sh

echo
bash scripts/ex_loops.sh

echo
bash scripts/ex_chars_strings.sh

echo
bash scripts/ex_logic.sh

echo
bash scripts/ex_semantic_errors.sh

echo
echo "============================================================"
echo "TODOS OS EXEMPLOS FORAM EXECUTADOS."
echo "============================================================"
