add_subdirectory(c99-3d-math)
add_subdirectory(emuGLES2)

project(cBoxWorld C)
set(CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

set(CMAKE_C_FLAGS "-std=c99")

find_package(GLFW3)
find_package(PNG)

cmake_minimum_required(VERSION 2.8)
aux_source_directory(. SRC_LIST)
set(HEADER_FILES
        boxworld.h)
add_executable(${PROJECT_NAME} ${SRC_LIST} ${HEADER_FILES})
target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARIES} ${PNG_LIBRARIES} emuGLES2s 3dmaths GL)
