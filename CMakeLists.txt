cmake_minimum_required(VERSION 3.3)
project(biu)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES
    biuc.cpp
    common.h
    parser.cpp
    parser.h
    typechecker.cpp
    typechecker.h
    codegen.cpp
    codegen.h)

add_executable(biu ${SOURCE_FILES})