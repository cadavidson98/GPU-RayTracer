project(glad LANGUAGES C)

add_library(glad STATIC
        src/glad.c
        include/glad/glad.h
        include/glad/khrplatform.h
        )

target_include_directories(glad
        PUBLIC
        $<INSTALL_INTERFACE:include>
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        )