cmake_minimum_required(VERSION 3.16)
project(trtle VERSION 0.1 LANGUAGES C)

add_library(trtle)

add_subdirectory(src)
target_include_directories(trtle PRIVATE src)
target_include_directories(trtle INTERFACE include)
