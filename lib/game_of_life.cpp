#include "game_of_life.h"
#include <cstdint>
#include <vector>
#include <random>
#include "oneapi/tbb/blocked_range2d.h"
#include "oneapi/tbb/parallel_for.h"
#include "oneapi/tbb/combinable.h"
#include "config.h"
#include "window.h"
#include <iostream>
#ifdef _MSC_VER
#include <intrin.h>
#endif
#define CL_TARGET_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION 120
#include <OpenCL/cl.h>
#include <OpenCL/cl_gl.h>
#include <OpenGL/OpenGL.h>


GameOfLife::GameOfLife(
        Grid* grid
    )
{
	setupGame(grid);
	setupPlatform();
	setupKernels();
	setupBuffers();

}

cl_uint GameOfLife::step() {
	cl_uint mistakeCount = ParallelStep();
	swap();
	return mistakeCount;
}


const std::vector<std::array<GLubyte, 4>> COLORS = {
	{0, 0, 0, 255},
	{230, 25, 75, 255},
	{60, 180, 75, 255},
	{255, 225, 25, 255},
	{0, 130, 200, 255},
	{245, 130, 48, 255},
	{145, 30, 180, 255},
	{70, 240, 240, 255},
	{240, 50, 230, 255},
	{210, 245, 60, 255},
	{250, 190, 212, 255},
	{0, 128, 128, 255},
	{220, 190, 255, 255},
	{170, 110, 40, 255},
	{255, 250, 200, 255},
	{128, 0, 0, 255},
	{170, 255, 195, 255},
	{128, 128, 0, 255},
	{255, 215, 180, 255},
	{0, 0, 128, 255},
	{128, 128, 128, 255},
	{255, 255, 255, 255},
};

void GameOfLife::setupGame(Grid* grid) {
	m_grid = grid;
	m_next = grid_init(grid->width, grid->height, grid->species);
	clear(m_next);

	m_rng = std::mt19937_64(std::random_device{}());
	m_dist = std::uniform_int_distribution<uint64_t>(0ULL, ~(0ULL));

	m_point_width_offset = 2.0 / float(grid->width);
	m_point_height_offset = 2.0 / float(grid->height);
	m_firstFrame = true;
}

void GameOfLife::setupPlatform() {
	cl_int err;
	cl_uint num_platforms = 0;
	clGetPlatformIDs(0, nullptr, &num_platforms);
	std::vector<cl_platform_id> platforms(num_platforms);
	clGetPlatformIDs(num_platforms, platforms.data(), nullptr);

	cl_platform_id platform = platforms[0]; // thanks apple
	cl_uint num_devices = 0;
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices);
	std::vector<cl_device_id> devices(num_devices);
	clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, num_devices, devices.data(), nullptr);
	m_device = devices[0]; // thanks apple

	CGLContextObj cglContext = CGLGetCurrentContext();
	CGLShareGroupObj cglShareObj = CGLGetShareGroup(cglContext);

	cl_context_properties props[] = {
		CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, 
		(cl_context_properties) cglShareObj,
		0
	};
	m_ctx = clCreateContext(props, 1, &m_device, nullptr, nullptr, &err);
}

void GameOfLife::setupBuffers() {
	cl_int err;
	size_t gridBytes = size(m_grid) * sizeof(uint64_t);
	m_inBuffer = clCreateBuffer(m_ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
				gridBytes, m_grid->arr, &err);
	m_outBuffer = clCreateBuffer(m_ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
				gridBytes, m_next->arr, &err);

	m_mistakeCount = clCreateBuffer(m_ctx, CL_MEM_READ_WRITE, sizeof(uint), nullptr, &err);
	m_totalVertices = clCreateBuffer(m_ctx, CL_MEM_READ_WRITE, sizeof(uint), nullptr, &err);

	m_num_vertices = m_grid->height * m_grid->width;
	m_vertices = new Vertex[m_num_vertices];

	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);

	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, m_num_vertices * sizeof(Vertex), m_vertices, GL_DYNAMIC_DRAW);

	GLsizei stride = sizeof(Vertex);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);

	m_vertexBuffer = clCreateFromGLBuffer(m_ctx, CL_MEM_READ_WRITE, m_VBO, &err);
}

