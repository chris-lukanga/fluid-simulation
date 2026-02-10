#ifndef GRAPHICS_SHADER_H
#define GRAPHICS_SHADER_H

#include "baseShader.h"

class GraphicsShader : public BaseShader {
public:
    // Constructor reads and builds the shader
    GraphicsShader(const char* vertexPath, const char* fragmentPath) {
        // 1. Retrieve source code
        std::string vCode = readFile(vertexPath);
        std::string fCode = readFile(fragmentPath);
        const char* vShaderCode = vCode.c_str();
        const char* fShaderCode = fCode.c_str();

        // 2. Compile shaders
        unsigned int vertex, fragment;

        // Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        checkCompileErrors(vertex, "VERTEX");

        // Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        checkCompileErrors(fragment, "FRAGMENT");

        // 3. Shader Program
        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // 4. Delete helpers
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }
};

#endif // GRAPHICS_SHADER_H