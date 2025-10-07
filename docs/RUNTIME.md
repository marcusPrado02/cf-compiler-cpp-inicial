# CF Runtime Contract

## 0. Overview

Este documento define o **contrato de runtime** da linguagem **CF (Compila Fofo)**:  
as convenções de execução, modelo de dados, representação de tipos,  
API do runtime (funções auxiliares como `print_str`, `print_int`, etc.)  
e o contrato que o **gerador de código (Codegen)** deve seguir.

---

## 1. Alvo (Backend)

- **Primário:** RISC-V RV32I (Linux-syscall ABI)
- **Secundário (opcional):** NASM x86-64 (Linux)

### Justificativa

- RISC-V é simples, educacional e compatível com simuladores como **Ripes**.  
- Syscalls padrão Linux (`write`, `exit`) permitem gerar programas executáveis reais.  
- Arquitetura little-endian; respeitamos big-endian apenas logicamente (CF define Inteiro big-endian apenas conceitualmente).

---

## 2. Modelo de Dados da Linguagem CF

| Tipo CF      | Tamanho | Representação | Observações |
|---------------|----------|----------------|--------------|
| **Inteiro**   | 4 bytes | big-endian (conceitual); interno little-endian | usado em aritmética e comparações |
| **Lógico**    | 1 byte  | `0xFF` (Verdade) / `0x00` (Mentira) | deve ser normalizado após cada operação lógica |
| **Caractere** | 1 byte  | ASCII | pode ser promovido para Inteiro em expressões |
| **String**    | variável | sequência ASCII terminada em `\0` | armazenada na seção `.rodata` |

---

## 3. Endianness

- **Dentro dos registradores:** usamos o endianness **nativo** (RISC-V é little-endian).  
- **Na memória interna (pilha):** também little-endian (não há troca de bytes).  
- **Na borda externa (I/O):** CF define Inteiro big-endian *lógico*,  
  mas como imprimimos valores decimais, não há necessidade de conversão binária —  
  a saída é textual.

---

## 4. Boolean Normalization

- Resultados intermediários de operações lógicas podem ser `0`/`1`.
- Antes de armazenar ou imprimir:
  ```asm
  # Normaliza para 0xFF / 0x00
  neg  a0, a0
  andi a0, a0, 0xFF
  ```
- `0xFF` representa **Verdade**, `0x00` representa **Mentira**.

---

## 5. Convenções do Backend RISC-V

| Registrador | Uso |
|--------------|-----|
| `a0`–`a7` | argumentos / retorno |
| `t0`–`t6` | temporários (sem preservação) |
| `sp` | stack pointer |
| `ra` | retorno de funções (`jal`) |

### Syscalls Linux
| Operação | `a7` | `a0` | `a1` | `a2` |
|-----------|-------|------|------|------|
| **write** | 64 | fd | buf | count |
| **exit** | 93 | code | – | – |

---

## 6. Layout de Pilha (Frame principal)

```
sp →  | variável n |  ← offsets negativos
      | variável n-1 |
      | temporários  |
```

- Cada variável CF recebe um **offset fixo** a partir de `sp`.
- A pilha é alinhada a 16 bytes.
- Temporários preferem registradores (`t0..t6`) e só vão para a pilha se necessário.

---

## 7. Pool de Strings

- Literais de string são armazenados na seção `.rodata`.
- Cada literal tem um **rótulo único** (`L.str.N`).
- Strings são terminadas com `\0`.

Exemplo:
```asm
.section .rodata
L.str.0: .asciz "Ola CF!\n"
```

---

## 8. API de Runtime (Funções auxiliares globais)

### print_str
Imprime uma string de tamanho conhecido.

```asm
.globl print_str
print_str:
    # Entrada: a0=addr, a1=len
    li a7, 64         # syscall write
    li a0, 1          # fd = stdout
    mv a1, a0         # (reorganizar se necessário)
    mv a2, a1         # count = len
    ecall
    ret
```

### print_char
Imprime 1 caractere (byte em `a0`).

```asm
.globl print_char
print_char:
    addi sp, sp, -16
    sb a0, 0(sp)
    li a7, 64
    li a0, 1
    addi a1, sp, 0
    li a2, 1
    ecall
    addi sp, sp, 16
    ret
```

