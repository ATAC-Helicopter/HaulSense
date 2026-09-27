cmake_minimum_required(VERSION 3.20)
project(ats_dualsense_daemon LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_executable(ats-dualsense src/main.cpp src/dualsense.cpp
    src/player_leds.cpp)
target_compile_options(ats-dualsense PRIVATE -Wall -Wextra -Wpedantic)
install(TARGETS ats-dualsense RUNTIME DESTINATION bin)
