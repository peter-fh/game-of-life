#include "grid.h"
#include <vector>
#include <random>
#include "config.h"
#include <iostream>

void clear(Grid* grid) {
	memset(grid->arr, 0, (grid->height+2)*(grid->width+2)*sizeof(uint64_t));
}
void set(Grid* grid, int x, int y, uint64_t value) {
	int i = (y+2) * grid->width + (x+2);
	grid->arr[i] = value;
}

uint64_t check(Grid* grid, int x, int y) {
	int i = (y+2) * grid->width + (x+2);
	return grid->arr[i];
}

int get_active_points(Grid* grid){
	int points = 0;
	for (int x=0; x < grid->width; x++) {
		for (int y=0; y < grid->height; y++) {
			uint64_t value = check(grid, x,y);
			if (value) {
				points++;
			}
		}
	}
	return points;
}

Grid* grid_init(int width, int height, int species) {
	Grid* grid = new Grid;
	grid->width = width;
	grid->height = height;
	grid->species = species;
	grid->arr = new uint64_t[(width+2) * (height+2)]();

	int threshold = (10 - species) * 10;
	if (threshold < 0) {
		threshold = 0;
	}

	// https://stackoverflow.com/questions/13445688/how-to-generate-a-random-number-in-c
	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> distribution(1,100);
	std::uniform_int_distribution<std::mt19937::result_type> color_distribution(1, species);

	for (int x=0; x < width; x++) {
		for (int y=0; y < height; y++) {
			bool should_enable = distribution(rng) > threshold;
			if (should_enable) {
				int color = color_distribution(rng);
				uint64_t value = 1ULL << ((color-1) * 4);
				if (value == 0) {
					std::cout << "Failed to initiate\n";
				}
				set(grid, x, y, value);
			}
		}
	}
	return grid;
}

size_t size(Grid* grid) {
	return (grid->height + 2) * (grid->width + 2);
}


/*
Grid::Grid(int width, int height, int species) {
	m_grid = std::vector<std::vector<uint64_t>>(
		width+2,
		std::vector<uint64_t>(height+2)
	);
	m_width = width;
	m_height = height;
	m_species = species;
}

uint64_t Grid::check(int x, int y) {
	return m_grid[x+1][y+1];
}

void Grid::set(int x, int y, uint64_t value) {
	m_grid[x+1][y+1] = value;
}

void Grid::clear() {
	for(auto& elem : m_grid) std::fill(elem.begin(), elem.end(), 0);
}

int Grid::get_active_points() {
	int points = 0;
	for (int x=0; x < m_width; x++) {
		for (int y=0; y < m_height; y++) {
			uint64_t value = this->check(x,y);
			if (value) {
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
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> distribution(1,100);
	std::uniform_int_distribution<std::mt19937::result_type> color_distribution(1, m_species);

	for (int x=0; x < m_width; x++) {
		for (int y=0; y < m_height; y++) {
			bool should_enable = distribution(rng) > threshold;
			if (should_enable) {
				int color = color_distribution(rng);
				uint64_t value = 1ULL << ((color-1) * 4);
				if (value == 0) {
					std::cout << "Failed to initiate\n";
				}
				set(x, y, value);
			}
		}
	}
	int total_points = get_active_points();
	double points_percentage = double(total_points) / double(m_height * m_width) * 100;
	std::cout << "Populated " << total_points << " squares (" << std::round(points_percentage) << "%)\n";
}
*/
