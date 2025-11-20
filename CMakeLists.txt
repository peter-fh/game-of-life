cmake_minimum_required(VERSION 3.10)

project(GameOfLife VERSION 1.0)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB LIB_SOURCES "lib/*.cpp")

add_executable(GameOfLife src/main.cpp ${LIB_SOURCES})
set_target_properties(GameOfLife PROPERTIES OUTPUT_NAME "Game Of Life")

if(DEBUG_MODE)
	message("Debug mode")
	target_compile_definitions(GameOfLife PRIVATE DEBUG_MODE)
endif()

include_directories(
	./include
)

find_package(TBB REQUIRED)

find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)
find_package(GLFW3 REQUIRED)
find_library(OpenCL_LIBRARY OpenCL)


target_link_libraries(GameOfLife
	PRIVATE
		TBB::tbb
		OpenGL::GL
		GLEW::GLEW
		glfw
		${OpenCL_LIBRARY}
)

