cmake_minimum_required(VERSION 3.0.2)

project(LungSurfacesVisualization)

find_package(VTK REQUIRED)
include(${VTK_USE_FILE})

set(GLFW_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/glfw")
set(GLM_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/glm")
set(GLEW_SOURCE_DIR "${CMAKE_SOURCE_DIR}/external/glew/glew-1.13.0")

if(EXISTS "${GLFW_SOURCE_DIR}/src/CMakeLists.txt")
else()
    file(REMOVE_RECURSE ${GLFW_SOURCE_DIR})
    execute_process(COMMAND git clone https://github.com/glfw/glfw.git ${GLFW_SOURCE_DIR})
endif()

if(EXISTS "${GLM_SOURCE_DIR}/CMakeLists.txt")
else()
    file(REMOVE_RECURSE ${GLM_SOURCE_DIR})
    execute_process(COMMAND git clone https://github.com/g-truc/glm.git ${GLM_SOURCE_DIR})
endif()


if(EXISTS "${GLEW_SOURCE_DIR}/build/cmake/CMakeLists.txt")
else()
    file(REMOVE_RECURSE ${GLEW_SOURCE_DIR})
    file(MAKE_DIRECTORY ${GLEW_SOURCE_DIR})
    message(STATUS "Downloading glew")
    file(DOWNLOAD "https://sourceforge.net/projects/glew/files/glew/1.13.0/glew-1.13.0.tgz/download"
        "${CMAKE_SOURCE_DIR}/glew.tgz"
        STATUS Status
        SHOW_PROGRESS
    )
    execute_process(
        COMMAND ${CMAKE_COMMAND} -E tar xfz "${CMAKE_SOURCE_DIR}/glew.tgz"
            WORKING_DIRECTORY  ${GLEW_SOURCE_DIR}
            RESULT_VARIABLE Result
    )
    if(NOT Result EQUAL "0")
        message(FATAL_ERROR "Failed extracting glew")
    endif()

    set(GLEW_SOURCE_DIR "${GLEW_SOURCE_DIR}/glew-1.13.0")
    file(REMOVE "${CMAKE_SOURCE_DIR}/glew.tgz")
endif()


# OpenGL
FIND_PACKAGE(OpenGL REQUIRED)
if(NOT OPENGL_FOUND)
    message(ERROR " OPENGL not found!")
endif(NOT OPENGL_FOUND)


# Building only the GLFW lib
set(BUILD_SHARED_LIBS OFF CACHE BOOL "")
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "")
set(GLFW_INSTALL OFF CACHE BOOL "")


add_subdirectory(${GLFW_SOURCE_DIR} ${GLEW_SOURCE_DIR})

include_directories(
	${GLFW_SOURCE_DIR}/include
    ${GLFW_SOURCE_DIR}/deps
    ${GLM_SOURCE_DIR}
    ${GLEW_SOURCE_DIR}/include
	includes
)

add_library(glew STATIC
    ${GLEW_SOURCE_DIR}/src/glew.c
    ${GLEW_SOURCE_DIR}/include
)

target_link_libraries(glew ${GLFW_LIBRARIES} ${OPENGL_LIBRARIES})

add_definitions( -DGLEW_STATIC)

set(SOURCES 
        main.cxx
        VTKModel.cxx
        includes/VTKModel.h
        includes/definitions.h
        shaders/common.glsl
        shaders/vertex.glsl
        shaders/fragment.glsl
)


add_executable(LungSurfacesVisualization ${SOURCES})

target_link_libraries(
    LungSurfacesVisualization 
        glfw 
        ${GLFW_LIBRARIES} 
        glew
        ${VTK_LIBRARIES}
)

