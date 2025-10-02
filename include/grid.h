#include <vector>
#include "GL/glew.h"
#include "GLFW/glfw3.h"

class Grid {
public:
    Grid(int width, int height, int species);
    bool check(int x, int y, int species);
    bool set(int x, int y, int species);
    void clear();
    void populate();
    int get_active_points();
    int m_width;
    int m_height;
    int m_species;
private:
    std::vector<std::vector<std::vector<bool>>> m_grid;
};

