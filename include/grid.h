#include <vector>
#include "GL/glew.h"
#include "GLFW/glfw3.h"

struct Grid {
    int height;
    int width;
    int species;
    uint64_t* arr;
};

void clear(Grid* grid);
void set(Grid* grid, int x, int y, uint64_t value);
uint64_t check(Grid* grid, int x, int y);
int get_active_points(Grid* grid);

Grid* grid_init(int width, int height, int species);

/*
class Grid {
public:
    Grid(int width, int height, int species);
    uint64_t check(int x, int y);
    void set(int x, int y, uint64_t value);
    void clear();
    void populate();
    int get_active_points();
    int m_width;
    int m_height;
    int m_species;
private:
    std::vector<std::vector<uint64_t>> m_grid;
};
*/

