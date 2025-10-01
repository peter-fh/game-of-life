#include "grid.h"
#include <vector>
#include <random>
#include <iostream>

Grid::Grid(int width, int height, int species) {
	m_grid = std::vector<std::vector<int>>(
		width,
		std::vector<int>(height)
	);
	m_width = width;
	m_height = height;
	m_species = species;
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
	int threshold = (10 - m_species) * 10;
	if (threshold < 0) {
		threshold = 0;
	}

	// https://stackoverflow.com/questions/13445688/how-to-generate-a-random-number-in-c
	std::random_device dev;
	std::mt19937 rng(0);
	std::uniform_int_distribution<std::mt19937::result_type> distribution(1,100);
	std::uniform_int_distribution<std::mt19937::result_type> color_distribution(1, m_species);

	for (int x=0; x < m_width; x++) {
		for (int y=0; y < m_height; y++) {
			bool should_enable = distribution(rng) > threshold;
			if (should_enable) {
				int color = color_distribution(rng);
				m_grid[x][y] = color;
			}
		}
	}
	int total_points = get_active_points();
	double points_percentage = double(total_points) / double(m_height * m_width) * 100;
	std::cout << "Populated " << get_active_points() << " squares (" << std::round(points_percentage) << "%)\n";
}

