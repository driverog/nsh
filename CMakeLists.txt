cmake_minimum_required(VERSION 3.11)
project(nsh C)

set(CMAKE_C_STANDARD 11)

add_executable(nsh main.c builtin.c builtin.h list.c list.h glist.c glist.h)