void GameOfLife::setupKernels() {
	cl_int err;

	const char* kernelSrc = R"CLC(
		__constant ulong TWO_NEIGHBOR_MASK   = 0x2222222222222222UL;
		__constant ulong THREE_NEIGHBOR_MASK = 0x3333333333333333UL;
		__constant ulong SPECIES_VALUE_MASK  = 0x1111111111111111UL;
		__constant ulong LAST_BIT_MASK       = 0x8888888888888888UL;
		__constant ulong THIRD_BIT_MASK      = 0x4444444444444444UL;
		__constant ulong SECOND_BIT_MASK     = 0x2222222222222222UL;

		__constant ulong MAGIC		     = 0x2545F4914F6CDD1DUL;

		// https://www.reedbeta.com/blog/hash-functions-for-gpu-rendering/
		uint pcg_hash(uint input) {
			uint state = input * 747796405u + 2891336453u;
			uint word = ((state >> ((state >> 28u) + 4u)) ^ state) * 277803737u;
			return (word >> 22u) ^ word;
		}

		kernel void gameOfLife(
			global ulong* in, 
			global ulong* out, 
			ulong seed, 
			int height, 
			int width, 
			int species
		) {
			int gid = get_global_id(0);
			int x = gid % width;
			int y = gid / width;
			int i = (y+1) * (width+2) + (x+1);
			ulong value = in[i];
			ulong neighbors = 0ul;
			int dx = width+2;
			neighbors += in[i-dx-1];
			neighbors += in[i-dx];
			neighbors += in[i-dx+1];
			neighbors += in[i-1];
			neighbors += in[i+1];
			neighbors += in[i+dx-1];
			neighbors += in[i+dx];
			neighbors += in[i+dx+1];
			if (!neighbors) {
				out[i] = 0ul;
				return;
			}

			// Live cell
			if (value) {
				ulong value_mask = value | value << 1 | value << 2 | value << 3;
				neighbors &= value_mask;
				bool two_neighbors = neighbors == (TWO_NEIGHBOR_MASK & value_mask);
				bool three_neighbors = neighbors == (THREE_NEIGHBOR_MASK & value_mask);
				if (two_neighbors || three_neighbors){
					out[i] = value;
				} else {
					out[i] = 0UL;
				}
				return;
			}

			// Dead cell
			if (neighbors & LAST_BIT_MASK) {
				out[i] = 0UL;
				return;
			}

			ulong m = neighbors & THIRD_BIT_MASK;
			m = m | m >> 1 | m >> 2;
			ulong n = neighbors & ~m;
			n = n & (n >> 1);
			if (!n) {
				out[i] = 0UL;
				return;
			} 

			ulong leading = 1ULL << (63-clz(n));
			ulong trailing = n & (~leading);
			if (trailing == 0) {
				out[i] = leading;
				return;
			}
			ulong rng_x = seed ^ gid;
			ulong rng = pcg_hash(rng_x);
			ulong random_num = rng & 1UL;
			if (random_num == 1) {
				out[i] = leading;
			} else {
				out[i] = trailing;
			}
		}

		struct Vertex {
			float2 position;
			uchar4 color;
		};

		kernel void checkVertices(
			global ulong* grid,
			global struct Vertex* vertices,
			volatile global uint* mistakes,
			volatile global uint* totalVertices,
			int width
		) {
			int gid = get_global_id(0);
			int x = gid % width;
			int y = gid / width;
			int grid_index = (y+1) * (width+2) + (x+1);
			int vertices_index = y * width + x;
			ulong value = grid[grid_index];
			struct Vertex vertex = vertices[vertices_index];
			if (value) {
				atomic_inc(totalVertices);
			}

			if (vertex.color[3] == 0 && value != 0) {
				atomic_inc(mistakes);
			}
			if (vertex.color[3] != 0 && value == 0) {
				atomic_inc(mistakes);
			}
		}

	)CLC";


	size_t srcLen = std::strlen(kernelSrc);
	m_program = clCreateProgramWithSource(m_ctx, 1, &kernelSrc, &srcLen, &err);
	err = clBuildProgram(m_program, 1, &m_device, "", nullptr, nullptr);
	if (err != CL_SUCCESS) {
		size_t logSize = 0;
		clGetProgramBuildInfo(m_program, m_device, CL_PROGRAM_BUILD_LOG, 0, nullptr, &logSize);
		std::vector<char> log(logSize);
		clGetProgramBuildInfo(m_program, m_device, CL_PROGRAM_BUILD_LOG, logSize, log.data(), nullptr);
		std::cerr << "Build Log:\n" << log.data() << "\n";
		std::cerr << "Build failed, aborting\n";
		clReleaseProgram(m_program);
		clReleaseContext(m_ctx);
		return;
	}

	m_queue = clCreateCommandQueue(m_ctx, m_device, 0, &err);




	m_gameKernel = clCreateKernel(m_program, "gameOfLife", &err);
	m_debugKernel = clCreateKernel(m_program, "checkVertices", &err);

}


using namespace oneapi;


