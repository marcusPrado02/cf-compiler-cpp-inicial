#!/usr/bin/env bash
set -e

# Sempre começa na raiz do projeto
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

echo "==> Limpando build antigo..."
rm -rf build

echo "==> Configurando CMake..."
cmake -S . -B build

echo "==> Compilando projeto..."
cmake --build build

echo "==> Build concluído com sucesso."
echo "Binário do compilador: build/apps/cf"
