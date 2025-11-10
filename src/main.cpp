#include "window.h"
#include "GLFW/glfw3.h"
#include "shader.h"
#include "game_of_life.h"
#include "config.h"
#include <random>

#define BACKGROUND_COLOR 0.0f, 0.0f, 0.0f, 0.0f

const int MAX_SPECIES = 16;
bool key_pressed = false;


void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
	if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
		key_pressed = true;
	}
	if ((key == GLFW_KEY_ESCAPE || key == GLFW_KEY_Q) && action == GLFW_PRESS) {
		key_pressed = true;
		glfwSetWindowShouldClose(window, true);
	}


}

int parse_species_arguments(int argc, char* argv[]) {
	int species = 5;
	if (argc >= 2) {
		int species_arg = atoi(argv[1]);
		if (species_arg >= 5 && species_arg <= 10) {
			std::cout << "Starting game of life with " << species_arg << " species\n";
			species = species_arg;
		} else if (argc == 3) {
			if (std::string(argv[2]) == "--force") {
				if (species_arg > MAX_SPECIES) {
					std::cout << "Only " << MAX_SPECIES << " colors are supported, defaulting to " << MAX_SPECIES << "\n";
					species = MAX_SPECIES;
				} else {
					std::cout << "Starting game of life with " << species_arg << " species\n";
					species = species_arg;
				}
			} else {
				std::cout << argv[2] << "\n";
			}
		} else {
			std::cout << "Invalid argument was given, defaulting to 5\n";
		}
	} else {
		std::cout << "Defaulting to 5 species since none were entered\n";
	}
	return species;
}

const uint64_t MAGIC = 0x2545F4914F6CDD1DULL;
uint64_t xorshift64star(uint64_t x) {
    /* initial seed must be nonzero, don't use a static variable for the state if multithreaded */
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    return x * MAGIC;
}

uint pcg_hash(uint input)
{
    uint state = input * 747796405u + 2891336453u;
    uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
    return (word >> 22u) ^ word;
}

void display_randomness(int height, int width) {
	std::mt19937_64 rng(std::random_device{}());
	std::uniform_int_distribution<uint64_t> dist(0ULL, ~(0ULL));
	uint64_t seed = dist(rng);

	for (int i=0; i < height * width; i++) {
		uint64_t x = i ^ seed;
		int rng = pcg_hash(x) & 1;
		std::cout << i << ", " << rng << "\n";
	}
	exit(0);
}

int main(int argc, char* argv[]) {
 

	const int height = 784; // 784
	const int width = 1024; // 1024
	//const int grid_height = 49;
	//const int grid_width = 64;
	const int grid_height = height;
	const int grid_width = width;
	const double target_fps = 60;
	const float point_size = float(width) / float(grid_width) * 2;
	//display_randomness(height, width);


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


	int species = parse_species_arguments(argc, argv);
	GLFWwindow* window = init_window(width, height, "Game of Life");
	Shader shader("vertex.glsl", "fragment.glsl");
	Grid* grid = grid_init(grid_width, grid_height, species);
	int total_points = get_active_points(grid);
	double points_percentage = double(total_points) / double(grid->height * grid->width) * 100;
	std::cout << "Populated " << total_points << " squares (" << std::round(points_percentage) << "%)\n";

	GameOfLife game(grid);

#ifdef __APPLE__
	glViewport(0, 0, width * 2, height * 2);
#else
	glViewport(0, 0, WIDTH, HEIGHT);
#endif

	glfwSwapInterval(1);

	glDisable(GL_DEPTH_TEST);
	glPolygonMode(GL_FRONT_AND_BACK, GL_POINTS);
	glPointSize(point_size);

	glfwSetKeyCallback(window, keyCallback);

	const double target_frame_time = 1.0 / target_fps;
	double average_frame_time = 0;
	const double alpha = 0.9;
	int frames_passed = 0;
	double total_frame_time = 0;

	while (!glfwWindowShouldClose(window)) {

		double last_frame_time = glfwGetTime();

		glClearColor(BACKGROUND_COLOR);
		glClear(GL_COLOR_BUFFER_BIT);

		shader.use();

		game.step();


		glfwSwapBuffers(window);

		glfwPollEvents();
		double current_frame_time = glfwGetTime() - last_frame_time;

		if (frames_passed == 0) {
			average_frame_time = 0.01;
			//std::cout << "First frame time: " << std::round(current_frame_time * 1000) << "ms\n";
		} else { 
			total_frame_time += current_frame_time;
			average_frame_time = alpha * average_frame_time + (1 - alpha) * current_frame_time;
			int fps = average_frame_time < target_frame_time ? target_fps : (1.0 / average_frame_time);
			std::cout << " Frame time: " << std::round(average_frame_time * 1000) << "ms " << "(" << fps << "fps) \r";
			std::cout << std::flush;
		}
		double frame_time_remaining = target_frame_time - current_frame_time;
		if (frame_time_remaining > 0) {
			std::this_thread::sleep_for(std::chrono::duration<double>(frame_time_remaining));
		}
		frames_passed++;

		if (frames_passed == 460) {
			double fps_450 = 450 / total_frame_time;
			std::cout << "Took " << std::round(total_frame_time * 1000) << "ms for the first 450 frames: ";
			if (fps_450 > target_fps) {
				std::cout << std::round(fps_450) << " fps, locked to " << target_fps << "\n";
			} else {
				std::cout << std::round(fps_450) << " fps\n";
			}
		}
#ifdef DEBUG_MODE
		while (!key_pressed) {
			glfwWaitEvents();
		}
		key_pressed = false;
#endif
	}

	std::cout << "\n";
	glfwDestroyWindow(window);
	glfwTerminate();

	return 1;
}
