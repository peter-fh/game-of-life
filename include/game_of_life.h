#include <vector>
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "grid.h"

struct Vertex {
    GLfloat position[2];
    GLubyte color[4];
};

class GameOfLife {
public:
    GameOfLife(Grid* grid, int cores);
    void step();
private:
    void thread_step(std::vector<Vertex>& vertices, int thread);
    void swap();
    void render();
    int m_cores;
    Grid* m_grid;
    Grid* m_next;
    GLuint m_VBO;
    GLuint m_VAO;
    std::vector<std::array<GLubyte, 4>> m_colors;
    std::vector<Vertex> m_vertices;
    std::vector<std::vector<Vertex>> m_thread_vertices;
};
