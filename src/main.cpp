#include "window.h"
#include "shader.h"
#include "game_of_life.h"
#include <thread>


#define BACKGROUND_COLOR 0.0f, 0.0f, 0.0f, 0.0f


void processInput(GLFWwindow* window) {
	if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
		glfwSetWindowShouldClose(window, true);
	}
}

int parse_species_arguments(char* arg, bool force) {
	int species = 5;

	int species_arg = atoi(arg);
	if (force) {
		if (species_arg > 22 || species_arg < 0) {
			std::cout << "Only 22 colors are supported, defaulting to 22\n";
			return 22;
		} 

		std::cout << "Starting game of life with " << species_arg << " species\n";
		return species_arg;
	}

	if (species_arg >= 5 && species_arg <= 10) {
		std::cout << "Starting game of life with " << species_arg << " species\n";
		return species_arg;
	} 

	std::cout << "Invalid argument was given, defaulting to 5\n";

	return 5;
}

int main(int argc, char* argv[]) {

	const int height = 784;
	const int width = 1024;
	const int cores = 4;
	const double target_fps = 30;

	std::cout << "\n";
	std::cout << "|-------------------------------------|\n";
	std::cout << "| ██████╗  █████╗  ███╗   ███╗███████╗|\n";
	std::cout << "| ██╔════╝ ██╔══██╗████╗ ████║██╔════╝|\n";
	std::cout << "| ██║  ███╗███████║██╔████╔██║█████╗  |\n";
	std::cout << "| ██║   ██║██╔══██║██║╚██╔╝██║██╔══╝  |\n";
	std::cout << "| ╚██████╔╝██║  ██║██║ ╚═╝ ██║███████╗|\n";
	std::cout << "|  ╚═════╝ ╚═╝  ╚═╝╚═╝     ╚═╝╚══════╝|\n";
	std::cout << "|                                     |\n";
	std::cout << "|  ██████╗ ███████╗                   |\n";
	std::cout << "| ██╔═══██╗██╔════╝                   |\n";
	std::cout << "| ██║   ██║█████╗                     |\n";
	std::cout << "| ██║   ██║██╔══╝                     |\n";
	std::cout << "| ╚██████╔╝██║                        |\n";
	std::cout << "|  ╚═════╝ ╚═╝                        |\n";
	std::cout << "|                                     |\n";
	std::cout << "| ██╗     ██╗███████╗███████╗         |\n";
	std::cout << "| ██║     ██║██╔════╝██╔════╝         |\n";
	std::cout << "| ██║     ██║█████╗  █████╗           |\n";
	std::cout << "| ██║     ██║██╔══╝  ██╔══╝           |\n";
	std::cout << "| ███████╗██║██║     ███████╗         |\n";
	std::cout << "| ╚══════╝╚═╝╚═╝     ╚══════╝         |\n";
	std::cout << "|-------------------------------------|\n";
	std::cout << "\n";

	bool force = false;
	bool profile = false;
	int species = 0;
	if (argc > 2) {
		for (int i=1; i < argc; i++) {
			std::string arg(argv[i]);
			if (arg == "--profile" || arg == "-p") {
				profile = true;
			}
			if (arg == "--force" || arg == "-f") {
				force = true;
			}
		}
	} 
	if (argc < 2) {
		std::cout << "No species number was given, defaulting to 5\n";
		species = 5;
	} else {
		species = parse_species_arguments(argv[1], force);
	}

	GLFWwindow* window = init_window(width, height, "Game of Life");
	Shader shader("vertex.glsl", "fragment.glsl");
	Grid* grid = new Grid(width, height, species);
	grid->populate(profile);
	GameOfLife game(grid, cores);

#ifdef __APPLE__
	glViewport(0, 0, width * 2, height * 2);
#else
	glViewport(0, 0, WIDTH, HEIGHT);
#endif

	glfwSwapInterval(1);

	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
	glPointSize(2.0f);

	const double target_frame_time = 1.0 / target_fps;
	double average_frame_time = 0;
	const double alpha = 0.9;
	int frames_passed = 0;
	double total_frame_time = 0;

	while (!glfwWindowShouldClose(window)) {

		double last_frame_time = glfwGetTime();
		processInput(window);

		glClearColor(BACKGROUND_COLOR);
		glClear(GL_COLOR_BUFFER_BIT);

		shader.use();

		game.step();


		glfwSwapBuffers(window);

		glfwPollEvents();
		double current_frame_time = glfwGetTime() - last_frame_time;

		if (average_frame_time == 0) {
			average_frame_time = current_frame_time;
		}
		average_frame_time = alpha * average_frame_time + (1 - alpha) * current_frame_time;
		int fps = average_frame_time < target_frame_time ? target_fps : (1.0 / average_frame_time);
		std::cout << " Frame time: " << std::round(average_frame_time * 1000) << "ms " << "(" << fps << "fps) \r";
		std::cout << std::flush;
		double frame_time_remaining = target_frame_time - current_frame_time;
		if (frame_time_remaining > 0) {
			std::this_thread::sleep_for(std::chrono::duration<double>(frame_time_remaining));
		}

		if (profile) {
			total_frame_time += current_frame_time;
			frames_passed++;
			if (frames_passed == 450) {
				std::cout << "Took " << std::round(total_frame_time * 1000) << "ms for the first 450 frames\n";
			}
		}
	}

	std::cout << "\n";
	glfwDestroyWindow(window);
	glfwTerminate();
	return 1;
}
