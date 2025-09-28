#include <vector>
#include "GL/glew.h"
#include "GLFW/glfw3.h"

class Grid {
public:
    Grid(int width, int height);
    int check(int x, int y);
    bool set(int x, int y, int value);
    void clear();
    void populate();
    int get_active_points();
    int m_width;
    int m_height;
private:
    std::vector<std::vector<int>> m_grid;
};

struct Vertex {
    GLfloat position[2];
    GLubyte color[4];
};

class GridRenderer {
public:
    GridRenderer(Grid* grid, int cores, int species);
    void step();
private:
    void thread_step(std::vector<Vertex>& vertices, int thread);
    void swap();
    void render();
    int m_cores;
    Grid* m_grid;
    Grid* m_next;
    GLuint m_VBO;
    std::vector<std::array<GLubyte, 4>> m_colors;
    std::vector<Vertex> m_vertices;
};
