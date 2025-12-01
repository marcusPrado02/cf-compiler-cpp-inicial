#!/usr/bin/env bash
set -e

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT_DIR"

CF_BIN="./build/apps/cf"
CODEGEN_SRC="examples/codegen_demo.cf"
OUT_ASM="examples/codegen_demo.s"

if [ ! -x "$CF_BIN" ]; then
  echo "Binário $CF_BIN não encontrado. Rodando scripts/build.sh..."
  bash scripts/build.sh
fi

# Exemplo CF mais completo para geração de código
if [ ! -f "$CODEGEN_SRC" ]; then
  cat > "$CODEGEN_SRC" << 'EOF'
Inteiro i;
Inteiro soma;
soma <- 0;
Para i em (1, 5, 1) {
    soma <- soma + i;
}
Imprimir("Soma de 1..5 = ", soma, "\n");
EOF
fi

echo "============================================================"
echo "   DEMO CODEGEN — GERAÇÃO DE ASSEMBLY RISC-V"
echo "============================================================"
echo "Arquivo de entrada: $CODEGEN_SRC"
echo
cat "$CODEGEN_SRC"
echo
echo "Assembly gerado (primeiras linhas):"
"$CF_BIN" --emit-asm "$CODEGEN_SRC" | tee "$OUT_ASM" | head -n 40

echo
echo "Assembly completo salvo em: $OUT_ASM"
echo "Você pode copiar esse arquivo e colar no Ripes ou outro simulador RISC-V."
