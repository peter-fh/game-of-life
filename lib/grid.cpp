#include "grid.h"
#include <vector>
#include <random>
#include <iostream>

Grid::Grid(int width, int height, int species) {
	m_grid = std::vector<std::vector<std::vector<bool>>>(
		width+2,
		std::vector<std::vector<bool>>(
			height+2,
			std::vector<bool>(species)
		)
	);
	m_width = width;
	m_height = height;
	m_species = species;
}


void Grid::clear() {
	for(auto& columns : m_grid){
		for(auto& elem: columns) {
		std::fill(elem.begin(), elem.end(), 0);
		}
	};
}

int Grid::get_active_points() {
	int points = 0;
	for (int species=0; species < m_species; species++){
		for (int x=1; x < m_width-1; x++) {
			for (int y=1; y < m_height-1; y++) {
				if (this->check(x, y, species)) {
					points++;
				}
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
	std::uniform_int_distribution<std::mt19937::result_type> color_distribution(1, m_species-1);

	for (int x=1; x < m_width-1; x++) {
		for (int y=1; y < m_height-1; y++) {
			bool should_enable = distribution(rng) > threshold;
			if (should_enable) {
				int color = color_distribution(rng);
				set(x, y, color);
			}
		}
	}
	int total_points = get_active_points();
	double points_percentage = double(total_points) / double(m_height * m_width) * 100;
	std::cout << "Populated " << get_active_points() << " squares (" << std::round(points_percentage) << "%)\n";
}

