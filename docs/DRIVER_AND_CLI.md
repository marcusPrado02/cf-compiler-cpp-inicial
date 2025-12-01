# 🚀 Driver e Interface de Linha de Comando (`cf/driver` e `apps/cf`)

### Documentação Oficial e Ampliada

O módulo **Driver** e o executável **`cf`** formam a “camada externa” do compilador CF (Compila Fofo).  
Eles são responsáveis por:

- orquestrar todas as fases do compilador,
- oferecer ferramentas de depuração (tokens, AST, semântica),
- gerar o assembly final,
- interagir com o usuário através da linha de comando.

---

# 🧠 1. O Papel do Driver

A função de nível mais alto:

```cpp
std::string compile_to_asm(const std::string& source);
```

Executa a pipeline:

```
Fonte → Lexer → Tokens
      → Parser → AST
      → Semântico → AST anotada
      → Otimizador → AST otimizada
      → Codegen → Assembly MIPS
```

É o ponto central da compilação.

---

# 🔧 2. Documentação Completa do Driver

## ✔ `compile_to_asm(const std::string& source)`

Etapas executadas:

### **1. Léxico**

Criação do `Lexer` e tokenização.

### **2. Parser**

Gera a AST com `parse_program`.

### **3. Análise Semântica**

Resolve escopos, tipos e regras da linguagem.

### **4. Otimizador**

Simplifica expressões e dobra constantes.

### **5. Segunda verificação (opcional)**

Assegura integridade pós-otimização.

### **6. Codegen**

Gera assembly MIPS estruturado.

Retorno → string contendo código `.asm`.

---

# 🖥️ 3. Aplicativo CLI `cf`

Local: `apps/apps_main.cpp`

Fornece interface semelhante a compiladores reais (GCC/Clang/Rustc).

---

# 📚 4. Componentes do CLI

## ✔ `read_all(std::istream& in)`

Lê um arquivo `.cf` inteiro em uma string.

## ✔ `usage()`

Mostra instruções:

```
Uso: cf [opções] arquivo.cf
Opções:
  --dump-tokens
  --dump-ast
  --check-semantics
  --emit-asm
```

## ✔ `int main(int argc, char** argv)`

Fluxo completo:

### **1. Leitura dos argumentos**

Detecta flags e caminho do arquivo.

### **2. Processamento conforme a flag**

- `--dump-tokens` → apenas lexer
- `--dump-ast` → lexer + parser
- `--check-semantics` → lexer + parser + semântico
- `--emit-asm` → pipeline completa

### **3. Sem flags**

Equivalente a `--emit-asm`.

### **4. Tratamento de erros**

Captura:

- `CompileError` → erro do compilador (retorno 2)
- `std::exception` → erro interno (retorno 3)

### **5. Códigos de retorno**

| Código | Uso                |
| ------ | ------------------ |
| `0`    | sucesso            |
| `1`    | erro de uso        |
| `2`    | erro de compilação |
| `3`    | erro interno       |

---

# 🧪 5. Exemplos de Uso

### Compilar para assembly

```
cf programa.cf > out.asm
```

### Dump de tokens

```
cf --dump-tokens programa.cf
```

### AST

```
cf --dump-ast programa.cf
```

### Checar semântica

```
cf --check-semantics programa.cf
```

---

# 🎯 Conclusão

O módulo Driver + CLI:

- orquestra todo o compilador
- fornece utilidades avançadas de debug
- permite inspeção de cada fase
- gera assembly final
- segue padrões profissionais de compiladores modernos

Documentação ideal para apresentação, estudo e manutenção.
