#pragma once
#include <stdexcept>
#include <string>
#include <string_view>
#include "cf/common/position.hpp"

namespace cf {

struct Diagnostic {
    Position pos{};
    std::string phase{};
    std::string message{};
    std::string hint{};
    std::string line_text{};
};

class CompileError : public std::runtime_error {
public:
    explicit CompileError(const Diagnostic& d);
    const Diagnostic& diag() const noexcept { return diag_; }
private:
    Diagnostic diag_;
};

}
