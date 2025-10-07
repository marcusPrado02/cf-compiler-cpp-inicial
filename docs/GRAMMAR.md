\
# Linguagem CF — Rascunho de Gramática

> Esta gramática inicial é um *rascunho* para guiar a implementação. Será refinada junto com os testes.
> Palavras‑chave e operadores conforme o enunciado do trabalho.

## Tokens (léxico)
- Tipos: `Inteiro`, `Logico`, `Caractere`
- Literais: inteiros decimais (`\d+`), caracteres `'a'` (ASCII), strings `"..."`
- Booleanos: `Verdade`, `Mentira`
- Identificadores: **somente letras**, sem distinção de caixa (case‑insensitive)
- Atribuição: `<-`
- Pontuação: `;`, `{`, `}`
- Operadores aritméticos: `+ - / % * **`
- Operadores lógicos/relacionais: `= <> > < <= >= & ^`
- Comentários: `$` (linha) e `$$ … $$` (multi‑linha)

## Comentários
- Linha: tudo após `$` até o fim da linha é ignorado.
- Bloco: `$$` abre e `$$` fecha; **não** aninhado.

## Esboço (LL-ish)
```
Programa      -> DeclLista
DeclLista     -> Decl DeclLista | ε
Decl          -> DeclVar | Cmd
DeclVar       -> Tipo VarDeclLista ';'
Tipo          -> 'Inteiro' | 'Logico' | 'Caractere'
VarDeclLista  -> VarInit (',' VarInit)*
VarInit       -> ident ('<-' Expr)?

Cmd           -> Bloco
              | 'Enquanto' Expr Bloco
              | 'Se' Expr Bloco 'Senao' Bloco
              | 'Para' ident 'em' '(' Expr ',' Expr (',' Expr)? ')' Bloco
              | 'Imprimir' '(' PrintArgs? ')' ';'
              | Atrib ';'

Bloco         -> '{' DeclOuCmdLista? '}'
DeclOuCmdLista-> (DeclVar | Cmd) DeclOuCmdLista | ε

Atrib         -> ident '<-' Expr

PrintArgs     -> PrintArg (',' PrintArg)*
PrintArg      -> Expr | string

Expr          -> OrExpr
OrExpr        -> AndExpr ('^' AndExpr)*
AndExpr       -> RelExpr ('&' RelExpr)*
RelExpr       -> AddExpr (RelOp AddExpr)?
RelOp         -> '=' | '<>' | '>' | '<' | '<=' | '>='
AddExpr       -> MulExpr (('+'|'-') MulExpr)*
MulExpr       -> PowExpr (('*'|'/'|'%') PowExpr)*
PowExpr       -> Unary ('**' Unary)*
Unary         -> ('+'|'-')? Primary
Primary       -> '(' Expr ')' | ident | int | char | BoolLit
BoolLit       -> 'Verdade' | 'Mentira'
```
Notas:
- Precedência: `**` > `* / %` > `+ -` > relacionais > `&` > `^`.
- Associatividade: `**` à direita; demais à esquerda.
- `ident` é *case‑insensitive* e contém somente letras.

## Regras semânticas (esboço)
- Declaração antes de uso; escopos por `{ ... }`.
- Tipagem forte entre `Inteiro`, `Logico`, `Caractere`.
- Operações aritméticas exigem `Inteiro` (com promoção de `Caractere` opcional).
- Comparações retornam `Logico`.
- `Imprimir` aceita `string` e expressões; concatenação via `+` entre `string` e não‑strings exige conversão implícita.

## Código de máquina/Assembly
- Alvo sugerido: **RISC‑V RV32I**, big‑endian lógico na serialização (vide requisitos CF para Inteiro/Caractere/Lógico).
- Booleans: `0xFF` (Verdade), `0x00` (Mentira).
