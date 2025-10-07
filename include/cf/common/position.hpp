#pragma once
#include <cstddef>
#include <string>

namespace cf {
struct Position {
    std::size_t line{1};
    std::size_t column{1};
    std::string to_string() const;
};
}
