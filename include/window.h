#ifndef WINDOW_H
#define WINDOW_H

#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include <string>

GLFWwindow* init_window(int height, int width, std::string title);

#endif
