add_library(cf
    common/position.cpp
    common/diagnostic.cpp
    lexer/token.cpp
    lexer/lexer.cpp
    ir/ast.cpp
    parser/parser.cpp
    semantic/semantic.cpp
    codegen/codegen.cpp
    driver/driver.cpp
)
target_include_directories(cf PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
