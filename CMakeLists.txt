
project(konkretcmpi)

cmake_minimum_required(VERSION 2.6)

find_package(CMPI)

# Set LIB_SUFFIX to 64 on 64bit architectures
if(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(LIB_SUFFIX "")
else(CMAKE_SIZEOF_VOID_P EQUAL 4)
    set(LIB_SUFFIX 64)
endif(CMAKE_SIZEOF_VOID_P EQUAL 4)



add_subdirectory(cmake)
add_subdirectory(src)
