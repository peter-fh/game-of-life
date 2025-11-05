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
#include <OpenCL/opencl.h>

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
const GLubyte _COLORS[22][4] = {
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

	cl_context_properties props[] = {
		CL_CONTEXT_PLATFORM, (cl_context_properties)m_platform,
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

		kernel void gameOfLife(global const ulong* in, global ulong* out, int height, int width, int species) {
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
			ulong random_num = i & 1UL;
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
			global const ulong* in, 
			global Vertex* out,
			int height, 
			int width, 
			float x_midpoint, 
			float y_midpoint, 
			float point_width_offset, 
			float point_height_offset
		) {
			int gid = get_global_id(0);
			int y = gid / width;
			int x = gid % width;
			int i = (y + 1) * (width+2) + (x + 1);
			ulong value = in[i];
			
			Vertex vertex;
			vertex.position.x = ((float)((x - x_midpoint)) / x_midpoint) + point_width_offset;
			vertex.position.y = ((float)((y - y_midpoint)) / y_midpoint) + point_height_offset+0.5;

			int species_index = 0;
			if (value) {
				species_index = (63-clz(value)) / 4+1;
			}
			for (int c=0; c < 4; c++) {
				vertex.color[c] = COLORS[species_index][c];
			}
			out[gid] = vertex;
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
	std::cout << "Vertex stride: " << stride << "\n";
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, stride, (void*)0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 4, GL_UNSIGNED_BYTE, GL_TRUE, stride, (void*)offsetof(Vertex, color));
	glEnableVertexAttribArray(1);
	m_point_width_offset = 1.0 / float(grid->width);
	m_point_height_offset = 1.0 / float(grid->height);

}

constexpr std::array<std::pair<int, int>, 8> directions = {{
    {-1, -1}, {-1, 0}, {-1, 1},
    { 0, -1},          { 0, 1},
    { 1, -1}, { 1, 0}, { 1, 1}
}};

int _nextValue(Grid* grid, int x, int y) {
	int value = check(grid, x, y);
	if (value) {
		int neighbors = 0;
		for (const std::pair<int, int>& point : directions) {
			int new_value = check(grid, x + point.first, y + point.second);
			if (new_value && new_value == value) {
				neighbors++;
			}
		}

		if (neighbors == 2 || neighbors == 3) {
			return value;
		}
		return 0;
	}

	int neighbors = 0;
	for (const std::pair<int, int>& point : directions) {
		int new_value = check(grid, x + point.first, y + point.second);
		if (new_value) {
			neighbors++;
		}
	}
	if (neighbors < 3) {
		return 0;
	}

	int total_waking_species = 0;

	static thread_local int neighbor_values[23];
	static thread_local int waking_species[3];
	std::fill_n(neighbor_values, grid->species + 1, 0);

	for (const std::pair<int, int>& point : directions) {
		int new_value = check(grid, x + point.first, y + point.second);
		if (new_value) {
			neighbor_values[new_value]++;
			if (neighbor_values[new_value] == 3) {
				waking_species[total_waking_species] = new_value;
				total_waking_species++;
			} else if (neighbor_values[new_value] == 4) {
				total_waking_species--;
			}
		}
	}

	if (total_waking_species == 0) {
		return 0;
	}

	static thread_local std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, total_waking_species - 1);
	return waking_species[dist(rng)];
}
const uint64_t TWO_NEIGHBOR_MASK =	0x2222222222222222;
const uint64_t THREE_NEIGHBOR_MASK =	0x3333333333333333; 
const uint64_t SPECIES_VALUE_MASK =	0x1111111111111111;
const uint64_t LAST_BIT_MASK =		0x8888888888888888;
const uint64_t THIRD_BIT_MASK =		0x4444444444444444;
const uint64_t SECOND_BIT_MASK =	0x2222222222222222;

