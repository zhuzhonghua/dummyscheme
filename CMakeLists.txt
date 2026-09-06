cmake_minimum_required(VERSION 3.10)

project(vm C CXX)

add_executable(vm main.cpp vm.cpp scmbasic.cpp scmmath.cpp scmstring.cpp scmcompiler.cpp scmport.cpp scmvector.cpp)
include_directories(.)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
