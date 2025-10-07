# Decisões de Design (inicial)

- **C++20**, CMake, foco em modularidade e testes.
- **Driver** orquestra: Lexer → Parser → Semântico → IR → Codegen.
- **AST + Visitor** para separação das fases.
- **Case-insensitive**: normalizamos lexemas de identificadores para minúsculas no Lexer, mantendo `spelling` original opcional.
- **Erros**: classes de exceção específicas por fase, com localização (linha/coluna), snippet e dica.
- **Codegen**: ponteiro de estratégia para destinos diferentes (RISC-V / NASM). Primeiro alvo: *RISC-V RV32I*.
