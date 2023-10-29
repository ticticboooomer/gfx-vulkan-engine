cmake_minimum_required(VERSION 3.26)
SET(CMAKE_C_STANDARD 23)
project(Potentia VERSION 0.0.1.0)

set(SOURCE_FILES "src/main.c")
set(HEADER_FILES)

add_executable(Potentia ${SOURCE_FILES})
