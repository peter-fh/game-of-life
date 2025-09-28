#ifndef SHADER_H
#define SHADER_H

#include <GL/glew.h> // include glad to get all the required OpenGL headers
  
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include "glm/glm.hpp"
  

class Shader
{
public:
    unsigned int id;
  
    Shader(const char* vertexPath, const char* fragmentPath);
    ~Shader();
    void use();

    GLint get(const std::string& name);
    void setm4f(const std::string& name, glm::mat4& mat) const;
    void set4f(const std::string& name, glm::vec4& vec) const;

};
  
#endif
