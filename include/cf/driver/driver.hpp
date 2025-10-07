#pragma once
#include <string>
#include "cf/codegen/codegen.hpp"

namespace cf {

class Driver {
public:
    std::string compile_to_asm(const std::string& source);
};

}
