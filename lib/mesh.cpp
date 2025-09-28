#include "mesh.h"

Mesh::Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices){
	this->vertices = vertices;
	this->indices = indices;

	setup_mesh();
}
Mesh::Mesh(){
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;


	
	vertices.emplace_back(0.5f, 0.5f, 0.0f);
	vertices.emplace_back(0.5f, -0.5f, 0.0f);
	vertices.emplace_back(-0.5f, -0.5f, 0.0f);
	vertices.emplace_back(-0.5f,  0.5f, 0.0f);
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(3);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(3);
	this->vertices = vertices;
	this->indices = indices;
	setup_mesh();
}

void Mesh::setup_mesh(){
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), &vertices[0], GL_STATIC_DRAW);
	//glBufferData(GL_ARRAY_BUFFER, sizeof(new_vertices), new_vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), &indices[0], GL_STATIC_DRAW);
	//glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(new_indices), new_indices, GL_STATIC_DRAW);


	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindVertexArray(0);

}

void test_error(std::string message){
	GLenum err;
	while ((err = glGetError()) != GL_NO_ERROR) {
		std::cerr << message << "\nError code:" << err << std::endl;
		exit(1);
	}
}

void Mesh::draw(){
	glBindVertexArray(VAO);
	test_error("Error after binding VAO");
	glDrawElements(GL_TRIANGLES, indices.size(), GL_UNSIGNED_INT, 0);
	//glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	glBindVertexArray(0);
}

Mesh::~Mesh(){
	glDeleteVertexArrays(1, &VAO);
	glDeleteBuffers(1, &VBO);
	glDeleteBuffers(1, &EBO);
}

std::ostream& operator<<(std::ostream& out, Mesh& mesh){
	out << "Vertices\n";
	for (Vertex v: mesh.vertices){
		out << "(" << v.position.x << ", " << v.position.y << ", " << v.position.z << "), ";
	}
	out << "Indices\n";
	for (unsigned int i: mesh.indices){
		out << i << " ";
	}
	return out;
}
