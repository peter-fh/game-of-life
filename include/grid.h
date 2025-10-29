#include <vector>
#include "GL/glew.h"
#include "GLFW/glfw3.h"

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

