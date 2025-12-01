#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include "cf/driver/driver.hpp"
#include "cf/common/diagnostic.hpp"
#include "cf/lexer/lexer.hpp"
#include "cf/lexer/token.hpp"
#include "cf/parser/parser.hpp"
#include "cf/parser/ast_dump.hpp"
#include "cf/semantic/semantics.hpp"
#include "cf/opt/optimizer.hpp"

/**
 * Lê todo o conteúdo de um stream para uma string.
 * 
 * - Usa um std::ostringstream para acumular o conteúdo.
 * - Lê todo o conteúdo do stream de entrada usando rdbuf().
 * - Retorna a string resultante.
 *
 * @param in Stream de entrada.
 * @return String com todo o conteúdo lido.
 */
static std::string read_all(std::istream& in){ std::ostringstream ss; ss << in.rdbuf(); return ss.str(); }

/**
 * Mostra a mensagem de uso do programa.
 */
static void usage(){
    std::cout << "CF Compiler (esqueleto)\n"
              << "Uso: cf <arquivo.cf> [opcoes]\n"
              << "Opcoes:\n"
              << "  -h,--help           mostra ajuda\n"
              << "  --dump-tokens       imprime a sequencia de tokens\n"
              << "  --dump-ast          imprime a AST apos o parse\n"
              << "  --check-semantics   executa analise semantica e informa 'OK' se passar\n"
              << "  --emit-asm          gera assembly RISC-V (RV32IM) para stdout\n";
}

int main(int argc, char** argv){
    try{
        /**
         * Primeiro, verifica se há argumentos suficientes.
         */
        if (argc < 2){
            usage();
            return 0;
        }

        /**
         * Lê o arquivo de entrada.
         */
        bool dumpTokens = false, dumpAst = false, checkSem = false, emitAsm = false;
        std::string file;

        /**
         * Processa os argumentos da linha de comando.
         */
        for (int i=1;i<argc;++i){
            std::string a = argv[i];
            if (a == "-h" || a == "--help"){ usage(); return 0; }
            else if (a == "--dump-tokens"){ dumpTokens = true; }
            else if (a == "--dump-ast"){ dumpAst = true; }
            else if (a == "--check-semantics"){ checkSem = true; }
            else if (a == "--emit-asm"){ emitAsm = true; }
            else { file = a; }
        }

        /**
         * Verifica se o arquivo foi fornecido.
         */
        if (file.empty()){ std::cerr << "arquivo de entrada nao informado\n"; return 1; }

        /**
         * Lê o conteúdo do arquivo.
         */
        std::ifstream f(file);
        if (!f){ std::cerr << "Nao foi possivel abrir '" << file << "'\n"; return 1; }
        std::string src = read_all(f);

        /**
         * Se a opção --dump-tokens foi fornecida, cria um lexer e imprime todos os tokens.
         */
        if (dumpTokens){
            cf::Lexer lex(src);
            while (true){
                cf::Token t = lex.next();
                std::cout << "[" << cf::to_string(t.kind) << "] \"" << t.lexeme << "\" @"
                          << t.pos.line << ":" << t.pos.column << "\n";
                if (t.kind == cf::TokenKind::End) break;
            }
            return 0;
        }

        cf::Lexer lex(src);
        cf::Parser p(std::move(lex));
        cf::Program prog = p.parse_program();

        /**
         * Se a opção --dump-ast foi fornecida, cria um parser, analisa o programa e imprime o AST.
         */
        if (dumpAst){
            cf::AstDump{std::cout}.dump(prog);
            return 0;
        }

        /**
         * Se a opção --check-semantics foi fornecida, cria um parser, analisa o programa e verifica a semântica.
         */
        if (checkSem){
            cf::Semantic sem; sem.analyze(prog);
            std::cout << "Semantics: OK\n";
            return 0;
        }

        /**
         * Se a opção --emit-asm foi fornecida, cria um parser, analisa o programa, verifica a semântica e gera assembly.
         */
        if (emitAsm){
            cf::Semantic sem; sem.analyze(prog);
            cf::Codegen cg;
            std::string asmcode = cg.emit(prog);
            std::cout << asmcode;
            return 0;
        }

        /**
         * Otimiza o programa usando o otimizador definido em optimizer.hpp.
         */
        cf::Optimizer opt;
        opt.run(prog);

        /**
         * Caminho normal (pipeline completo até asm — se seu Driver já existia)
         * 
         * - Analisa o programa (semântica); se falhar, lança CompileError.
         * - Gera assembly RISC-V (RV32IM) para o programa.
         * - Imprime o código assembly gerado.
         */
        cf::Semantic sem; sem.analyze(prog);
        cf::Codegen cg;
        std::string asmcode = cg.emit(prog);
        std::cout << asmcode << std::endl;
        return 0;
    } catch (const cf::CompileError& e){
        /**
         * Se ocorrer um erro de compilação, imprime a mensagem de erro detalhada.
         */
        std::cerr << e.what() << std::endl;
        return 2;
    } catch (const std::exception& e){
        /**
         * Para outros erros, imprime a mensagem de erro genérica.
         */
        std::cerr << "erro: " << e.what() << std::endl;
        return 3;
    }
}
