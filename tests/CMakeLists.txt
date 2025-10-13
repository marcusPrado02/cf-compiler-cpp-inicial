add_executable(cf_tests smoke_tests.cpp)
target_link_libraries(cf_tests PRIVATE cf)
add_test(NAME smoke COMMAND cf_tests)

add_executable(cf_lexer_tests lexer_tests.cpp)
target_link_libraries(cf_lexer_tests PRIVATE cf)
add_test(NAME lexer COMMAND cf_lexer_tests)

add_executable(cf_parser_tests parser_tests.cpp)
target_link_libraries(cf_parser_tests PRIVATE cf)
add_test(NAME parser COMMAND cf_parser_tests)

add_executable(cf_codegen_tests codegen_tests.cpp)
target_link_libraries(cf_codegen_tests PRIVATE cf)
add_test(NAME codegen COMMAND cf_codegen_tests)
