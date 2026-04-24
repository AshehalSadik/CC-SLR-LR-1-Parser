cmake_minimum_required(VERSION 3.20)
project(CC_SLR_LR_1_Parser)

set(CMAKE_CXX_STANDARD 17)

add_executable(CC_SLR_LR_1_Parser
        src/main.cpp
        src/grammar.cpp
        src/items.cpp
        src/parsing_table.cpp
        src/slr_parser.cpp
        src/tree.cpp)
