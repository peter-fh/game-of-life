cmake_minimum_required(VERSION 3.10)

project(GameOfLife VERSION 1.0)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB LIB_SOURCES "lib/*.cpp")

add_executable(GameOfLife src/main.cpp ${LIB_SOURCES})
set_target_properties(GameOfLife PROPERTIES OUTPUT_NAME "Game Of Life")

include_directories(
	./include
)

find_package(TBB REQUIRED)

find_package(OpenCLHeaders REQUIRED)
find_package(OpenCLICDLoader REQUIRED)
find_package(OpenCLHeadersCpp REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLFW3 REQUIRED)
find_package(GLM REQUIRED)
find_package(assimp REQUIRED)

target_link_libraries(GameOfLife
	TBB::tbb
	OpenCL::Headers
	OpenCL::OpenCL
	OpenCL::HeadersCpp
	OpenGL::GL
        GLEW::GLEW
        glfw
        glm::glm
        assimp::assimp
)