cl_uint GameOfLife::ParallelStep()
{
	size_t globalWorkSize = m_grid->width * m_grid->height;
	size_t gridBytes = size(m_grid) * sizeof(uint64_t);
	uint64_t seed = m_dist(m_rng);
	cl_uint zero = 0;

	if (!m_firstFrame) {
		clEnqueueWriteBuffer(m_queue, m_mistakeCount, CL_TRUE, 0, sizeof(cl_uint), &zero, 0, NULL, NULL);
		clEnqueueWriteBuffer(m_queue, m_totalVertices, CL_TRUE, 0, sizeof(cl_uint), &zero, 0, NULL, NULL);

		clSetKernelArg(m_debugKernel, 0, sizeof(cl_mem), &m_outBuffer);
		clSetKernelArg(m_debugKernel, 1, sizeof(cl_mem), &m_vertexBuffer);
		clSetKernelArg(m_debugKernel, 2, sizeof(cl_mem), &m_mistakeCount);
		clSetKernelArg(m_debugKernel, 3, sizeof(cl_mem), &m_totalVertices);
		clSetKernelArg(m_debugKernel, 4, sizeof(int), &m_grid->width);

		clEnqueueNDRangeKernel(m_queue, m_debugKernel, 1, nullptr, &globalWorkSize, nullptr, 0, nullptr, nullptr);
	}


	clSetKernelArg(m_gameKernel, 0, sizeof(cl_mem), &m_inBuffer);
	clSetKernelArg(m_gameKernel, 1, sizeof(cl_mem), &m_outBuffer);
	clSetKernelArg(m_gameKernel, 2, sizeof(uint64_t), &seed);
	clSetKernelArg(m_gameKernel, 3, sizeof(int), &m_grid->height);
	clSetKernelArg(m_gameKernel, 4, sizeof(int), &m_grid->width);
	clSetKernelArg(m_gameKernel, 5, sizeof(int), &m_grid->species);

	clEnqueueNDRangeKernel(m_queue, m_gameKernel, 1, nullptr, &globalWorkSize, nullptr, 0, nullptr, nullptr);


	tbb::parallel_for(tbb::blocked_range2d<int, int>(0, m_grid->height, 0, m_grid->width), 
		[this](const tbb::blocked_range2d<int, int>& r) {
			Grid* grid = m_grid;
			Grid* next = m_next;
			float x_midpoint = (float)(grid->width+1) / 2.0;
			float y_midpoint = (float)(grid->height+1) / 2.0;
			int width = grid->width;
			int height = grid->height;
			float x_midpoint_reciprocal = 1.0 / x_midpoint;
			float y_midpoint_reciprocal = 1.0 / y_midpoint;
			for (int x = r.cols().begin(); x < r.cols().end(); x++) {
				for (int y = r.rows().begin(); y < r.rows().end(); y++) {
					uint64_t value = check(grid, x, y);
					Vertex vertex;
					int vertex_index = y * width + x;
					if (value) {
						vertex.position[0] = float(x - x_midpoint) * x_midpoint_reciprocal + m_point_width_offset;
						vertex.position[1] = float(y - y_midpoint) * y_midpoint_reciprocal + m_point_height_offset;
						int species_index = __builtin_ctzll(value) / 4+1;
						for (int i=0; i < 4; i++) {
							vertex.color[i] = COLORS[species_index][i];
						}

					} else {
						vertex.position[0] = 0;
						vertex.position[1] = 0;
						for (int i=0; i < 4; i++) {
							vertex.color[i] = 0;
						}
						
					}
					m_vertices[vertex_index] = vertex;
				}
			}
		}
	   );


	clFinish(m_queue);

	cl_uint vertexCount;
	if (!m_firstFrame) {
		cl_uint mistakeCount;
		clEnqueueReadBuffer(m_queue, m_mistakeCount, CL_TRUE, 0, sizeof(cl_uint), &mistakeCount, 0, nullptr, nullptr);

		if (mistakeCount != 0) {
			std::cout << "Frame had " << mistakeCount << " mistakes!\n\n" << std::flush;
		}

		clEnqueueReadBuffer(m_queue, m_totalVertices, CL_TRUE, 0, sizeof(cl_uint), &vertexCount, 0, nullptr, nullptr);
	} else {
		m_firstFrame = false;
		vertexCount = 0;
	}

	clEnqueueReadBuffer(m_queue, m_outBuffer, CL_TRUE, 0, gridBytes, m_next->arr, 0, nullptr, nullptr);

	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, m_num_vertices * sizeof(Vertex), &m_vertices[0], GL_DYNAMIC_DRAW);
	glDrawArrays(GL_POINTS, 0, m_grid->height * m_grid->width);


	return vertexCount;
}

void GameOfLife::swap() {
	std::swap(m_grid, m_next);
	std::swap(m_inBuffer, m_outBuffer);
}




