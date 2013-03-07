
project(konkretcmpi)

cmake_minimum_required(VERSION 2.6)

set(CMAKE_C_FLAGS "-std=c99 -Wall -pedantic -g -Wextra -Wno-unused-parameter -Wformat -Wparentheses -Wl,--no-undefined ${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "-std=c++98 -Wall -pedantic -Wno-long-long -g -Wextra -Wno-unused-parameter -Wformat -Wparentheses -Wl,--no-undefined ${CMAKE_CXX_FLAGS}")

find_package(CMPI)

# Set LIB_SUFFIX to 64 on 64bit architectures
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(LIB_SUFFIX "")
else(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(LIB_SUFFIX 64)
endif(CMAKE_SIZEOF_VOID_P EQUAL 4)

option(WITH_PYTHON "Build experimental Python bindings" OFF)

add_subdirectory(cmake)
add_subdirectory(src)
