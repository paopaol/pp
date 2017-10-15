add_library(errors errors.cpp)

add_executable(errors_test errors_test.cpp)
target_link_libraries(errors_test pp)
add_test(
     errors_package_test_new
     errors_test  "new"
)
set_tests_properties(
    errors_package_test_new
    PROPERTIES
    PASS_REGULAR_EXPRESSION "new error"
)
add_test(
     errors_package_test_get
     errors_test  "get"
)
set_tests_properties(
    errors_package_test_get
    PROPERTIES
    PASS_REGULAR_EXPRESSION "EOF"
)
add_test(
    errors_package_test_eq
    errors_test "eq"
)

set_tests_properties(
    errors_package_test_eq
    PROPERTIES
    PASS_REGULAR_EXPRESSION "1"
)

add_test(
  errors_return_tuple
  errors_test "return_tuple"
  )
