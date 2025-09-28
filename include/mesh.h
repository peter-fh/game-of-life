#ifndef MESH_H
#define MESH_H

#include "glm/glm.hpp"
#include <string>
#include "shader.h"

struct Vertex {
    glm::vec3 position;
    Vertex(float x, float y, float z) : position(x, y, z) {}
};


class Mesh {
public:
    Mesh(std::vector<Vertex> vertices, std::vector<unsigned int> indices);
    Mesh();
    ~Mesh();

    std::vector<Vertex>         vertices;
    std::vector<unsigned int>   indices;

    //void Draw(Shader& shader);
    void draw();

    friend std::ostream& operator<<(std::ostream& out, Mesh& mesh);

private:
    unsigned int VAO, VBO, EBO;
    void setup_mesh();
};

#endif
