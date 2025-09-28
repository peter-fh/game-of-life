#ifndef MODEL_H
#define MODEL_H
#include "mesh.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

class Model {
public:
    Model(const char* path);
    Model();
    void draw();
private:
    std::vector<Mesh*> meshes;
    std::string dir;

    void load_model(std::filesystem::path);
    void process_node(aiNode* node, const aiScene* scene);
    Mesh* process_mesh(aiMesh* mesh, const aiScene* scene);

    friend std::ostream& operator<<(std::ostream& out, Model& model);

};

#endif
