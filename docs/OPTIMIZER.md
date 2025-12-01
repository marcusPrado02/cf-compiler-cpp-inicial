# ⚡ Otimizador da AST (`cf/opt`)

O módulo **Optimizer** é responsável por melhorar a AST antes da geração de código MIPS, aplicando transformações seguras que simplificam expressões, reduzem instruções redundantes e produzem um código final muito mais eficiente.  
Ele faz parte das fases intermediárias do compilador CF e segue princípios usados em compiladores reais como GCC, LLVM, Rustc e Clang.

Esta documentação aprofunda completa e detalhadamente o funcionamento interno do otimizador, suas técnicas, racional arquitetural e exemplos práticos.

---

# 🧭 1. O papel do Otimizador no Pipeline do Compilador CF

Pipeline completo do compilador:

```
Código → Lexer → Tokens
       → Parser → AST Inicial
       → Semântico → AST Validada
       → [Optimizer] → AST Otimizada
       → Codegen → Assembly MIPS
```

O otimizador atua **após a análise semântica**, garantindo:

- tipos já verificados
- expressões com operadores válidos
- ausência de erros sintáticos ou semânticos
- AST completa e coerente

Isso significa que o otimizador pode aplicar transformações agressivas sem risco de alterar a semântica original.

---

# 🧩 2. Arquitetura Geral do Módulo

A interface pública é:

```cpp
class Optimizer {
public:
    void run(Program& p);
};
```

### ✔ O que `run()` faz?

- Percorre todos os statements do programa.
- Visita recursivamente cada expressão.
- Aplica otimizações **in-place** (modifica a própria AST).
- Simplifica nós redundantes.
- Retorna a AST mais eficiente possível sem alterar comportamento.

Essa abordagem é padrão em transformações AST de compiladores reais.

---

# 🔍 3. Tipos de Otimizações Aplicadas

O módulo executa otimizações **locais**, focadas em expressões:

---

## 🔥 3.1 Constant Folding (Dobramento de Constantes)

Se uma expressão pode ser computada em tempo de compilação, ela é substituída por um literal.

### Exemplos:

| Expressão     | Resultado |
| ------------- | --------- |
| `2 + 3`       | `5`       |
| `10 * 0`      | `0`       |
| `1 + (2 + 3)` | `6`       |
| `'A' + 2`     | `67`      |
| `4 ** 1`      | `4`       |

Vantagens:

- reduz instruções geradas pelo codegen
- simplifica a AST substancialmente
- melhora eficiência no simulador MIPS

---

## ⚙️ 3.2 Simplificações Algébricas

Regras derivadas da matemática dos operadores:

| Regra        | Efeito                      |
| ------------ | --------------------------- |
| `x + 0 → x`  | elimina operação redundante |
| `0 + x → x`  | simplifica estrutura        |
| `x - 0 → x`  | idem                        |
| `x * 1 → x`  | menor custo no MIPS         |
| `1 * x → x`  | elimina multiplicação       |
| `x * 0 → 0`  | reduz todo o ramo           |
| `x / 1 → x`  | remove divisão              |
| `x ** 1 → x` | elimina expoente            |
| `x ** 0 → 1` | regra aritmética            |

Essas otimizações seguem exatamente os padrões usados por GCC e LLVM nos estágios AST/IR.

---

## 🧺 3.3 Remoção de Agrupamentos Inúteis

Se o node `ExprGroup` não altera mais a precedência da expressão, ele é removido.

Exemplos:

```
((5))        → 5
(3) + (4)    → 3 + 4
(2 * (3+4))  → 2 * 7  (mantém apenas onde necessário)
```

---

# 🧩 4. Estruturas Internas e Fluxo do Otimizador

## ✔ 4.1 `optimize_expr(ExprPtr e)`

Função recursiva principal:

1. identifica tipo do nó
2. chama otimizações específicas
3. retorna o nó mais simples possível

Padrão clássico de visitação:

- se `e` é literal → retorna
- se é `Group` → otimiza interno
- se é `Binary` → otimiza ambos os lados + aplica regras
- outros tipos mantêm estrutura original

---

## ✔ 4.2 `optimize_group(ExprGroup* grp)`

A função:

- otimiza a expressão interna
- decide se o agrupamento ainda é necessário
- retorna a expressão interna se possível

Racional:

- parênteses só existem para preservar precedência
- após simplificação, podem ser descartáveis

---

## ✔ 4.3 `optimize_binary(ExprBinary* bin)`

Esta é a parte mais sofisticada:

1. otimiza `lhs` e `rhs`
2. detecta se são literais (via `int_literal` ou `int_like_literal`)
3. tenta constant folding
4. se falhar, tenta simplificações algébricas
5. devolve melhor nó possível

---

# 🔬 5. Helpers de Literais

### ✔ `int_literal(const Expr* e, int& value)`

Retorna verdadeiro se:

- `e` é `ExprInteger`
- extrai o valor em `value`

### ✔ `int_like_literal(const Expr* e, int& value)`

Além de inteiros, trata:

- `ExprChar` → converte para ASCII
- possibilita otimizar expressões como `'A' + 3`

Essa abordagem é equivalente ao conceito de “integral promotion” em C.

---

# 🧪 6. Exemplo Completo (Antes → Depois)

### Código CF:

```
Inteiro x <- 1 + 2 * (3 + 4);
```

### AST original:

```
      (+)
     /      (1)   (*)
         /        (2)   (+)
             /             (3) (4)
```

### Otimização passo a passo:

1. `(3 + 4)` → `7`
2. `2 * 7` → `14`
3. `1 + 14` → `15`

### AST final:

```
(15)
```

### Código MIPS gerado:

```asm
li $a0, 15
sw $a0, -4($fp)
```

Simplificação máxima. Nenhum cálculo em tempo de execução.

---

# 📉 7. Impacto Direto no Codegen

Sem otimização:

- várias operações aritméticas
- mais _push/pop_ na pilha
- mais instruções no simulador

Com otimização:

- codegen extremamente simples
- programa executa mais rápido
- menor risco de erros
- assembly mais legível

---

# 🔐 8. Segurança das Otimizações

A análise semântica garante:

- tipos corretos
- operadores válidos
- variáveis declaradas
- nenhuma divisão por zero
- coerência da árvore

Isso permite ao otimizador:

- aplicar regras matemáticas sem ambiguidade
- assumir integridade das expressões
- manter 100% da semântica original

O otimizador **nunca altera o comportamento** do programa CF.

---

# 🚫 9. O que NÃO está implementado (mas pode ser no futuro)

O otimizador atual é propositalmente simples.  
Estes são caminhos naturais de evolução:

- Dead Code Elimination (DCE)
- Propagação global de constantes
- Reordenação de instruções
- Otimização de loops (strength reduction)
- Construção de CFG e SSA
- Inline expansion de expressões
- Copy propagation

Essas técnicas são típicas de compiladores de médio/grande porte e dependem de uma IR mais sofisticada.

---

# 🎯 10. Conclusão

O otimizador AST do compilador CF:

- é seguro
- é determinístico
- reduz complexidade
- melhora performance
- simplifica codegen
- segue padrões de compilação reais
- permite evolução futura do compilador

Apesar de simples, ele representa um componente essencial na cadeia de execução, garantindo que a linguagem CF produza assembly eficiente e limpo.

---

Se quiser, posso gerar também:

- versão PDF
- diagramas Mermaid (antes/depois das árvores)
- documentação do Parser e Semântico em mesmo nível.
