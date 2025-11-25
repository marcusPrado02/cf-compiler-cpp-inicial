#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

echo "============================================================"
echo "   EXEMPLO IF/ELSE — controle de fluxo"
echo "============================================================"

cmake -S . -B build >/dev/null
cmake --build build >/dev/null

echo
echo ">> Codigo fonte (examples/ex_branching.cf):"
echo "------------------------------------------------------------"
cat examples/ex_branching.cf
echo "------------------------------------------------------------"

echo
echo ">> Execucao:"
echo "------------------------------------------------------------"
./build/apps/cf examples/ex_branching.cf
echo "------------------------------------------------------------"
