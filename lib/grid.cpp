#include "grid.h"
#include <functional>
#include <vector>
#include <random>
#include <thread>

Grid::Grid(int width, int height) {
	m_grid = std::vector<std::vector<int>>(
		width,
		std::vector<int>(height)
	);
	m_width = width;
	m_height = height;
}

int Grid::check(int x, int y) {
	if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
		return 0;
	}

	return m_grid[x][y];
}

bool Grid::set(int x, int y, int value) {
	if (x < 0 || x >= m_width || y < 0 || y >= m_height) {
		return false;
	}
	m_grid[x][y] = value;
	return true;
}

void Grid::clear() {
	for (int x=0; x < m_width; x++) {
		for (int y=0; y < m_height; y++) {
			m_grid[x][y] = 0;
		}
	}
}

int Grid::get_active_points() {
	int points = 0;
	for (int x=0; x < m_width; x++) {
		for (int y=0; y < m_height; y++) {
			if (this->check(x, y)) {
				points++;
			}
		}
	}
	return points;
}

void Grid::populate() {
	srand(time(NULL));
	int threshold = 50;

	// https://stackoverflow.com/questions/13445688/how-to-generate-a-random-number-in-c
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> distribution(1,100);
	std::uniform_int_distribution<std::mt19937::result_type> color_distribution(1,22);

	for (int x=0; x < m_width; x++) {
		for (int y=0; y < m_height; y++) {
			bool should_enable = distribution(rng) > threshold;
			if (should_enable) {
				int color = color_distribution(rng);
				m_grid[x][y] = color;
			}
		}
	}
}

GridRenderer::GridRenderer(Grid* grid, int cores, int species) {
	m_grid = grid;
	m_next = new Grid(grid->m_width, grid->m_height);
	GLuint VAO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &m_VBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_DYNAMIC_DRAW);

	GLsizei stride = sizeof(Vertex);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);
	m_cores = cores;
	m_colors = {
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
		{0, 0, 0, 255},
	};
}

void GridRenderer::step() {
	std::thread threads[m_cores];
	m_vertices = {};
	std::vector<std::vector<Vertex>> thread_vertices = std::vector<std::vector<Vertex>>(
		m_cores,
		std::vector<Vertex>()
	);
	for (int i=0; i < m_cores; i++) {
		threads[i] = std::thread(&GridRenderer::thread_step, this, std::ref(thread_vertices[i]), i);
	}
	for (int i=0; i < m_cores; i++) {
		threads[i].join();
		m_vertices.insert(m_vertices.end(), thread_vertices[i].begin(), thread_vertices[i].end());
	}
	swap();
	render();
}

int checkNeighbors(Grid* grid, int x, int y){ 
	int neighbors = 0;
	if (grid->check(x-1, y-1) != 0) {
		neighbors++;
	}
	if (grid->check(x-1, y) != 0) {
		neighbors++;
	}
	if (grid->check(x-1, y+1) != 0) {
		neighbors++;
	}
	if (grid->check(x, y-1) != 0) {
		neighbors++;
	}
	if (grid->check(x, y+1) != 0) {
		neighbors++;
	}
	if (grid->check(x+1, y-1) != 0) {
		neighbors++;
	}
	if (grid->check(x+1, y) != 0) {
		neighbors++;
	}
	if (grid->check(x+1, y+1) != 0) {
		neighbors++;
	}
	return neighbors;
}

const std::vector<std::pair<int, int>> directions = {
	std::make_pair(-1, -1),
	std::make_pair(-1, 0),
	std::make_pair(-1, 1),
	std::make_pair(0, -1),
	std::make_pair(0, 1),
	std::make_pair(1, -1),
	std::make_pair(1, 0),
	std::make_pair(1, 1),
};

int nextValue(Grid* grid, int x, int y) {
	int value = 0;


	int neighbors = 0;
	int neighbor_values[8];
	for (const std::pair<int, int>& point : directions) {
		value = grid->check(x + point.first, y + point.second);
		if (value) {
			neighbor_values[neighbors] = value;
			neighbors++;
		}
	}

	value = grid->check(x, y);
	if (value) {
		if (neighbors == 2 || neighbors == 3) {
			return value;
		}
		return 0;
	}

	if (neighbors != 3) {
		return 0;
	}

	if (neighbor_values[0] == neighbor_values[1] || neighbor_values[0] == neighbor_values[2]) {
		return neighbor_values[0];
	}
	return neighbor_values[1];
}


void GridRenderer::thread_step(std::vector<Vertex>& vertices, int thread) {
	int width = m_grid->m_width / m_cores;
	int begin = width * thread;
	int end = width * (thread + 1);
	float x_midpoint = (float)m_grid->m_width / 2.0;
	float y_midpoint = (float)m_grid->m_height / 2.0;
	for (int x = begin; x < end; x++) {
		for (int y = 0; y < m_grid->m_height; y++) {
			int value = nextValue(m_grid, x, y);
			if (value) {
				Vertex vertex;
				vertex.position[0] = float(x - x_midpoint) / x_midpoint;
				vertex.position[1] = float(y - y_midpoint) / y_midpoint;

				for (int i=0; i < 4; i++) {
					vertex.color[i] = m_colors[value][i];
				}
				vertices.push_back(vertex);

				m_next->set(x, y, value);
			}

		}
	}
}

void GridRenderer::swap() {
	Grid* temp = m_grid;
	m_grid = m_next;
	m_next = temp;
	m_next->clear();
}

void GridRenderer::render() {
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(GL_POINTS, 0, m_vertices.size());
}

