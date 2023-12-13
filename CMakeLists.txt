cmake_minimum_required(VERSION 3.26)
set(CMAKE_C_STANDARD 23)
project(Potentia VERSION 0.0.1.0)
set(NDEBUG ON)
set(SOURCE_FILES "src/main.c" "src/backend/gpu.c")

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(glfw)
target_link_libraries(${PROJECT_NAME} glfw)


target_include_directories(${PROJECT_NAME} PUBLIC include)

