#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

if [ ! -d "build" ]; then
  echo "Build ainda não existe. Rodando scripts/build.sh..."
  bash scripts/build.sh
fi

cd build

echo "==> Rodando testes com CTest..."
ctest --output-on-failure
