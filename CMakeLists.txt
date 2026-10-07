cmake_minimum_required(VERSION 3.20)
project(basw LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_library(basw_core STATIC
    src/graph.cpp
    src/jaccard.cpp
    src/static_core.cpp
    src/static_clustering.cpp
    src/window_engine.cpp
    src/active_edge_index.cpp
    src/active_window_graph.cpp
    src/clustering_snapshot.cpp
    src/static_window_runner.cpp
    src/event_parser.cpp
    src/static_experiment.cpp
    src/sequential_exact_runner.cpp
    src/exact_clustering_state.cpp
    src/batch_exact_clustering_state.cpp
    src/batch_exact_runner.cpp
    src/batch_experiment.cpp
    src/sequential_experiment.cpp
)
target_include_directories(basw_core PUBLIC include)

if(MSVC)
    target_compile_options(basw_core PRIVATE /W4 /permissive-)
else()
    target_compile_options(basw_core PRIVATE -Wall -Wextra -Wpedantic)
endif()

foreach(program basw_static basw_batch basw_seq)
    add_executable(${program} apps/${program}.cpp)
    target_link_libraries(${program} PRIVATE basw_core)
endforeach()
