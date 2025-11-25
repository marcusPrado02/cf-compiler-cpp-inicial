#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

echo "============================================================"
echo "   EXEMPLOS DE ERROS SEMANTICOS"
echo "============================================================"

cmake -S . -B build >/dev/null
cmake --build build >/dev/null

echo
echo ">> Codigo fonte (examples/ex_semantic_errors.cf):"
echo "------------------------------------------------------------"
cat examples/ex_semantic_errors.cf
echo "------------------------------------------------------------"

echo
echo ">> Execucao (espera-se erro semantico):"
echo "------------------------------------------------------------"
set +e
./build/apps/cf --check-semantics examples/ex_semantic_errors.cf
echo "Codigo de saida = $?"
set -e
echo "------------------------------------------------------------"
