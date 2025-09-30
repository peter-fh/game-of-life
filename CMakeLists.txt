cmake_minimum_required(VERSION 3.10)

project(GameOfLife VERSION 1.0)


set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

file(GLOB LIB_SOURCES "lib/*.cpp")

add_executable(GameOfLife src/main.cpp ${LIB_SOURCES})
set_target_properties(GameOfLife PROPERTIES OUTPUT_NAME "Game Of Life")

include_directories(
	./include
	/opt/homebrew/Cellar/glew/2.2.0_1/include 
	/opt/homebrew/Cellar/glfw/3.4/include
	/opt/homebrew/Cellar/glm/1.0.1/include
	/opt/homebrew/Cellar/assimp/6.0.2/include
)

find_package(TBB REQUIRED)

target_link_libraries(GameOfLife
	TBB::tbb
	/opt/homebrew/Cellar/glfw/3.4/lib/libglfw.dylib 
	/opt/homebrew/Cellar/glew/2.2.0_1/lib/libGLEW.dylib 
	/opt/homebrew/Cellar/glm/1.0.1/lib/libglm.dylib
	/opt/homebrew/Cellar/assimp/6.0.2/lib/libassimp.dylib
	/System/Library/Frameworks/OpenGL.framework
)
