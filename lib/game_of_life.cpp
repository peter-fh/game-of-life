#include "game_of_life.h"
#include "GLFW/glfw3.h"
#include "oneapi/tbb/blocked_range2d.h"
#include "oneapi/tbb/parallel_for.h"
#include <vector>
#include <random>

const std::vector<std::array<GLubyte, 4>> COLORS = {
	{0, 0, 0, 255},
	{230, 25, 75, 255},
	{60, 180, 75, 255},
	{255, 225, 25, 255},
	{0, 130, 200, 255},
	{245, 130, 48, 255},
	{145, 30, 180, 255},
	{70, 240, 240, 255},
	{240, 50, 230, 255},
	{210, 245, 60, 255},
	{250, 190, 212, 255},
	{0, 128, 128, 255},
	{220, 190, 255, 255},
	{170, 110, 40, 255},
	{255, 250, 200, 255},
	{128, 0, 0, 255},
	{170, 255, 195, 255},
	{128, 128, 0, 255},
	{255, 215, 180, 255},
	{0, 0, 128, 255},
	{128, 128, 128, 255},
	{255, 255, 255, 255},
};

GameOfLife::GameOfLife(Grid* grid, int cores) {
	m_grid = grid;
	m_next = new Grid(grid->m_width, grid->m_height, grid->m_species);
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);

	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), &m_vertices[0], GL_DYNAMIC_DRAW);

	GLsizei stride = sizeof(Vertex);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);
	m_cores = cores;
}

constexpr std::array<std::pair<int, int>, 8> directions = {{
    {-1, -1}, {-1, 0}, {-1, 1},
    { 0, -1},          { 0, 1},
    { 1, -1}, { 1, 0}, { 1, 1}
}};

int nextValue(Grid* grid, int x, int y) {
	int value = grid->check(x, y);
	if (value) {
		int neighbors = 0;
		for (const std::pair<int, int>& point : directions) {
			int new_value = grid->check(x + point.first, y + point.second);
			if (new_value && new_value == value) {
				neighbors++;
			}
		}

		if (neighbors == 2 || neighbors == 3) {
			return value;
		}
		return 0;
	}

	int neighbors = 0;
	for (const std::pair<int, int>& point : directions) {
		int new_value = grid->check(x + point.first, y + point.second);
		if (new_value) {
			neighbors++;
		}
	}
	if (neighbors < 3) {
		return 0;
	}

	int total_waking_species = 0;

	static thread_local int neighbor_values[23];
	static thread_local int waking_species[3];
	std::fill_n(neighbor_values, grid->m_species + 1, 0);

	for (const std::pair<int, int>& point : directions) {
		int new_value = grid->check(x + point.first, y + point.second);
		if (new_value) {
			neighbor_values[new_value]++;
			if (neighbor_values[new_value] == 3) {
				waking_species[total_waking_species] = new_value;
				total_waking_species++;
			} else if (neighbor_values[new_value] == 4) {
				total_waking_species--;
			}
		}
	}

	if (total_waking_species == 0) {
		return 0;
	}

	srand(static_cast<unsigned int>(time(0)));
	static thread_local std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, total_waking_species - 1);
	return waking_species[dist(rng)];
}

using namespace oneapi;

void GameOfLife::step() {
	double step_start_time = glfwGetTime();
	size_t estimated_capacity = m_vertices.size() * 1.1;
	m_vertices.clear();
	m_vertices.reserve(estimated_capacity);
	double lambda_start_time = glfwGetTime();
	tbb::parallel_for(tbb::blocked_range2d<int, int>(0, m_grid->m_height, 0, m_grid->m_width), 
		   [this](const tbb::blocked_range2d<int, int>& r) {
			Grid* grid = m_grid;
			Grid* next = m_next;
			float x_midpoint = (float)grid->m_width / 2.0;
			float y_midpoint = (float)grid->m_height / 2.0;
			for (int x = r.cols().begin(); x < r.cols().end(); x++) {
				for (int y = r.rows().begin(); y < r.rows().end(); y++) {
					int value = nextValue(grid, x, y);
					if (value) {
						Vertex vertex;
						vertex.position[0] = float(x - x_midpoint) / x_midpoint;
						vertex.position[1] = float(y - y_midpoint) / y_midpoint;

						for (int i=0; i < 4; i++) {
							vertex.color[i] = COLORS[value][i];
						}
						m_vertices.push_back(vertex);

						next->set(x, y, value);
					}

				}
			   }
		}
	);
	swap();
	render();
}


void GameOfLife::swap() {
	Grid* temp = m_grid;
	m_grid = m_next;
	m_next = temp;
	m_next->clear();
}

void GameOfLife::render() {
	m_rendered_vertices.assign(m_vertices.begin(), m_vertices.end());
	//glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, m_rendered_vertices.size() * sizeof(Vertex), m_rendered_vertices.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(GL_POINTS, 0, m_rendered_vertices.size());
}

