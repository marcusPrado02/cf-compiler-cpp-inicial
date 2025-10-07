#pragma once
#include <string>
#include "cf/ir/ast.hpp"

namespace cf {

class Codegen {
public:
    virtual ~Codegen() = default;
    virtual std::string emit(const Program& p) = 0;
};

// RISC-V RV32I *esqueleto*
class CodegenRV32I : public Codegen {
public:
    std::string emit(const Program& p) override;
};

}
