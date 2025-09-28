#include "shader.h"

Shader::Shader(const char* vertex_filename, const char* fragment_filename){

	std::filesystem::path vertexPath = std::filesystem::path("./shader") / std::filesystem::path(vertex_filename);
	std::filesystem::path fragmentPath = std::filesystem::path("./shader") / std::filesystem::path(fragment_filename);
	
	std::string vertex_code;
	std::string fragment_code;
	std::ifstream vertex_file;
	std::ifstream fragment_file;

	vertex_file.exceptions (std::ifstream::failbit | std::ifstream::badbit);
	fragment_file.exceptions (std::ifstream::failbit | std::ifstream::badbit);

	try {
		vertex_file.open(vertexPath);
		std::stringstream vertex_stream;
		vertex_stream << vertex_file.rdbuf();
		vertex_file.close();
		vertex_code   = vertex_stream.str();
	}
	catch(std::ifstream::failure e) {
		std::cout << "ERROR::SHADER::VERTEX::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}

	try {
		fragment_file.open(fragmentPath);
		std::stringstream fragment_stream;
		fragment_stream << fragment_file.rdbuf();		
		fragment_file.close();
		fragment_code = fragment_stream.str();		
	} catch(std::ifstream::failure e)
	{
		std::cout << "ERROR::SHADER::FRAGMENT::FILE_NOT_SUCCESFULLY_READ" << std::endl;
	}

	const char* vertex_shader_code = vertex_code.c_str();
	const char* fragment_shader_code = fragment_code.c_str();
	
	int success;
	char log[512];

	GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_shader_code, NULL);
	glCompileShader(vertex_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success){
		glGetShaderInfoLog(vertex_shader, 512, NULL, log);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILURE" << std::endl;
	}

	GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment_shader, 1, &fragment_shader_code, NULL);
	glCompileShader(fragment_shader);
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success){
		glGetShaderInfoLog(fragment_shader, 512, NULL, log);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILURE" << std::endl;
	}

	this->id = glCreateProgram();
	glAttachShader(this->id, vertex_shader);
	glAttachShader(this->id, fragment_shader);
	glLinkProgram(this->id);
	glGetProgramiv(this->id, GL_LINK_STATUS, &success);
	if (!success){
		glGetProgramInfoLog(this->id, 512, NULL, log);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILURE" << std::endl;
	}

	//Now that the shaders have been linked their objects can be deleted
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

}

Shader::~Shader(){
	glDeleteProgram(this->id);
}

void Shader::use(){
	glUseProgram(this->id);
}


GLint Shader::get(const std::string& name){
	return glGetUniformLocation(this->id, name.c_str());
}

void Shader::setm4f(const std::string& name, glm::mat4& mat) const{
	GLint matID = glGetUniformLocation(this->id, name.c_str());
	glUniformMatrix4fv(matID, 1, GL_FALSE, &mat[0][0]);


}
void Shader::set4f(const std::string& name, glm::vec4& vec) const {
	GLint vecID = glGetUniformLocation(this->id, name.c_str());
	glUniform4f(vecID, vec[0], vec[1], vec[2], vec[3]);
}
