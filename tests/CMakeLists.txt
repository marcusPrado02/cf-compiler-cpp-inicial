add_executable(cf_tests smoke_tests.cpp)
target_link_libraries(cf_tests PRIVATE cf)
add_test(NAME smoke COMMAND cf_tests)
