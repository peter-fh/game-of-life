#include <vector>
#include "GL/glew.h"
#include "GLFW/glfw3.h"

class Grid {
public:
    Grid(int width, int height, int species);
    void clear();
    void populate();
    inline bool check(int x, int y) {
        return m_grid[x+1][y+1];
    }

    inline void set(int x, int y, int value) {
        m_grid[x+1][y+1] = value;
    }
    int get_active_points();
    int m_width;
    int m_height;
    int m_species;
private:
    std::vector<std::vector<uint64_t>> m_grid;
};

