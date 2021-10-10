cmake_minimum_required(VERSION 3.9)
project(GPU_Raytracer LANGUAGES C CXX)

message("Current dir: ${CMAKE_CURRENT_SOURCE_DIR}")

set(SOURCEFILES src/raytraceGUI.cpp src/bvh.cpp)
set(HEADERFILES include/bvh.h include/PGA_3D.h include/structs.h include/config.h)

add_executable(${PROJECT_NAME} ${SOURCEFILES} ${HEADERFILES})

add_subdirectory(extern/glfw)
add_subdirectory(extern/glad)
add_subdirectory(extern/stbimage)

find_package(OpenGL REQUIRED)
target_include_directories(${PROJECT_NAME} PUBLIC include)
target_link_libraries(${PROJECT_NAME} glad glfw stbimage OpenGL::GL)

set(DATA_DIR_BUILD ${CMAKE_CURRENT_SOURCE_DIR}/data)
set(DATA_DIR_INSTALL ${CMAKE_INSTALL_PREFIX}/share/${PROJECT_NAME}/data)

message(STATUS "Data dir (in build tree): ${DATA_DIR_BUILD}")
message(STATUS "Data dir (in install tree): ${DATA_DIR_INSTALL}")

# Configure a header file to pass some of the CMake settings to the source code
configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/config.h.in
    ${CMAKE_CURRENT_SOURCE_DIR}/include/config.h
)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION bin)

install(
    DIRECTORY data/
    DESTINATION ${DATA_DIR_INSTALL}
)