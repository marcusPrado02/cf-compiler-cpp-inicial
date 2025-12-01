#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

CF_BIN="./build/apps/cf"
OPT_SRC="examples/opt_demo.cf"

if [ ! -x "$CF_BIN" ]; then
  echo "Binário $CF_BIN não encontrado. Rodando scripts/build.sh..."
  bash scripts/build.sh
fi

# Exemplo com expressões boas para constant folding e simplificações
if [ ! -f "$OPT_SRC" ]; then
  cat > "$OPT_SRC" << 'EOF'
Inteiro x;
Inteiro y;
x <- 2 + 3 * 4;         $ deve virar 14
y <- (10 - 5) * (2 + 3); $ deve virar 25
Imprimir("x = ", x, ", y = ", y, "\n");
EOF
fi

echo "============================================================"
echo "   DEMO OTIMIZAÇÃO — CONSTANT FOLDING"
echo "============================================================"
echo "Arquivo: $OPT_SRC"
echo
cat "$OPT_SRC"
echo
echo "(Nesta demo, o otimizador roda antes da geração de código,"
echo " dobrando constantes e simplificando as expressões aritméticas)."
echo
echo "Você pode combinar este arquivo com --dump-ast ou --emit-asm:"
echo "  ./build/apps/cf --dump-ast $OPT_SRC"
echo "  ./build/apps/cf --emit-asm $OPT_SRC | head -n 40"
