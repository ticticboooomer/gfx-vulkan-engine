project(PotentiaSample_Triangle)

set(SOURCE_FILES
        src/main.c)

add_executable(${PROJECT_NAME} ${SOURCE_FILES})

target_link_libraries(${PROJECT_NAME} PUBLIC PotentiaEngine)

