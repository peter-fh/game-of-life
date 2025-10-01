#include "game_of_life.h"
#include <thread>
#include <vector>


GameOfLife::GameOfLife(Grid* grid, int cores) {
	m_grid = grid;
	m_next = new Grid(grid->m_width, grid->m_height, grid->m_species);
	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);

	glBindVertexArray(m_VAO);
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
	};
	m_thread_vertices = std::vector<std::vector<Vertex>>(m_cores);

}

void GameOfLife::step() {
	std::thread threads[m_cores];
	size_t estimated_capacity = m_vertices.size() * 1.1;
	m_vertices.clear();
	m_vertices.reserve(estimated_capacity);

	for (auto& vec : m_thread_vertices) {
		vec.clear();
		vec.reserve(estimated_capacity / m_cores);
	}

	for (int i=0; i < m_cores; i++) {
		threads[i] = std::thread(&GameOfLife::thread_step, this, std::ref(m_thread_vertices[i]), i);
	}

	for (int i=0; i < m_cores; i++) {
		threads[i].join();
		m_vertices.insert(m_vertices.end(), m_thread_vertices[i].begin(), m_thread_vertices[i].end());
	}

	swap();
	render();
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
	static thread_local int neighbor_values[23];
	std::fill_n(neighbor_values, grid->m_species + 1, 0);

	for (const std::pair<int, int>& point : directions) {
		int new_value = grid->check(x + point.first, y + point.second);
		if (new_value) {
			neighbor_values[new_value]++;
			neighbors++;
		}
	}

	if (neighbors < 3) {
		return 0;
	}


	for (int i=1; i < grid->m_species+1; i++) {
		if (neighbor_values[i] == 3) {
			return i;
		}
	}


	return 0;

}


void GameOfLife::thread_step(std::vector<Vertex>& vertices, int thread) {
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

void GameOfLife::swap() {
	Grid* temp = m_grid;
	m_grid = m_next;
	m_next = temp;
	m_next->clear();
}

void GameOfLife::render() {
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, m_vertices.size() * sizeof(Vertex), m_vertices.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(GL_POINTS, 0, m_vertices.size());
}

