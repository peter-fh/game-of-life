#include <vector>
#include "GL/glew.h"
#include "GLFW/glfw3.h"

class Grid {
public:
    Grid(int width, int height, int species);
    int check(int x, int y);
    bool set(int x, int y, int value);
    void clear();
    void populate(bool profile);
    int get_active_points();
    int m_width;
    int m_height;
    int m_species;
private:
    std::vector<std::vector<int>> m_grid;
};

