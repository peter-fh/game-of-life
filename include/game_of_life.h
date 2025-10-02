#include <vector>
#include <oneapi/tbb/concurrent_vector.h>
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
    void swap();
    void render();
    int m_cores;
    Grid* m_grid;
    Grid* m_next;
    GLuint m_VBO;
    GLuint m_VAO;
    oneapi::tbb::concurrent_vector<Vertex> m_vertices;
    std::vector<Vertex> m_rendered_vertices;

    std::atomic<double> m_step_time;
    std::atomic<double> m_vertex_time;
};

