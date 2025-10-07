#include "cf/codegen/codegen.hpp"

namespace cf {

std::string CodegenRV32I::emit(const Program& /*p*/){
    // Esqueleto: produz um assembly mínimo válido
    return R"(.section .text
.globl _start
_start:
  # TODO: gerar código a partir do AST
  li a0, 0
  li a7, 93     # ecall: exit
  ecall
)";
}

}
