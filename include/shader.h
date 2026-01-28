#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h> // include glad to get all the required OpenGL headers
#include <GLFW/glfw3.h>
  
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
  
//function to check when a window is resized
void framebuffer_size_callback(GLFWwindow* window, int width, int height);

//function to check for program linking errors
void checkProgramLinking(unsigned int program);

//function to read shader source code from a file and removing any BOM if present
std::string readShaderSource(const char* filePath);

//function to check for shader compilation errors
void checkShaderCompilation(unsigned int shader, std::string shaderType);


class Shader
{
public:
    // the program ID
    unsigned int ID;
  
    // constructor reads and builds the shader
    Shader(const char* vertexPath, const char* fragmentPath);
    // use/activate the shader
    void use();
    // utility uniform functions
    void setBool(const std::string &name, bool value) const;  
    void setInt(const std::string &name, int value) const;   
    void setFloat(const std::string &name, float value) const;
    void setVec3(const std::string &name, float x, float y, float z) const;
    void setMat4(const std::string &name, const float* mat) const;
};
  
#endif // SHADER_H