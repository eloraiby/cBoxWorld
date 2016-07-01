add_subdirectory(c99-3d-math)
add_subdirectory(emuGLES2)

project(cBoxWorld C)
set(CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

#set(CMAKE_C_FLAGS "-std=c99")

find_package(GLFW3)
find_package(PNG)
find_package(Freetype)

cmake_minimum_required(VERSION 2.8)
set(SRC_FILES
        stb/stb_rect_pack.c
        utf8.c
        image.c
        render.c
        font.c
        main.c)
set(HEADER_FILES
        boxworld.h
        stb/stb_rect_pack.h)
include_directories(${FREETYPE_INCLUDE_DIRS})
add_executable(${PROJECT_NAME} ${SRC_FILES} ${HEADER_FILES})
target_link_libraries(${PROJECT_NAME} ${GLFW_LIBRARIES} ${PNG_LIBRARIES} ${FREETYPE_LIBRARIES} emuGLES2s 3dmaths GL)
