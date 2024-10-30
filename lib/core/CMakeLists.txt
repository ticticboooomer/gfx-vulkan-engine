project(PotentiaCore VERSION  0.0.1.0)

set(SOURCE_FILES
        src/handle.c
        src/arrays.c
)

add_library(${PROJECT_NAME} ${SOURCE_FILES})

target_include_directories(${PROJECT_NAME} PUBLIC include)