#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

CF_BIN="./build/apps/cf"
DEMO_SRC="examples/demo.cf"

if [ ! -x "$CF_BIN" ]; then
  echo "Binário $CF_BIN não encontrado. Rodando scripts/build.sh..."
  bash scripts/build.sh
fi

if [ ! -f "$DEMO_SRC" ]; then
  echo "Arquivo $DEMO_SRC não encontrado. Crie um exemplo ou ajuste este script."
  exit 1
fi

echo "============================================================"
echo "   DEMO LÉXICO — TOKENS DE $DEMO_SRC"
echo "============================================================"
echo
"$CF_BIN" --dump-tokens "$DEMO_SRC"
