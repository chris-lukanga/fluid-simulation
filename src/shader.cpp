#include <shader.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <glm/glm.hpp>




std::string readShaderSource(const char* filePath){
    std::ifstream file(filePath, std::ios::binary);
    if(!file){
        throw std::runtime_error("Failed to open shader file");
    }
    std::ostringstream contents;
    contents << file.rdbuf();
    std::string source = contents.str();
    // Remove BOM if present
    if(source.size() >= 3 && 
       static_cast<unsigned char>(source[0]) == 0xEF && 
       static_cast<unsigned char>(source[1]) == 0xBB && 
       static_cast<unsigned char>(source[2]) == 0xBF){
        source = source.substr(3);
    }
    return source;
}

void checkShaderCompilation(unsigned int shader, std::string shaderType){
    int success;
    char infoLog[512];
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if(!success){
        glGetShaderInfoLog(shader, 512, nullptr, infoLog);
        std::cerr << "ERROR::"<<shaderType<< "SHADER::COMPILATION_FAILED\n" << infoLog << std::endl;
    }
}

void checkProgramLinking(unsigned int program){
    int success;
    char infoLog[512];
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if(!success){
        glGetProgramInfoLog(program, 512, nullptr, infoLog);
        std::cerr << "ERROR::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
    }
}


Shader::Shader(const char* vertexPath, const char* fragmentPath)
{
    // 1. Retrieve the vertex/fragment source code from filePath
    std::string vertexCode;
    std::string fragmentCode;
    
    // We wrap this in a try-catch because readShaderSource throws
    try {
        vertexCode = readShaderSource(vertexPath);
        fragmentCode = readShaderSource(fragmentPath);
    }
    catch (std::exception& e) {
        std::cerr << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ: " << e.what() << std::endl;
        exit(-1);
    }

    // Now it is safe to get the C pointers because the string objects 
    // (vertexCode, fragmentCode) exist for the rest of this function.
    const char* vShaderCode = vertexCode.c_str();
    const char* fShaderCode = fragmentCode.c_str();

    // 2. Compile shaders
    unsigned int vertex, fragment;
    
    // Vertex Shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, nullptr);
    glCompileShader(vertex);
    checkShaderCompilation(vertex, "VERTEX");
    
    // Fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, nullptr);
    glCompileShader(fragment);
    checkShaderCompilation(fragment, "FRAGMENT");
    
    // Shader Program
    ID = glCreateProgram();
    glAttachShader(ID, vertex);
    glAttachShader(ID, fragment);
    glLinkProgram(ID);
    checkProgramLinking(ID);
    
    // Delete shaders
    glDeleteShader(vertex);
    glDeleteShader(fragment);
}

void Shader::use()
{
    glUseProgram(ID);
}

void Shader::setBool(const std::string &name, bool value) const
{         
    glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
}

void Shader::setInt(const std::string &name, int value) const
{ 
    glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
}

void Shader::setFloat(const std::string &name, float value) const
{ 
    glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
}

void Shader::setVec3(const std::string &name, float x, float y, float z) const
{
    glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z);
}

void Shader::setMat4(const std::string &name, const float* mat) const
{
    glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, mat);
}



