#ifndef COMPUTE_SHADER_H
#define COMPUTE_SHADER_H

#include "baseShader.h"

class ComputeShader : public BaseShader {
public:
    ComputeShader(const char* computePath) {
        // 1. Retrieve source code
        std::string cCode = readFile(computePath);
        const char* cShaderCode = cCode.c_str();

        // 2. Compile
        unsigned int compute;
        compute = glCreateShader(GL_COMPUTE_SHADER);
        glShaderSource(compute, 1, &cShaderCode, NULL);
        glCompileShader(compute);
        checkCompileErrors(compute, "COMPUTE");

        // 3. Program
        ID = glCreateProgram();
        glAttachShader(ID, compute);
        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");

        // 4. Cleanup
        glDeleteShader(compute);
    }
    
    // Helper specifically for Compute Shaders
    void dispatch(unsigned int x, unsigned int y, unsigned int z) {
        glDispatchCompute(x, y, z);
    }
    
    void memoryBarrier(GLbitfield barriers) {
        glMemoryBarrier(barriers);
    }
};

#endif // COMPUTE_SHADER_H