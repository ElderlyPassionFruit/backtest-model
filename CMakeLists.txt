cmake_minimum_required(VERSION 3.10)

project(hft-simulator)

set (CMAKE_CXX_STANDARD 17)

add_definitions(-Wall -Wextra -Wno-unused-result -Wno-sign-compare -Werror -O2 -std=c++17)

set(PROJECT_BACKTEST_DIR ${CMAKE_CURRENT_SOURCE_DIR}/BackTest)
set(PROJECT_APPLICATION_DIR ${CMAKE_CURRENT_SOURCE_DIR}/Application)
set(PROJECT_BIN_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bin)

add_subdirectory(${PROJECT_BACKTEST_DIR})
add_subdirectory(${PROJECT_APPLICATION_DIR})