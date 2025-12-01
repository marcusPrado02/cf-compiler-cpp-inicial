#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

CF_BIN="./build/apps/cf"
OK_SRC="examples/sem_ok.cf"
ERR_SRC="examples/sem_err.cf"

if [ ! -x "$CF_BIN" ]; then
  echo "Binário $CF_BIN não encontrado. Rodando scripts/build.sh..."
  bash scripts/build.sh
fi

# Cria exemplos de semântica (OK e erro) se não existirem
if [ ! -f "$OK_SRC" ]; then
  cat > "$OK_SRC" << 'EOF'
Inteiro x;
x <- 1 + 2;
Imprimir("x = ", x, "\n");
EOF
fi

if [ ! -f "$ERR_SRC" ]; then
  cat > "$ERR_SRC" << 'EOF'
Inteiro x;
y <- x + 1;
EOF
fi

echo "============================================================"
echo "   DEMO SEMÂNTICA — CÓDIGO VÁLIDO"
echo "============================================================"
echo "Arquivo: $OK_SRC"
echo
cat "$OK_SRC"
echo
echo "Saída do analisador semântico:"
"$CF_BIN" --check-semantics "$OK_SRC"
echo
echo

echo "============================================================"
echo "   DEMO SEMÂNTICA — CÓDIGO COM ERRO"
echo "============================================================"
echo "Arquivo: $ERR_SRC"
echo
cat "$ERR_SRC"
echo
echo "Saída do analisador semântico:"
set +e
"$CF_BIN" --check-semantics "$ERR_SRC"
RET=$?
set -e
echo
echo "Código de saída do compilador (esperado != 0): $RET"
