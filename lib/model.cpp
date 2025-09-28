#include "model.h"

Model::Model(const char* filepath){
	std::filesystem::path assets("./assets");
	std::filesystem::path path= assets / std::filesystem::path(filepath);
	
	load_model(path);
}

Model::Model(){

	this->meshes.push_back(new Mesh());
}

void Model::draw(){
	for (unsigned int i=0; i < meshes.size(); ++i){
		meshes[i]->draw();
	}
}

void Model::load_model(std::filesystem::path filepath){
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filepath, aiProcess_Triangulate);
	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode){
		std::cout << "ERROR::ASSIMP::" << importer.GetErrorString() << std::endl;
	}
	dir = filepath.remove_filename();
	process_node(scene->mRootNode, scene);
}


void Model::process_node(aiNode* node, const aiScene* scene){
	for (int i=0; i < node->mNumMeshes; ++i){
		aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
		meshes.push_back(process_mesh(mesh, scene));
	}

	for (int i=0; i < node->mNumChildren; ++i){
		process_node(node->mChildren[i], scene);
	}
}

Mesh* Model::process_mesh(aiMesh* mesh, const aiScene* scene){
	std::vector<Vertex> vertices;
	std::vector<unsigned int> indices;

	for (int i=0; i < mesh->mNumVertices; ++i){
	aiVector3D aiVertex = mesh->mVertices[i];
	Vertex vertex(aiVertex.x, aiVertex.y, aiVertex.z);
	vertices.push_back(vertex);
	}
	for (int i=0; i < mesh->mNumFaces; ++i){
		aiFace face = mesh->mFaces[i];
		for (int j=0; j < face.mNumIndices; ++j){
			indices.push_back(face.mIndices[j]);
		}
	}

	return new Mesh(vertices, indices);
}

std::ostream& operator<<(std::ostream& out, Model& model){
	for (Mesh* mesh: model.meshes){
		out << mesh;
	}
	out << "\n";
	return out;
}

