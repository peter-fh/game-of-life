#include <random>
#include <vector>
#include <oneapi/tbb/concurrent_vector.h>
#include "GL/glew.h"
#include "grid.h"

#define CL_TARGET_OPENCL_VERSION 120
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnullability-completeness-on-arrays"
#include <OpenCL/opencl.h>
#pragma clang diagnostic pop

struct Vertex {
    GLfloat position[2];
    GLubyte color[4];
    GLubyte _pad[4]; // Padding for OpenCL
};

class GameOfLife {
public:
    GameOfLife(
        Grid* grid
    );
    cl_uint step();
private:
    /* Setup Functions */
    void setupGame(Grid* grid);
    void setupPlatform();
    void setupKernels();
    void setupBuffers();

    /* Recompute functions */
    cl_uint ParallelStep();
    void swap();

    /* Game Specific Variables */
    float m_point_width_offset;
    float m_point_height_offset;
    int m_num_vertices;
    bool m_firstFrame;
    std::mt19937_64 m_rng;
    std::uniform_int_distribution<uint64_t> m_dist;


    /* OpenCL objects */
    cl_device_id m_device;
    cl_context m_ctx;
    cl_program m_program;
    cl_command_queue m_queue;
    cl_kernel m_gameKernel;
    cl_kernel m_debugKernel;

    /* Buffers */
    Grid* m_grid;
    Grid* m_next;
    GLuint m_VBO;
    GLuint m_VAO;
    Vertex* m_vertices;
    cl_mem m_inBuffer;
    cl_mem m_outBuffer;
    cl_mem m_vertexBuffer;
    cl_mem m_totalVertices;
    cl_mem m_mistakeCount;




};

