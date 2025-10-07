#include "cf/common/position.hpp"
#include <sstream>
namespace cf {
std::string Position::to_string() const {
    std::ostringstream os; os << line << ":" << column; return os.str();
}
}
