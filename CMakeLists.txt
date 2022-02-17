cmake_minimum_required(VERSION 3.15)
project (dummyscheme)
set(SOURCE main.cpp dummyscheme.cpp)
add_executable(${PROJECT_NAME} ${SOURCE})
