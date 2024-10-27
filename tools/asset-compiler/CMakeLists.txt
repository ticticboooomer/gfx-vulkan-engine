cmake_minimum_required(VERSION 3.26)
project(PotentiaAssetCompiler VERSION 0.0.1.0)

set(SOURCE_FILES
        src/main.c
        src/assets/mesh.c
        src/core/handle.c)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} assimp)

target_include_directories(${PROJECT_NAME} PUBLIC include)