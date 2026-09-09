cmake_minimum_required(VERSION 3.10)

project(scheme C CXX)

set(SOURCES main.cpp vm.cpp scmbasic.cpp scmmath.cpp scmstring.cpp scmcompiler.cpp scmport.cpp scmvector.cpp)
add_executable(vm ${SOURCES})
add_executable(vmr4 ${SOURCES})
target_compile_definitions(vmr4 PRIVATE SCHEME_STD_R4RS=1)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0")

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 98)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
