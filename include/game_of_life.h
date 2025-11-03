#include <vector>
#include <oneapi/tbb/concurrent_vector.h>
#include "GL/glew.h"
#include "grid.h"

#define CL_TARGET_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION 120
#include <OpenCL/opencl.h>

struct Vertex {
    GLfloat position[2];
    GLubyte color[4];
};

class GameOfLife {
public:
    GameOfLife(
        Grid* grid
    );
    void step();
private:
    void swap();
    void render();
    int m_cores;
    Grid* m_grid;
    Grid* m_next;
    GLuint m_VBO;
    GLuint m_VAO;
    float m_point_width_offset;
    float m_point_height_offset;

    //oneapi::tbb::concurrent_vector<Vertex> m_vertices;
    std::vector<Vertex> m_vertices;
    std::vector<Vertex> m_rendered_vertices;

    cl_platform_id m_platform;
    cl_device_id m_device;
    cl_context m_ctx;
    cl_program m_program;
    cl_command_queue m_queue;
    cl_kernel m_kernel;

    cl_mem m_inBuffer;
    cl_mem m_outBuffer;
    cl_mem m_vertexBuffer;




};

