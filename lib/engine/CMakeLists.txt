project(PotentiaEngine)
set(SOURCE_FILES
        src/main.c
        src/backend/gpu.c
        src/backend/window.c
        src/backend/pipeline.c
        src/backend/util.c
        src/core/error.c)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

find_package(Vulkan REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS})
target_link_libraries(${PROJECT_NAME} Vulkan::Vulkan)

target_link_libraries(${PROJECT_NAME} cglm glfw)

target_include_directories(${PROJECT_NAME} PUBLIC include)