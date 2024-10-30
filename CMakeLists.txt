cmake_minimum_required(VERSION 3.26)
set(CMAKE_C_STANDARD 23)
project(Potentia VERSION 0.0.1.0)
set(NDEBUG ON)

# 3rd Party Deps
add_subdirectory(3rdparty)

# Engine & Libs
add_subdirectory(lib)

#Samples
add_subdirectory(samples)

# Tools
add_subdirectory(tools)



