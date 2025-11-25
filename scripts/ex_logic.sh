#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

echo "============================================================"
echo "   EXEMPLO LOGICO — & e |, comparacoes"
echo "============================================================"

cmake -S . -B build >/dev/null
cmake --build build >/dev/null

echo
echo ">> Codigo fonte (examples/ex_logic.cf):"
echo "------------------------------------------------------------"
cat examples/ex_logic.cf
echo "------------------------------------------------------------"

echo
echo ">> Execucao:"
echo "------------------------------------------------------------"
./build/apps/cf examples/ex_logic.cf
echo "------------------------------------------------------------"
