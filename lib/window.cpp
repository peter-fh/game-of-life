#include "window.h"
#include <iostream>


static void glfwError(int id, const char* description) {
	std::cout << id << std::endl;
	std::cout << description << std::endl;
}


GLFWwindow* init_window(int height, int width, std::string title){
	glfwSetErrorCallback(&glfwError);

	//Starts up glfw
	if (!glfwInit()) {
		exit(1);
	}

	//Tell GLFW what version of OpenGL we're using
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	//Core profile means only modern functions are available
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

	//Setting error callback function
	glfwSetErrorCallback(&glfwError);

	//Starts up glfw
	if (!glfwInit()) {
		exit(1);
	}

	//Tell GLFW what version of OpenGL we're using
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
	//Core profile means only modern functions are available
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
	GLFWwindow* window = glfwCreateWindow(height, width, title.c_str(), NULL, NULL);
	if (window == NULL) {
		glfwTerminate();
		exit(1);
	}
	glfwMakeContextCurrent(window);

	//Start up glew
	glewInit();
	return window;
}
