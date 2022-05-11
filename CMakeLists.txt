cmake_minimum_required(VERSION 3.15)
project (dummyscheme)

#set(CMAKE_CXX_STANDARD 11)
#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -w")
#set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")

SET(CMAKE_CXX_FLAGS "-O0 -Wall -g -ggdb")
SET(CMAKE_C_FLAGS "-O0 -Wall -g -ggdb")
#SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O0 -Wall -g -ggdb")	

set(COMMON_SOURCE dummyscheme.cpp tokenize.cpp env.cpp value.cpp util.cpp core.cpp)

set(TARGET_SOURCE main.cpp ${COMMON_SOURCE})
set(TEST_SOURCE testmain.cpp ${COMMON_SOURCE})

add_executable(${PROJECT_NAME} ${TARGET_SOURCE})
add_executable(test${PROJECT_NAME} ${TEST_SOURCE})