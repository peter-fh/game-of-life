#include "game_of_life.h"
#include "GLFW/glfw3.h"
#include "oneapi/tbb/blocked_range2d.h"
#include "oneapi/tbb/parallel_for.h"
#include <cstdint>
#include <vector>
#include <random>
#include "config.h"
#include <iostream>

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
	m_point_width_offset = 1.0 / float(grid->m_width);
	m_point_height_offset = 1.0 / float(grid->m_height);
}

constexpr std::array<std::pair<int, int>, 8> directions = {{
    {-1, -1}, {-1, 0}, {-1, 1},
    { 0, -1},          { 0, 1},
    { 1, -1}, { 1, 0}, { 1, 1}
}};

int _nextValue(Grid* grid, int x, int y) {
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

	static thread_local std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, total_waking_species - 1);
	return waking_species[dist(rng)];
}
const uint64_t TWO_NEIGHBOR_MASK =	0x2222222222222222;
const uint64_t THREE_NEIGHBOR_MASK =	0x3333333333333333; 
const uint64_t SPECIES_VALUE_MASK =	0x1111111111111111;
const uint64_t LAST_BIT_MASK =		0x8888888888888888;
const uint64_t THIRD_BIT_MASK =		0x4444444444444444;
const uint64_t SECOND_BIT_MASK =	0x2222222222222222;

// 0x100203001000
/*
* 0001 0010 0011 0100 0101 0110 0111 1000 1001 
* 0001 0010 0011 0100 0101 0110 0111 
* a = (n & (n >> 1)) & SPECIES_VALUE_MASK
* 0000 0000 0001 0000 0000 0000 0001
* b = (n ^ (n >> 2)) & SPECIES_VALUE_MASK
* 0001 0001 0001 0001 0000 0001 0000
* c = a & b
* 0000 0000 0001 0000 0000 0000 0000
*
*n =	0001 0010 0011 0100 0101 0110 0111 
*a =	n & SECOND_LAST_BIT_MASK
*  =	0000 0000 0000 0100 0100 0100 0100
*a =	a | a >> 1 | a >> 2
*  =	0000 0000 0000 0111 0111 0111 0111
*n &= ~a
*n=	0001 0010 0011 0100 0101 0110 0111 &
*	1111 1111 1111 1000 1000 1000 1000
*n=	0001 0010 0011 0000 0000 0000 0000
*n &= n >> 1
n=	0001 0010 0011 0000 0000 0000 0000 &
	0000 1001 0001 1000 0000 0000 0000
=	0000 0000 0001 0000 0000 0000 0000

	0001 0000 red 
	0000 0001 green

	0001 0000
	0001 0000
	0001 0000
	0001 0000
	0001 0000
	0001 0000
	0001 0000
	0001 0000

	1000 0000

	0x10 red
	0x01 green

	0x80


* m = n & THIRD_BIT_MASK
* m = m | m >> 1 | m >> 2
* n = n & ~m
* n = n & (n >> 1)
*/
uint64_t nextValue(Grid* grid, int x, int y) {
	uint64_t value = grid->check(x, y);
	uint64_t neighbors = 0ULL;
	for (const std::pair<int, int>& point : directions) {
		neighbors += grid->check(x + point.first, y + point.second);
	}

	if (!neighbors) {
		return 0ULL;
	}

	// Live cell
	if (value) {
		uint64_t value_mask = value | value << 1 | value << 2 | value << 3;

		neighbors &= value_mask;
		bool two_neighbors = neighbors == (TWO_NEIGHBOR_MASK & value_mask);
		bool three_neighbors = neighbors == (THREE_NEIGHBOR_MASK & value_mask);
		if (two_neighbors || three_neighbors){
			return value;
		}
		return 0ULL;
	}

	// Dead cell
	if (neighbors & LAST_BIT_MASK) {
		return 0ULL;
	}

	uint64_t m = neighbors & THIRD_BIT_MASK;
	m = m | m >> 1 | m >> 2;
	uint64_t n = neighbors & ~m;
	n = n & (n >> 1);
	if (!n) {
		return 0ULL;
	} 

	uint64_t leading = 1ULL << (63- __builtin_clzll(n));
	uint64_t trailing = 1ULL << __builtin_ctzll(n);
	if (leading == trailing) {
		return leading;
	}
	static thread_local std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, 1);
	if (dist(rng) == 0) {
		return leading;
	}
	return trailing;

}

using namespace oneapi;

void GameOfLife::step() {
	size_t estimated_capacity = m_vertices.size() * 1.1;
	m_vertices.clear();
	m_vertices.reserve(estimated_capacity);
	/*
	Grid* grid = m_grid;
	Grid* next = m_next;
	float x_midpoint = (float)grid->m_width / 2.0;
	float y_midpoint = (float)grid->m_height / 2.0;
	for (int x = 0; x < grid->m_width; x++) {
		for (int y = 0; y < grid->m_height; y++) {
			uint64_t value = grid->check(x, y);
			if (value) {
				Vertex vertex;
				vertex.position[0] = float(x - x_midpoint) / x_midpoint + m_point_width_offset;
				vertex.position[1] = float(y - y_midpoint) / y_midpoint + m_point_height_offset;

				int species_index = __builtin_ctzll(value) / 4;
				std::cout << "Value: " << value << ", Species index: " << species_index << "\n";
				for (int i=0; i < 4; i++) {
					vertex.color[i] = COLORS[species_index][i];
				}
				m_vertices.push_back(vertex);

			}
			int next_value = nextValue(grid, x, y);
			if (next_value) {
				//std::cout << "(" << x << ", " << y << "): " << next_value << "\n";
				next->set(x, y, next_value);
			}

		}
	}
	*/
	tbb::parallel_for(tbb::blocked_range2d<int, int>(0, m_grid->m_height, 0, m_grid->m_width), 
		   [this](const tbb::blocked_range2d<int, int>& r) {
			Grid* grid = m_grid;
			Grid* next = m_next;
			float x_midpoint = (float)grid->m_width / 2.0;
			float y_midpoint = (float)grid->m_height / 2.0;
			for (int x = r.cols().begin(); x < r.cols().end(); x++) {
				for (int y = r.rows().begin(); y < r.rows().end(); y++) {
					uint64_t value = grid->check(x, y);
					if (value) {
						Vertex vertex;
						vertex.position[0] = float(x - x_midpoint) / x_midpoint + m_point_width_offset;
						vertex.position[1] = float(y - y_midpoint) / y_midpoint + m_point_height_offset;

						int species_index = __builtin_ctzll(value) / 4+1;
						for (int i=0; i < 4; i++) {
							vertex.color[i] = COLORS[species_index][i];
						}
						m_vertices.push_back(vertex);

					}
					uint64_t next_value = nextValue(grid, x, y);
					if (next_value) {
						next->set(x, y, next_value);
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

