#include "cf/driver/driver.hpp"
#include "cf/lexer/lexer.hpp"
#include "cf/parser/parser.hpp"
#include "cf/semantic/semantics.hpp"
#include "cf/codegen/codegen.hpp"
#include "cf/opt/optimizer.hpp"

namespace cf {

    std::string Driver::compile_to_asm(const std::string &input)
    {
        // 1. LÉXICO
        Lexer lexer(input);

        // 2. PARSER
        Parser parser(lexer);
        Program program = parser.parse_program();  

        // 3. SEMÂNTICO
        Semantic sema;            
        sema.analyze(program);

        // 4. OTIMIZADOR 
        Optimizer opt;
        opt.run(program);
        // faz sentido rodar semântica de novo:
        sema.analyze(program);

        // 5. CODEGEN
        Codegen gen;             
        return gen.emit(program);
    }

} 