// 0x100203001000
/*
* 0001 0010 0011 0100 0101 0110 0111 1000 1001 
* 0001 0010 0011 0100 0101 0110 0111 
* a = (n & (n >> 1)) & SPECIES_VALUE_MASK
* 0000 0000 0001 0000 0000 0000 0001
* b = (n ^ (n >> 2)) & SPECIES_VALUE_MASK
* 0001 0001 0001 0001 0000 0001 0000
* c = a & b
* 0000 0000 0001 0000 0000 0000 0000
*
*n =	0001 0010 0011 0100 0101 0110 0111 
*a =	n & SECOND_LAST_BIT_MASK
*  =	0000 0000 0000 0100 0100 0100 0100
*a =	a | a >> 1 | a >> 2
*  =	0000 0000 0000 0111 0111 0111 0111
*n &= ~a
*n=	0001 0010 0011 0100 0101 0110 0111 &
*	1111 1111 1111 1000 1000 1000 1000
*n=	0001 0010 0011 0000 0000 0000 0000
*n &= n >> 1
n=	0001 0010 0011 0000 0000 0000 0000 &
	0000 1001 0001 1000 0000 0000 0000
=	0000 0000 0001 0000 0000 0000 0000

	0001 0000 red 
	0000 0001 green

	0001 0000
	0001 0000
	0001 0000
	0001 0000
	0001 0000
	0001 0000
	0001 0000
	0001 0000

	1000 0000

	0x10 red
	0x01 green

	0x80


* m = n & THIRD_BIT_MASK
* m = m | m >> 1 | m >> 2
* n = n & ~m
* n = n & (n >> 1)
*/

inline int least_significant_index(uint64_t n) {
#ifdef _MSC_VER
	int index;
	_BitScanForward64(&index, n);
	return index;
#else
	return __builtin_ctzll(n);
#endif
}

inline int most_significant_index(uint64_t n) {
#ifdef _MSC_VER
	int index;
	_BitScanReverse64(&index, n);
	return index;
#else
	return 63-__builtin_clzll(n);
#endif
}

uint64_t nextValue(Grid* grid, int x, int y) {
	uint64_t value = check(grid, x, y);
	uint64_t neighbors = 0ULL;
	for (const std::pair<int, int>& point : directions) {
		neighbors += check(grid, x + point.first, y + point.second);
	}

	if (!neighbors) {
		return 0ULL;
	}

	// Live cell
	if (value) {
		uint64_t value_mask = value | value << 1 | value << 2 | value << 3;

		neighbors &= value_mask;
		bool two_neighbors = neighbors == (TWO_NEIGHBOR_MASK & value_mask);
		bool three_neighbors = neighbors == (THREE_NEIGHBOR_MASK & value_mask);
		if (two_neighbors || three_neighbors){
			return value;
		}
		return 0ULL;
	}

	// Dead cell
	if (neighbors & LAST_BIT_MASK) {
		return 0ULL;
	}

	uint64_t m = neighbors & THIRD_BIT_MASK;
	m = m | m >> 1 | m >> 2;
	uint64_t n = neighbors & ~m;
	n = n & (n >> 1);
	if (!n) {
		return 0ULL;
	} 

	uint64_t leading = 1ULL << most_significant_index(n);
	uint64_t trailing = 1ULL << least_significant_index(n);
	if (leading == trailing) {
		return leading;
	}
	static thread_local std::mt19937 rng(std::random_device{}());
	std::uniform_int_distribution<int> dist(0, 1);
	if (dist(rng) == 0) {
		return leading;
	}
	return trailing;

}

using namespace oneapi;


void GameOfLife::GPUStep()
{
	size_t globalWorkSize = m_grid->width * m_grid->height;
	size_t gridBytes = size(m_grid) * sizeof(uint64_t);


	clSetKernelArg(m_gameKernel, 0, sizeof(cl_mem), &m_inBuffer);
	clSetKernelArg(m_gameKernel, 1, sizeof(cl_mem), &m_outBuffer);
	clSetKernelArg(m_gameKernel, 2, sizeof(int), &m_grid->height);
	clSetKernelArg(m_gameKernel, 3, sizeof(int), &m_grid->width);
	clSetKernelArg(m_gameKernel, 4, sizeof(int), &m_grid->species);

	clEnqueueNDRangeKernel(m_queue, m_gameKernel, 1, nullptr, &globalWorkSize, nullptr, 0, nullptr, nullptr);


	clFinish(m_queue);
	clEnqueueReadBuffer(m_queue, m_outBuffer, CL_TRUE, 0, gridBytes, m_next->arr, 0, nullptr, nullptr);
}

