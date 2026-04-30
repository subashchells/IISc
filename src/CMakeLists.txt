cmake_minimum_required(VERSION 3.20)
project(CardioScheduler CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
add_compile_options(-Wall -Wextra)

set(CARDIO_SCHEDULER_SOURCES
    data_structures/graph.cpp
    data/loader.cpp
    scheduler/topological.cpp
    scheduler/allocator.cpp
    scheduler/strategies.cpp
    scheduler/hungarian.cpp
    scheduler/policies.cpp
)

add_executable(cardio_scheduler
    main.cpp
    bench.cpp
    bench_adversarial.cpp
    bench_urgency.cpp
    ${CARDIO_SCHEDULER_SOURCES}
)

# src/ for "data/loader.h" style includes; third_party/ for nlohmann/json.hpp
target_include_directories(cardio_scheduler PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
)
