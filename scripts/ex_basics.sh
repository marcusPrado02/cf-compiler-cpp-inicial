#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

echo "============================================================"
echo "   EXEMPLO BASICO — declarations, atribuicoes, imprimir"
echo "============================================================"

cmake -S . -B build >/dev/null
cmake --build build >/dev/null

echo
echo ">> Codigo fonte (examples/ex_basics.cf):"
echo "------------------------------------------------------------"
cat examples/ex_basics.cf
echo "------------------------------------------------------------"

echo
echo ">> Execucao (sem flags especiais):"
echo "------------------------------------------------------------"
./build/apps/cf examples/ex_basics.cf
echo "------------------------------------------------------------"
