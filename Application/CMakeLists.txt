add_executable(hft-simulator main.cpp)
add_executable(unit-tests unit_tests.cpp)

target_link_libraries(unit-tests backtest)

set_target_properties(hft-simulator unit-tests PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BIN_DIR})