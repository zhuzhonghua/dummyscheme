cmake_minimum_required(VERSION 3.15)
project (dummyscheme)
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")	
set(COMMON_SOURCE dummyscheme.cpp tokenize.cpp env.cpp value.cpp util.cpp core.cpp)

set(TARGET_SOURCE main.cpp ${COMMON_SOURCE})
set(TEST_SOURCE testmain.cpp ${COMMON_SOURCE})

add_executable(${PROJECT_NAME} ${TARGET_SOURCE})
add_executable(test${PROJECT_NAME} ${TEST_SOURCE})