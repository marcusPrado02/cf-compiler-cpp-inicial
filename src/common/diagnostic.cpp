#include "cf/common/diagnostic.hpp"
#include <sstream>

namespace cf {

CompileError::CompileError(const Diagnostic& d)
: std::runtime_error([&]{
    std::ostringstream os;
    os << d.phase << " error at " << d.pos.to_string() << ": " << d.message;
    if (!d.hint.empty()) os << "\nHint: " << d.hint;
    if (!d.line_text.empty()) os << "\n> " << d.line_text;
    return os.str();
}()), diag_(d) {}

}
