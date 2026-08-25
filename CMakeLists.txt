cmake_minimum_required(VERSION 3.10)

project(vm C CXX)

add_executable(vm src/main.cpp src/vm.cpp src/scmbasic.cpp src/scmmath.cpp src/scmstring.cpp src/scmcompiler.cpp src/scmport.cpp src/scmvector.cpp)
include_directories(src)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
