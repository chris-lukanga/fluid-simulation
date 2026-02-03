#ifndef COMPUTESHADER_H
#define COMPUTESHADER_H

#include <string>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <iostream>

#include <glad/glad.h>

class ComputeShader {
public:
    explicit ComputeShader(const std::string& filePath) {
        m_program = createProgram(filePath);
    }

    ~ComputeShader() {
        if (m_program)
            glDeleteProgram(m_program);
    }

    // Not copyable
    ComputeShader(const ComputeShader&) = delete;
    ComputeShader& operator=(const ComputeShader&) = delete;

    // Movable
    ComputeShader(ComputeShader&& other) noexcept {
        m_program = other.m_program;
        other.m_program = 0;
    }

    ComputeShader& operator=(ComputeShader&& other) noexcept {
        if (this != &other) {
            glDeleteProgram(m_program);
            m_program = other.m_program;
            other.m_program = 0;
        }
        return *this;
    }

    void use() const {
        glUseProgram(m_program);
    }

    void dispatch(
        unsigned int groupsX,
        unsigned int groupsY = 1,
        unsigned int groupsZ = 1
    ) const {
        glDispatchCompute(groupsX, groupsY, groupsZ);
    }

    void memoryBarrier() const {
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Uniform helpers
    void setFloat(const char* name, float value) const {
        GLint loc = glGetUniformLocation(m_program, name);
        if (loc != -1)
            glUniform1f(loc, value);
    }

    void setUInt(const char* name, unsigned int value) const {
        GLint loc = glGetUniformLocation(m_program, name);
        if (loc != -1)
            glUniform1ui(loc, value);
    }

    GLuint id() const { return m_program; }

    void setVec2(const char* name, float x, float y) const {
        GLint loc = glGetUniformLocation(m_program, name);
        if (loc != -1)
            glUniform2f(loc, x, y);
    }

    void setInt(const char* name, int value) const {
        GLint loc = glGetUniformLocation(m_program, name);
        if (loc != -1)
            glUniform1i(loc, value);
    }
    void setBool(const char* name, bool value) const {
        GLint loc = glGetUniformLocation(m_program, name);
        if (loc != -1)
            glUniform1i(loc, static_cast<int>(value));
    }

private:
    GLuint m_program = 0;

    static std::string loadFile(const std::string& path) {
        std::ifstream file(path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open shader file: " + path);
        }
        std::ostringstream contents;
        contents << file.rdbuf();
        std::string source = contents.str();
        
        // Remove BOM if present
        if (source.size() >= 3 &&
            static_cast<unsigned char>(source[0]) == 0xEF &&
            static_cast<unsigned char>(source[1]) == 0xBB &&
            static_cast<unsigned char>(source[2]) == 0xBF) {
            source = source.substr(3);
        }
        return source;
    }

    static GLuint createProgram(const std::string& path) {
        std::string source = loadFile(path);
        const char* src = source.c_str();

        GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(shader, 1, &src, nullptr);
        glCompileShader(shader);

        GLint success = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
        if (!success) {
            char log[1024];
            glGetShaderInfoLog(shader, 1024, nullptr, log);
            glDeleteShader(shader);
            throw std::runtime_error("Compute shader compile error (" + path + "):\n" + std::string(log));
        }

        GLuint program = glCreateProgram();
        glAttachShader(program, shader);
        glLinkProgram(program);

        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            char log[1024];
            glGetProgramInfoLog(program, 1024, nullptr, log);
            glDeleteShader(shader);
            glDeleteProgram(program);
            throw std::runtime_error("Compute shader link error:\n" + std::string(log));
        }

        glDeleteShader(shader);
        return program;
    }
};

#endif // COMPUTESHADER_H