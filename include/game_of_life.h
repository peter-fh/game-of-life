#include <vector>
#include "GL/glew.h"
#include "GLFW/glfw3.h"
#include "grid.h"
#include "oneapi/tbb.h"

struct Vertex {
    GLfloat position[2];
    GLubyte color[4];
};

using namespace oneapi;
class Stepper {
public:

	Stepper(Grid* grid, Grid* next, tbb::concurrent_vector<Vertex>& vertices):
		m_grid(grid),
		m_next(next),
		m_vertices(vertices)
	{}

	void operator()  (const tbb::blocked_range2d<int, int>& r) const;
private:
	Grid* m_grid;
	Grid* m_next;
	tbb::concurrent_vector<Vertex>& m_vertices;
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
    oneapi::tbb::concurrent_vector<Vertex> m_vertices;
};

