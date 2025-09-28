#include "window.h"
#include "shader.h"
#include "grid.h"
#include <thread>


#define HEIGHT 786
#define WIDTH 1024

#define BACKGROUND_COLOR 0.0f, 0.0f, 0.0f, 0.0f
#define OBJECT_COLOR 0.0f, 1.0f, 0.8f, 1.0f



bool fill_mode = false;
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods){
}


bool first_mouse = true;
double lastX;
double lastY;
void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}

void mouse_callback(GLFWwindow*, double xpos, double ypos){
}

int main() {

	GLFWwindow* window = init_window(WIDTH, HEIGHT, "MUG TIME");
	Shader shader("vertex.glsl", "fragment.glsl");
	Grid* grid = new Grid(WIDTH, HEIGHT);
	grid->populate();
	GridRenderer renderer(grid, 4, 5);

#ifdef __APPLE__
	glViewport(0, 0, WIDTH * 2, HEIGHT * 2);
#else
	glViewport(0, 0, WIDTH, HEIGHT);
#endif

	glfwSetCursorPosCallback(window, mouse_callback);  
	glfwSetKeyCallback(window, key_callback);

	glfwSwapInterval(1);

	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	glPointSize(1.0f);

	double last_frame_time = 0.0;
	const double target_fps = 30.0;
	const double target_frame_time = 1.0 / target_fps;

	while (!glfwWindowShouldClose(window)) {
		double last_frame_time = glfwGetTime();

		processInput(window);

		glClearColor(BACKGROUND_COLOR);
		glClear(GL_COLOR_BUFFER_BIT);

		shader.use();

		renderer.step();


		glfwSwapBuffers(window);

		glfwPollEvents();

		double current_frame_time = glfwGetTime() - last_frame_time;
		std::cout << " Frame time: " << std::round(current_frame_time * 1000) << "ms \r";
		std::cout << std::flush;
		double frame_time_remaining = target_frame_time - current_frame_time;
		if (frame_time_remaining > 0) {
			std::this_thread::sleep_for(std::chrono::duration<double>(frame_time_remaining));
		} 
	}

	glfwDestroyWindow(window);
	glfwTerminate();
	return 1;
}