### print_int
Imprime um inteiro decimal.

> Implementar: divisão sucessiva por 10, empilhando dígitos e imprimindo em ordem inversa.

### newline
Imprime `\n`.

```asm
.globl newline
newline:
    addi sp, sp, -16
    li t0, 10          # '\n'
    sb t0, 0(sp)
    li a7, 64
    li a0, 1
    addi a1, sp, 0
    li a2, 1
    ecall
    addi sp, sp, 16
    ret
```

---

## 9. Contrato para o Gerador de Código (Codegen)

### Expressões
- Cada expressão deixa o resultado em `a0`.
- Operações binárias podem usar `t0/t1` como registradores temporários.
- Comparações geram `0/1` e depois aplicam normalização p/ `0xFF/0x00`.

### Atribuição
- `ident <- expr`:
  - Avalia `expr` → resultado em `a0`.
  - Faz store no offset do `ident` (`sw`/`sb`).
  - Tipos:
    - Inteiro: 4 bytes (`sw`).
    - Caractere: 1 byte (`sb`).
    - Lógico: 1 byte (`sb` com `andi a0, a0, 0xFF`).

### Controle de fluxo

#### Se / Senao
```asm
    # cond -> a0 (0xFF/0x00)
    beqz a0, .Lelse
    ... bloco_then ...
    j .Lend
.Lelse:
    ... bloco_else ...
.Lend:
```

#### Enquanto
```asm
.Lcond:
    ... avalia cond (resultado em a0)
    beqz a0, .Lend
    ... corpo ...
    j .Lcond
.Lend:
```

#### Para
Açúcar sintático para:

```cf
Para i em (inicio, fim, passo)
{
  ...
}
```

→

```cf
i <- inicio;
Enquanto i <= fim {
  ...
  i <- i + passo;
}
```

---

## 10. Convenção de Impressão (Imprimir)

`Imprimir(arg1, arg2, ...)` gera:

```asm
# Para cada argumento:
# - string:  la a0, L.str.N; li a1, len; jal print_str
# - char:    mv a0, reg; jal print_char
# - int:     mv a0, reg; jal print_int
```

Concatenar strings e expressões deve ser feito na linguagem CF, não no runtime.

---

## 11. Exemplo “Hello Runtime”

```asm
.section .rodata
L.str.0: .asciz "Ola CF!\n"

.section .text
.globl _start
_start:
    # print_str("Ola CF!\n", 8)
    la a0, L.str.0
    li a1, 8
    jal print_str

    # exit(0)
    li a0, 0
    li a7, 93
    ecall
```

---

## 12. Critérios de Aceitação (Checklist)

- [x] Backend primário definido (RISC-V RV32I Linux-syscall)  
- [x] Tipos CF documentados (tamanho, representação, coerção)  
- [x] Endianness e normalização de boolean definidos  
- [x] Convenções de registradores e stack documentadas  
- [x] API de runtime (`print_*`, `newline`) especificada  
- [x] Contrato de Codegen descrito (expressões, atribuições, fluxos)  
- [x] Exemplo `Hello Runtime` incluído

---

## 13. Apêndice — NASM x86-64 (opcional)

### Syscalls Linux x86-64

| Operação | `rax` | `rdi` | `rsi` | `rdx` |
|-----------|--------|-------|-------|-------|
| write | 1 | fd | buf | count |
| exit | 60 | code | – | – |

### print_str (x86-64)
```asm
global print_str
print_str:
    mov rax, 1      ; syscall write
    mov rdi, 1      ; fd = stdout
    syscall
    ret
```

### print_int (x86-64)
> Igual à versão RISC-V, mas usando registradores `rax`, `r10` temporários.

### Convenções equivalentes:
- `rax`: resultado das expressões.
- `r10`, `r11`: temporários.
- `rbp` / `rsp`: frame principal, variáveis locais com offset negativo.

---

## 14. Referências

- [RISC-V Syscall Table (Linux ABI)](https://github.com/riscv/riscv-isa-manual)
- [Ripes Simulator](https://github.com/mortbopet/Ripes)
- [NASM Syscalls Reference](https://filippo.io/linux-syscall-table/)