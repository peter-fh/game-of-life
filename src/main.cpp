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

int main(int argc, char* argv[]) {

	int species = 5;
	if (argc >= 2) {
		int species_arg = atoi(argv[1]);
		if (species_arg >= 5 && species_arg <= 10) {
			std::cout << "Starting game of life with " << species_arg << " species\n";
			species = species_arg;
		} else if (argc == 3) {
			if (std::string(argv[2]) == "--force") {
				std::cout << "Starting game of life with " << species_arg << " species\n";
				species = species_arg;
			} else {
				std::cout << argv[2] << "\n";
			}
		} else {
			std::cout << "Invalid argument was given, defaulting to 5\n";
		}
	} else {
		std::cout << "Defaulting to 5 species since none were entered\n";
	}
	GLFWwindow* window = init_window(WIDTH, HEIGHT, "MUG TIME");
	Shader shader("vertex.glsl", "fragment.glsl");
	Grid* grid = new Grid(WIDTH, HEIGHT, species);
	grid->populate();
	GridRenderer renderer(grid, 4);

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
	glPointSize(2.0f);

	const double target_fps = 30;
	const double target_frame_time = 1.0 / target_fps;
	double average_frame_time = 0.05;
	const double alpha = 0;

	while (!glfwWindowShouldClose(window)) {

		processInput(window);

		glClearColor(BACKGROUND_COLOR);
		glClear(GL_COLOR_BUFFER_BIT);

		shader.use();

		double last_frame_time = glfwGetTime();
		renderer.step();
		double current_frame_time = glfwGetTime() - last_frame_time;


		glfwSwapBuffers(window);

		glfwPollEvents();

		average_frame_time = alpha * average_frame_time + (1 - alpha) * current_frame_time;
		int fps = average_frame_time < target_frame_time ? target_fps : (1.0 / average_frame_time);
		std::cout << " Frame time: " << std::round(average_frame_time * 1000) << "ms " << "(" << fps << "fps) \r";
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
