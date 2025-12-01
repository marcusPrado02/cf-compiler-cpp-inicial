# 🧩 Módulo Common (`cf/common`) — Documentação Oficial

O módulo **Common** é a fundação do compilador CF.  
Ele reúne **estruturas essenciais**, **modelos de erro**, **posicionamento no código** e **exceções** usadas por _todas_ as demais fases:

- Léxico
- Sintático
- Semântico
- Otimizações
- Codegen
- Driver / CLI

Ao centralizar essas estruturas, garantimos:

- consistência em mensagens de erro
- facilidade de debug
- rastreamento preciso da posição no código
- arquitetura limpa e modular

---

# 📍 1. `Position` — Representação de Posições no Código-Fonte

```cpp
struct Position {
    int line;   // linha (1-based)
    int column; // coluna (1-based)

    std::string to_string() const;
};
```

### ✔ Função

`Position` identifica a localização exata (linha e coluna) de qualquer elemento do código–tokens, expressões, comandos, ou erros. Isso permite mensagens de erro altamente precisas.

Exemplo:

```
Erro: variável não declarada
Linha 12: x <- y + 1
                ^
```

Sem `Position`, essa precisão seria impossível.

---

# 🚨 2. `Diagnostic` — Estrutura Completa de Erro

```cpp
struct Diagnostic {
    Position pos;
    std::string phase;
    std::string message;
    std::string hint;
    std::string line_text;
};
```

### ✔ Descrição dos Campos

| Campo       | Função                                                          |
| ----------- | --------------------------------------------------------------- |
| `pos`       | Onde ocorreu o erro no arquivo                                  |
| `phase`     | Fase do compilador (“lexical”, “syntax”, “semantic”, “codegen”) |
| `message`   | Texto principal do erro                                         |
| `hint`      | Sugestão opcional de solução                                    |
| `line_text` | Linha exata do código-fonte onde ocorreu o problema             |

### ✔ Vantagens

- Segue o padrão de compiladores modernos (Clang, GCC, Rustc)
- Permite gerar mensagens claras e ricas em detalhes
- Ajuda o usuário e facilita depuração

---

# ⚠️ 3. `CompileError` — Exceção Padrão de Erro

```cpp
class CompileError : public std::runtime_error {
    Diagnostic diag_;
public:
    CompileError(const Diagnostic& d);
    const Diagnostic& diag() const;
};
```

### ✔ Papel da `CompileError`

É a exceção responsável por interromper a compilação quando ocorre um erro em qualquer fase.

Usada por:

- Lexer
- Parser
- Semântico
- Codegen

### ✔ Como funciona

- Recebe um `Diagnostic`
- Monta automaticamente uma mensagem final para `.what()`
- Permite inspeção completa via `diag()`

Exemplo de saída final:

```
[semantic:2:12] variável 'y' não foi declarada
Linha 2:    x <- x + y;
                      ^
Dica: declare 'y' antes de usá-la.
```

---

# 🔌 4. Uso do Módulo Common nas Fases do Compilador

## 4.1 Léxico

Tipos de erros detectados:

- caractere inválido
- string não fechada
- comentário não finalizado
- número inválido

Diagnóstico:

```
phase = "lexical"
```

---

## 4.2 Sintático

Erros típicos:

- token inesperado
- blocos não fechados
- parênteses ausentes
- fim de arquivo inesperado

Diagnóstico:

```
phase = "syntax"
```

---

## 4.3 Semântico

Erros como:

- variável não declarada
- tipos incompatíveis
- condição não-lógica em `Se`, `Enquanto`, `Para`

Diagnóstico:

```
phase = "semantic"
```

---

## 4.4 Codegen

Erros possíveis:

- operação ainda não suportada
- tipo inesperado em geração de código
- falhas internas de offset

Diagnóstico:

```
phase = "codegen"
```

---

# 🧠 5. Boas Práticas Incorporadas

### ✔ Centralização

Todas as mensagens e estruturas de erro estão no Common.

### ✔ Consistência

Mesma formatação em todas as fases.

### ✔ Encapsulamento

A lógica complexa de erros fica escondida do resto do compilador.

### ✔ Testabilidade

É fácil testar mensagens de erro geradas pelo compilador.

---

# 🧾 6. Exemplo Completo com `Diagnostic` e `CompileError`

Código CF:

```
Inteiro x <- 10;
x <- x + y;
```

Erro gerado:

```
[semantic:2:12] variável 'y' não foi declarada
Linha 2:    x <- x + y;
                      ^
Dica: declare 'y' antes de usá-la.
```

Toda a estrutura vem automaticamente de `Position` + `Diagnostic` + `CompileError`.

---

# 🎯 Conclusão

O módulo **Common** é o alicerce do compilador CF:

- fornece posicionamento preciso
- define diagnóstico completo
- implementa exceção única e padronizada
- estabelece clareza e qualidade nas mensagens de erro
- suporta todas as demais fases do compilador

Sem ele, o compilador seria inconsistente, frágil e difícil de depurar.
