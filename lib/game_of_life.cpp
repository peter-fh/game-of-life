#include "game_of_life.h"
#include <cstdint>
#include <vector>
#include <random>
#include "oneapi/tbb/blocked_range2d.h"
#include "oneapi/tbb/parallel_for.h"
#include "oneapi/tbb/combinable.h"
#include "config.h"
#include <iostream>
#ifdef _MSC_VER
#include <intrin.h>
#endif
#define CL_TARGET_OPENCL_VERSION 120
#define CL_HPP_TARGET_OPENCL_VERSION 120
#include <OpenCL/cl.h>
#include <OpenGL/OpenGL.h>

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

GameOfLife::GameOfLife(
        Grid* grid
    )
{
	m_grid = grid;
	m_next = grid_init(grid->width, grid->height, grid->species);
	clear(m_next);

	m_rng = std::mt19937_64(std::random_device{}());
	m_dist = std::uniform_int_distribution<uint64_t>(0ULL, ~(0ULL));

	cl_int err;

	cl_uint num_platforms = 0;
	clGetPlatformIDs(0, nullptr, &num_platforms);
	std::vector<cl_platform_id> platforms(num_platforms);
	clGetPlatformIDs(num_platforms, platforms.data(), nullptr);

	m_platform = platforms[0];
	cl_uint num_devices = 0;
	clGetDeviceIDs(m_platform, CL_DEVICE_TYPE_ALL, 0, nullptr, &num_devices);
	std::vector<cl_device_id> devices(num_devices);
	clGetDeviceIDs(m_platform, CL_DEVICE_TYPE_ALL, num_devices, devices.data(), nullptr);
	m_device = devices[0];

	CGLContextObj cglContext = CGLGetCurrentContext();
	CGLShareGroupObj cglShareObj = CGLGetShareGroup(cglContext);

	cl_context_properties props[] = {
		CL_CONTEXT_PROPERTY_USE_CGL_SHAREGROUP_APPLE, 
		(cl_context_properties) cglShareObj,
		0
	};
	m_ctx = clCreateContext(props, 1, &m_device, nullptr, nullptr, &err);


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
			volatile global uint* aliveCount, 
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
					atomic_inc(aliveCount);
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
			atomic_inc(aliveCount);

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


		typedef struct {
			float2 position;
			uchar color[4];
		} Vertex;

		__constant uchar COLORS[22][4] = {
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

		kernel void render(
			global ulong* in, 
			global Vertex* out,
			int height, 
			int width, 
			float x_midpoint, 
			float y_midpoint, 
			float point_width_offset, 
			float point_height_offset
		) {
			int gid = get_global_id(0);
			int x = gid % width;
			int y = gid / width;
			int i = (y+1) * (width+2) + (x+1);
			ulong value = in[i];

			Vertex vertex;
			float vertex_x = (float)(x-x_midpoint) / x_midpoint;
			float vertex_y = (float)(y-y_midpoint) / y_midpoint;

			for (int c=0; c < 4; c++) {
				vertex.color[c] = COLORS[21][c];
			}

			out[gid] = vertex;


			//int species_index = 0;
			//if (value) {
			//	species_index = (63-clz(value)) / 4+1;
			//}
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

	// --- Command Queue ---
	m_queue = clCreateCommandQueue(m_ctx, m_device, 0, &err);

	// --- Buffers ---
	size_t gridBytes = size(grid) * sizeof(uint64_t);
	m_inBuffer = clCreateBuffer(m_ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
				gridBytes, m_grid->arr, &err);
	m_outBuffer = clCreateBuffer(m_ctx, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
				gridBytes, m_next->arr, &err);
	m_countBuffer = clCreateBuffer(m_ctx, CL_MEM_READ_WRITE, 
				sizeof(cl_uint), NULL, &err);

	int num_vertices = grid->height * grid->width;
	m_vertexBuffer = clCreateBuffer(m_ctx, CL_MEM_WRITE_ONLY,
				 num_vertices * sizeof(Vertex), nullptr, &err);
	m_vertices = new Vertex[num_vertices];

	// --- Kernel setup ---
	m_gameKernel = clCreateKernel(m_program, "gameOfLife", &err);
	m_renderKernel = clCreateKernel(m_program, "render", &err);

	glGenVertexArrays(1, &m_VAO);
	glGenBuffers(1, &m_VBO);

	glBindVertexArray(m_VAO);
	glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
	glBufferData(GL_ARRAY_BUFFER, num_vertices * sizeof(Vertex), &m_vertices[0], GL_DYNAMIC_DRAW);

	GLsizei stride = sizeof(Vertex);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);
	m_point_width_offset = 2.0 / float(grid->width);
	m_point_height_offset = 2.0 / float(grid->height);

}

constexpr std::array<std::pair<int, int>, 8> directions = {{
    {-1, -1}, {-1, 0}, {-1, 1},
    { 0, -1},          { 0, 1},
    { 1, -1}, { 1, 0}, { 1, 1}
}};

const uint64_t TWO_NEIGHBOR_MASK =	0x2222222222222222;
const uint64_t THREE_NEIGHBOR_MASK =	0x3333333333333333; 
const uint64_t SPECIES_VALUE_MASK =	0x1111111111111111;
const uint64_t LAST_BIT_MASK =		0x8888888888888888;
const uint64_t THIRD_BIT_MASK =		0x4444444444444444;
const uint64_t SECOND_BIT_MASK =	0x2222222222222222;



using namespace oneapi;

void GameOfLife::CPURender()
{
	m_rendered_vertices.clear();
	tbb::combinable<std::vector<Vertex>> local_vertices;


	tbb::parallel_for(tbb::blocked_range2d<int, int>(0, m_grid->height, 0, m_grid->width), 
		[this, &local_vertices](const tbb::blocked_range2d<int, int>& r) {
			auto &verts = local_vertices.local();
			Grid* grid = m_grid;
			Grid* next = m_next;
			float x_midpoint = (float)(m_grid->width+1) / 2.0;
			float y_midpoint = (float)(m_grid->height+1) / 2.0;
			for (int x = r.cols().begin(); x < r.cols().end(); x++) {
				for (int y = r.rows().begin(); y < r.rows().end(); y++) {
					uint64_t value = check(grid, x, y);
					if (value) {
						Vertex vertex;
						vertex.position[0] = float(x - x_midpoint) / x_midpoint + m_point_width_offset;
						vertex.position[1] = float(y - y_midpoint) / y_midpoint + m_point_height_offset;

						int species_index = __builtin_ctzll(value) / 4+1;
						for (int i=0; i < 4; i++) {
							vertex.color[i] = COLORS[species_index][i];
						}
						verts.push_back(vertex);

					}
				}
			}
		}
	   );

	local_vertices.combine_each([&](std::vector<Vertex>& vec) {
		m_rendered_vertices.insert(m_rendered_vertices.end(), vec.begin(), vec.end());
	});

	glBufferData(GL_ARRAY_BUFFER, m_rendered_vertices.size() * sizeof(Vertex), m_rendered_vertices.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(GL_POINTS, 0, m_rendered_vertices.size());
}

cl_uint GameOfLife::ParallelStep()
{
	size_t globalWorkSize = m_grid->width * m_grid->height;
	size_t gridBytes = size(m_grid) * sizeof(uint64_t);
	uint64_t seed = m_dist(m_rng);
	cl_uint zero = 0;
	clEnqueueWriteBuffer(m_queue, m_countBuffer, CL_TRUE, 0, sizeof(cl_uint), &zero, 0, NULL, NULL);

	clSetKernelArg(m_gameKernel, 0, sizeof(cl_mem), &m_inBuffer);
	clSetKernelArg(m_gameKernel, 1, sizeof(cl_mem), &m_outBuffer);
	clSetKernelArg(m_gameKernel, 2, sizeof(cl_mem), &m_countBuffer);
	clSetKernelArg(m_gameKernel, 3, sizeof(uint64_t), &seed);
	clSetKernelArg(m_gameKernel, 4, sizeof(int), &m_grid->height);
	clSetKernelArg(m_gameKernel, 5, sizeof(int), &m_grid->width);
	clSetKernelArg(m_gameKernel, 6, sizeof(int), &m_grid->species);

	clEnqueueNDRangeKernel(m_queue, m_gameKernel, 1, nullptr, &globalWorkSize, nullptr, 0, nullptr, nullptr);

	CPURender();

	clFinish(m_queue);
	clEnqueueReadBuffer(m_queue, m_outBuffer, CL_TRUE, 0, gridBytes, m_next->arr, 0, nullptr, nullptr);
	cl_uint aliveCount;
	clEnqueueReadBuffer(m_queue, m_countBuffer, CL_TRUE, 0, sizeof(cl_uint), &aliveCount, 0, nullptr, nullptr);
	return aliveCount;
}


void GameOfLife::check_vertices()
{
	int num_vertices = m_grid->height * m_grid->width;
	int out_of_bounds = 0;
	for (int i=0; i < num_vertices; i++)
	{
		Vertex vert = m_vertices[i];
		int x = vert.position[0];
		int y = vert.position[1];
		if (x < -1 || x > 1 || y < -1 || y > 1){
			std::cout << "Out of bound vertex: " << x << ", " << y << "\n";
			out_of_bounds++;
		}
	}
	std::cout << "Total out of bounds vertices: " << out_of_bounds << "\n";
}


cl_uint GameOfLife::step() {
	cl_uint aliveCount = ParallelStep();
	swap();
	return aliveCount;
}

void GameOfLife::swap() {
	std::swap(m_grid, m_next);
	std::swap(m_inBuffer, m_outBuffer);
}

void GameOfLife::render() {
	glBufferData(GL_ARRAY_BUFFER, size(m_grid) * sizeof(Vertex), m_vertices, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_POINTS, 0, size(m_grid));
}

