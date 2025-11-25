#!/usr/bin/env bash
set -euo pipefail

# Raiz do projeto
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
CF_BIN="$ROOT/build/apps/cf"
RUNTIME="$ROOT/ripes_runtime.s"

if [ ! -x "$CF_BIN" ]; then
  echo "Erro: binário do compilador não encontrado em $CF_BIN."
  echo "Rode primeiro: scripts/build.sh"
  exit 1
fi

if [ $# -lt 1 ]; then
  echo "Uso: $0 arquivo.cf [saida.s]" >&2
  exit 1
fi

SRC="$1"
OUT="${2:-ripes_out.s}"

TMP="$(mktemp)"

# 1) Gera o assembly "normal" do compilador
"$CF_BIN" "$SRC" > "$TMP"

# 2) Concatena o runtime e adapta o final (ecall -> loop .Lhalt)
{
  # Runtime com print_str/print_int e .Lhalt
  cat "$RUNTIME"
  echo
  # Código gerado pelo compilador, mas com ecall substituído
  sed -e 's/^[[:space:]]*ecall/  j .Lhalt   # ecall substituido para Ripes/' "$TMP"
} > "$OUT"

rm "$TMP"

echo "Assembly adaptado para Ripes salvo em: $OUT"
echo "Abra esse arquivo no Ripes (File -> Open assembly) e rode."
echo "Depois, veja na memória:"
echo "  - outbuf  : bytes das strings impressas"
echo "  - intbuf  : inteiros impressos via print_int"
