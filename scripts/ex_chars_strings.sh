#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

echo "============================================================"
echo "   EXEMPLO CHARS + STRINGS"
echo "============================================================"

cmake -S . -B build >/dev/null
cmake --build build >/dev/null

echo
echo ">> Codigo fonte (examples/ex_chars_strings.cf):"
echo "------------------------------------------------------------"
cat examples/ex_chars_strings.cf
echo "------------------------------------------------------------"

echo
echo ">> Execucao:"
echo "------------------------------------------------------------"
./build/apps/cf examples/ex_chars_strings.cf
echo "------------------------------------------------------------"
