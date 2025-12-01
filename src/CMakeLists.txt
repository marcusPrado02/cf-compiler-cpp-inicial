add_library(cf-core
    common/position.cpp
    common/diagnostic.cpp
    lexer/token.cpp
    lexer/lexer.cpp
    ir/ast.cpp
    parser/parser.cpp
    semantic/semantics.cpp
    codegen/codegen.cpp
    driver/driver.cpp
    opt/optimizer.cpp
)

target_include_directories(cf-core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/../include)