void GameOfLife::GPURender()
{
	size_t globalWorkSize = m_grid->width * m_grid->height;
	size_t vertexBytes = m_grid->width * m_grid->height * sizeof(Vertex);
	size_t gridBytes = size(m_grid) * sizeof(uint64_t);
	clEnqueueWriteBuffer(m_queue, m_inBuffer, CL_TRUE, 0, gridBytes,
		      m_grid->arr, 0, nullptr, nullptr);

	float x_midpoint = (float)(m_grid->width+1) / 2.0;
	float y_midpoint = (float)(m_grid->height+1) / 2.0;
	clSetKernelArg(m_renderKernel, 0, sizeof(cl_mem), &m_inBuffer);
	clSetKernelArg(m_renderKernel, 1, sizeof(cl_mem), &m_vertexBuffer);
	clSetKernelArg(m_renderKernel, 2, sizeof(int), &m_grid->height);
	clSetKernelArg(m_renderKernel, 3, sizeof(int), &m_grid->width);
	clSetKernelArg(m_renderKernel, 4, sizeof(float), &x_midpoint);
	clSetKernelArg(m_renderKernel, 5, sizeof(float), &y_midpoint);
	clSetKernelArg(m_renderKernel, 6, sizeof(float), &m_point_width_offset);
	clSetKernelArg(m_renderKernel, 7, sizeof(float), &m_point_height_offset);

	clEnqueueNDRangeKernel(m_queue, m_renderKernel, 1, nullptr, &globalWorkSize, nullptr, 0, nullptr, nullptr);

	clFinish(m_queue);
	clEnqueueReadBuffer(m_queue, m_vertexBuffer, CL_TRUE, 0, vertexBytes, m_vertices, 0, nullptr, nullptr);
}

void GameOfLife::CPUStep()
{
	tbb::parallel_for(tbb::blocked_range2d<int, int>(0, m_grid->height, 0, m_grid->width), 
		[this](const tbb::blocked_range2d<int, int>& r) {
			Grid* grid = m_grid;
			Grid* next = m_next;
			for (int x = r.cols().begin(); x < r.cols().end(); x++) {
				for (int y = r.rows().begin(); y < r.rows().end(); y++) {
					uint64_t next_value = nextValue(grid, x, y);
					set(next, x, y, next_value);
				}
			}
		}
	   );
}
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

void GameOfLife::ParallelStep()
{
	size_t globalWorkSize = m_grid->width * m_grid->height;
	size_t gridBytes = size(m_grid) * sizeof(uint64_t);


	clSetKernelArg(m_gameKernel, 0, sizeof(cl_mem), &m_inBuffer);
	clSetKernelArg(m_gameKernel, 1, sizeof(cl_mem), &m_outBuffer);
	clSetKernelArg(m_gameKernel, 2, sizeof(int), &m_grid->height);
	clSetKernelArg(m_gameKernel, 3, sizeof(int), &m_grid->width);
	clSetKernelArg(m_gameKernel, 4, sizeof(int), &m_grid->species);

	clEnqueueNDRangeKernel(m_queue, m_gameKernel, 1, nullptr, &globalWorkSize, nullptr, 0, nullptr, nullptr);

	CPURender();

	clFinish(m_queue);
	clEnqueueReadBuffer(m_queue, m_outBuffer, CL_TRUE, 0, gridBytes, m_next->arr, 0, nullptr, nullptr);
}

void GameOfLife::SimpleRender()
{
	m_rendered_vertices.clear();

	float x_midpoint = (float)(m_grid->width+1) / 2.0;
	float y_midpoint = (float)(m_grid->height+1) / 2.0;
	for (int x = 0; x < m_grid->width; x++) {
		for (int y = 0; y < m_grid->height; y++) {
			uint64_t value = check(m_grid, x, y);
			if (value) {
				Vertex vertex;
				vertex.position[0] = float(x - x_midpoint) / x_midpoint + m_point_width_offset;
				vertex.position[1] = float(y - y_midpoint) / y_midpoint + m_point_height_offset;

				int species_index = __builtin_ctzll(value) / 4+1;
				for (int i=0; i < 4; i++) {
					vertex.color[i] = COLORS[species_index][i];
				}
				m_rendered_vertices.push_back(vertex);

			}

		}
	}

	glBufferData(GL_ARRAY_BUFFER, m_rendered_vertices.size() * sizeof(Vertex), m_rendered_vertices.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(GL_POINTS, 0, m_rendered_vertices.size());
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


void GameOfLife::step() {
	ParallelStep();
	swap();
}

void GameOfLife::swap() {
	std::swap(m_grid, m_next);
	std::swap(m_inBuffer, m_outBuffer);
}

void GameOfLife::render() {
	/*
	m_rendered_vertices.assign(m_vertices.begin(), m_vertices.end());
	glBufferData(GL_ARRAY_BUFFER, m_rendered_vertices.size() * sizeof(Vertex), m_rendered_vertices.data(), GL_DYNAMIC_DRAW);
	glDrawArrays(GL_POINTS, 0, m_rendered_vertices.size());
	*/
	glBufferData(GL_ARRAY_BUFFER, size(m_grid) * sizeof(Vertex), m_vertices, GL_DYNAMIC_DRAW);
	glDrawArrays(GL_POINTS, 0, size(m_grid));
}

