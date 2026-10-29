cmake_minimum_required(VERSION 3.16)
project(MiniRedis VERSION 1.0 LANGUAGES CXX)

# Tell Apple Clang to use modern C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Turn on helpful compiler warnings to catch bugs early
add_compile_options(-Wall -Wextra)

# Tell the compiler that .hpp menu files live in the include/ folder
include_directories(include)

# Tell CMake to compile src/main.cpp into an app named mini-redis-server
add_executable(mini-redis-server
    src/main.cpp
)