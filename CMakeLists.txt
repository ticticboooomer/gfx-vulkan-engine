cmake_minimum_required(VERSION 3.26)
SET(CMAKE_C_STANDARD 23)
project(Potentia VERSION 0.0.1.0)
set(SOURCE_FILES "src/main.c" "src/engine/structure.c" "src/engine/config.c")
set(HEADER_FILES "src/engine/structure.h" "src/engine/config.h")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)



