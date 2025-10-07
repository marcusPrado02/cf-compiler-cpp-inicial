#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "cf/driver/driver.hpp"
#include "cf/common/diagnostic.hpp"

static std::string read_all(std::istream& in){ std::ostringstream ss; ss << in.rdbuf(); return ss.str(); }

int main(int argc, char** argv){
    try{
        if (argc < 2){
            std::cout << "CF Compiler (esqueleto)\n";
            std::cout << "Uso: cf <arquivo.cf>\n";
            std::cout << "Opcoes: -h (ajuda)\n";
            return 0;
        }
        std::string arg = argv[1];
        if (arg == "-h" || arg == "--help"){
            std::cout << "Uso: cf <arquivo.cf> -> gera assembly RISC-V na saída padrão\n";
            return 0;
        }
        std::ifstream f(arg);
        if (!f){ std::cerr << "Nao foi possivel abrir '" << arg << "'\n"; return 1; }
        std::string src = read_all(f);
        cf::Driver d;
        std::string asmcode = d.compile_to_asm(src);
        std::cout << asmcode << std::endl;
        return 0;
    } catch (const cf::CompileError& e){
        std::cerr << e.what() << std::endl;
        return 2;
    } catch (const std::exception& e){
        std::cerr << "erro: " << e.what() << std::endl;
        return 3;
    }
}
