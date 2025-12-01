#include <cassert>
#include <string>
#include "cf/driver/driver.hpp"

int main(){
    const std::string src = R"(Inteiro i <- 1;
Imprimir("ok");
)";
    cf::Driver d;
    auto out = d.compile_to_asm(src);
    assert(out.find("_start") != std::string::npos);
    return 0;
}
