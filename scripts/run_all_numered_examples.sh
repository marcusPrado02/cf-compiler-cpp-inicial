#!/usr/bin/env bash
set -e

# Diretório raiz do projeto
ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

EXAMPLES_DIR="$ROOT_DIR/examples"
OUT_DIR="$ROOT_DIR/examples_out"   # pasta para saída dos .asm
CF_BIN="$ROOT_DIR/build/apps/cf"

echo "------------------------------------------------------------"
echo "     GERANDO ARQUIVOS .asm PARA EXEMPLOS NUMERADOS"
echo "     Origem:  $EXAMPLES_DIR"
echo "     Saída:   $OUT_DIR"
echo "------------------------------------------------------------"

# Confere se diretórios existem
if [ ! -d "$EXAMPLES_DIR" ]; then
  echo "ERRO: pasta '$EXAMPLES_DIR' não encontrada."
  exit 1
fi

if [ ! -x "$CF_BIN" ]; then
  echo "ERRO: binário do compilador '$CF_BIN' não encontrado."
  exit 1
fi

mkdir -p "$OUT_DIR"

# Processa apenas arquivos que começam com número
for src in "$EXAMPLES_DIR"/[0-9]*.cf; do
  [ -e "$src" ] || continue

  filename="$(basename "$src")"
  base="${filename%.cf}"
  out_file="$OUT_DIR/$base.asm"

  echo
  echo ">>> Compilando $filename → $base.asm"
  echo "----------------------------------------"

  "$CF_BIN" "$src" > "$out_file"

  echo "OK: gerado $out_file"
done

echo
echo "------------------------------------------------------------"
echo "   FIM — TODOS OS .asm FORAM GERADOS"
echo "------------------------------------------------------------"
