#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <algorithm>
#include <string>
#include <fstream>
#include <sstream>
#include <iomanip>

#include "computeShader.h" 
#include "shader.h" // Assuming you renamed shader.h to GraphicsShader.h as discussed, or keep it as "shader.h"



// ---------------------------------------------------------
// 1. Constants & Globals
// ---------------------------------------------------------
int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;

float deltaTime = 0.0f;
float lastFrame = 0.0f;

float radius = 1.0f;
bool resendData = false; 
bool pressed = false;

constexpr uint32_t MAX_PARTICLES = 10000;
constexpr uint32_t INITIAL_PARTICLES = 500;
constexpr float GRAVITY = 0.0f;       // Global downward gravity (if needed)
float GRAVITY_CONSTANT = 25.0f; // Interaction strength

GLFWwindow* window;

struct alignas(16) Particle {
    glm::vec4 pos_radius; // x,y,z, radius
    glm::vec4 velocity; 
    glm::vec4 color;
};

// Global Vectors
std::vector<Particle> particles;
std::vector<glm::vec2> fields;

// OpenGL IDs
unsigned int VAO, VBO, bgVAO, bgVBO;
GLuint particlesSSBO[2];
GLuint fieldSSBO;

// ---------------------------------------------------------
// 2. Helper Declarations
// ---------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height);
void processInput(GLFWwindow* window);
void initWindow();
void initGeometry();
void initSSBOs();
glm::vec4 randomColour();
glm::vec4 randomDirection2D();
void circle(float x, float y, float r);
void dumpParticlesToFile(GLuint particlesSSBO, size_t particleCount, const std::string& filename);
void initParticles();

// ---------------------------------------------------------
// 3. Main
// ---------------------------------------------------------
int main()
{
    srand(static_cast <unsigned> (time(0)));

    initWindow();
    
    // --- Shaders ---
    // Make sure your classes are set up to handle these paths
    GraphicsShader shader("shaders/vertex2D.vert", "shaders/fragment2D.frag");
    GraphicsShader bgShader("shaders/background.vert", "shaders/background.frag");
    
    // Create distinct objects for distinct tasks
    ComputeShader gravityShader("shaders/gravity.comp"); // Calculates field
    ComputeShader physicsShader("shaders/physics.comp"); // Moves particles

    // --- Data Setup ---
    initGeometry();

    initParticles();

    // Init Field (Grid) - Initialize to 0
    fields.resize(SCR_WIDTH * SCR_HEIGHT, glm::vec2(0.0f));

    initSSBOs();

    // --- Ping-Pong State ---
    int readIndex = 0;  // Frame N
    int writeIndex = 1; // Frame N+1

    // --- Render Loop ---
    float fpsTimer = 0.0f;
    int fpsFrameCount = 0;

    while (!glfwWindowShouldClose(window))
    {
        // Time Logic
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // FPS Counter
        fpsTimer += deltaTime;
        fpsFrameCount++;
        if (fpsTimer >= 1.0f) {
            std::cout << "FPS: " << fpsFrameCount << " | Particles: " << particles.size() << " | Gravity Constant: " << GRAVITY_CONSTANT << "\r";
            std::cout.flush();
            fpsTimer = 0.0f;
            fpsFrameCount = 0;
        }

        processInput(window);
        if (SCR_WIDTH == 0 || SCR_HEIGHT == 0) { glfwWaitEvents(); continue; }

        // ---------------------------------------------------------
        // 1. DATA UPLOAD (Only if new particles added)
        // ---------------------------------------------------------
        if (resendData) {
            int newIndex = static_cast<int>(particles.size()) - 1;
            GLintptr offset = newIndex * sizeof(Particle);
            
            // Upload to BOTH buffers to prevent flickering
            for (int i = 0; i < 2; i++) {
                glBindBuffer(GL_SHADER_STORAGE_BUFFER, particlesSSBO[i]);
                glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, sizeof(Particle), &particles[newIndex]);
            }
            resendData = false;
        }

        // ---------------------------------------------------------
        // 2. BIND BUFFERS (The Swap Logic)
        // ---------------------------------------------------------
        // Binding 0 = Input (Read Old Frame)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particlesSSBO[readIndex]);
        // Binding 1 = Output (Write New Frame)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, particlesSSBO[writeIndex]);
        // Binding 3 = Field Data (Read/Write)
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, fieldSSBO);


        // ---------------------------------------------------------
        // 3. COMPUTE PASS 1: Gravity Field
        // ---------------------------------------------------------
        // This shader runs for every pixel (grid cell) to calculate the field
        gravityShader.use();
        gravityShader.setVec2("dimensions", (float)SCR_WIDTH, (float)SCR_HEIGHT);
        gravityShader.setInt("numParticles", (int)particles.size());
        gravityShader.setFloat("gravityConstant", GRAVITY_CONSTANT);
        gravityShader.setInt("numFields", (int)fields.size());

        // Dispatch based on GRID SIZE (Width * Height)
        // Group size is usually 256 in X.
        unsigned int totalPixels = (unsigned int)fields.size();
        gravityShader.dispatch((totalPixels + 255) / 256, 1, 1);
        
        // Wait for field calculations to finish before physics uses them
        glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

        // ---------------------------------------------------------
        // 4. COMPUTE PASS 2: Particle Physics
        // ---------------------------------------------------------
        // This shader runs for every particle to move it
        physicsShader.use();
        
        physicsShader.setInt("numParticles", (int)particles.size());
        physicsShader.setFloat("gravity", GRAVITY);
        physicsShader.setVec2("dimensions", (float)SCR_WIDTH, (float)SCR_HEIGHT);
        physicsShader.setInt("numFields", (int)fields.size());
        physicsShader.setFloat("gravityConstant", GRAVITY_CONSTANT);

        unsigned int totalParticles = (unsigned int)particles.size();
        //substeps
        int numSubsteps = 4;
        for (int i = 0; i < numSubsteps; i++) {
            physicsShader.setFloat("deltaTime", deltaTime / (float)numSubsteps);
            physicsShader.dispatch((totalParticles + 255) / 256, 1, 1);
            glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
            std::swap(readIndex, writeIndex);
        }


        // ---------------------------------------------------------
        // 5. RENDER STEP
        // ---------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT);
        glm::mat4 projection = glm::ortho(-(float)SCR_WIDTH / 2.0f, (float)SCR_WIDTH / 2.0f, -(float)SCR_HEIGHT / 2.0f, (float)SCR_HEIGHT / 2.0f);

        // A. Draw Background (Heatmap)
        bgShader.use();
        bgShader.setVec2("dimensions", (float)SCR_WIDTH, (float)SCR_HEIGHT);
        bgShader.setFloat("fieldScale", 0.01f); // Adjust this to make heatmap brighter/dimmer
        
        glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(SCR_WIDTH, SCR_HEIGHT, 1.0f));
        bgShader.setMat4("uModel", model);
        bgShader.setMat4("uProjection", projection);
        
        glBindVertexArray(bgVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // B. Draw Particles
        shader.use();
        shader.setMat4("projection", projection);
        
        // CRITICAL: Bind the *writeIndex* buffer to Binding 0 for the vertex shader
        // The vertex shader reads from Binding 0 to get the *latest* positions.
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particlesSSBO[writeIndex]);
        
        glBindVertexArray(VAO);
        if (!particles.empty()) {
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(particles.size()));
        }

        // ---------------------------------------------------------
        // 6. SWAP & SNAPSHOT
        // ---------------------------------------------------------
        
        // Handle Screenshot (Dump)
        if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS) {
            if (!pressed) {
                // Dump the buffer we just wrote to
                dumpParticlesToFile(particlesSSBO[writeIndex], particles.size(), "particle_dump.csv");
                pressed = true;
            }
        } else {
            pressed = false;
        }

        // Swap indices for next frame
        // Read becomes Write, Write becomes Read
        std::swap(readIndex, writeIndex);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    {// Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteVertexArrays(1, &bgVAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &bgVBO);
    glDeleteBuffers(2, particlesSSBO);
    glDeleteBuffers(1, &fieldSSBO);
    }

    glfwTerminate();
    return 0;
}

// ---------------------------------------------------------
// Helper Implementations
// ---------------------------------------------------------

void initWindow(){
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Compute Shader Physics", nullptr, nullptr);
    if (!window) { glfwTerminate(); exit(-1); }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) exit(-1);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
}

void initGeometry() {
    float quadVertices[] = { -0.5f,-0.5f, 0.5f,-0.5f, 0.5f,0.5f, -0.5f,-0.5f, 0.5f,0.5f, -0.5f,0.5f };
    
    // Particle Quad
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Background Quad
    glGenVertexArrays(1, &bgVAO); glGenBuffers(1, &bgVBO);
    glBindVertexArray(bgVAO); glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
}

void initSSBOs() {
    // 1. Particles (Double Buffered)
    glGenBuffers(2, particlesSSBO);
    std::vector<Particle> initialBuffer(MAX_PARTICLES);
    // Copy existing particles into the zeroed buffer
    if(!particles.empty()) {
        std::copy(particles.begin(), particles.end(), initialBuffer.begin());
    }

    for (int i = 0; i < 2; i++) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, particlesSSBO[i]);
        // Allocate full MAX_PARTICLES size on GPU
        glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_PARTICLES * sizeof(Particle), initialBuffer.data(), GL_DYNAMIC_DRAW);
    }

    // 2. Field (Single Buffered)
    glGenBuffers(1, &fieldSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, fieldSSBO);
    // Use fields.size() which is width * height
    glBufferData(GL_SHADER_STORAGE_BUFFER, fields.size() * sizeof(glm::vec2), fields.data(), GL_DYNAMIC_DRAW);
    
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind
}

glm::vec4 randomColour() {
    return glm::vec4((rand() % 100) / 100.0f, (rand() % 100) / 100.0f, (rand() % 100) / 100.0f, 1.0f);
}

glm::vec4 randomDirection2D(){
    float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.14159265f;
    return glm::vec4(cos(angle), sin(angle), 0.0f, 0.0f);
}

void circle(float x, float y, float radius) {
    if (particles.size() >= MAX_PARTICLES) return;
    
    Particle newparticle;
    newparticle.pos_radius = glm::vec4(x, y, 1.0f, radius);
    newparticle.velocity = randomDirection2D() * 0.0f; 
    newparticle.color = glm::vec4(0.2f, 0.6f, 1.0f, 1.0f); // Light blue
    
    particles.push_back(newparticle);
    resendData = true; 
}

void processInput(GLFWwindow* window) {
    static bool mousePressed = false;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        GRAVITY_CONSTANT = glm::clamp(GRAVITY_CONSTANT + 10.0f * deltaTime, 0.5f, 500.0f);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        GRAVITY_CONSTANT = glm::clamp(GRAVITY_CONSTANT - 10.0f * deltaTime, 0.5f, 500.0f);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!mousePressed) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            float centerX = (float)xpos - (SCR_WIDTH / 2.0f);
            float centerY = (SCR_HEIGHT / 2.0f) - (float)ypos; 
            circle(centerX, centerY, 10.0f);
            mousePressed = true;
        }
    } else {
        mousePressed = false;
    }
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
    // Note: If you resize the window, you should technically resize the 'fields' vector 
    // and re-allocate the fieldSSBO, otherwise the grid logic will break.
}

void dumpParticlesToFile(GLuint particlesSSBO, size_t particleCount, const std::string& filename) {
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particlesSSBO);
    Particle* gpuParticles = static_cast<Particle*>(
        glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY)
    );

    if (!gpuParticles) {
        std::cerr << "ERROR: Failed to map particle SSBO\n";
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return;
    }

    std::ofstream out(filename);
    if (!out.is_open()) {
        glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
        return;
    }

    out << std::fixed << std::setprecision(4);
    out << "index,x,y,z,r,vx,vy,vz,vw,cr,cg,cb,ca\n";

    for (size_t i = 0; i < particleCount; ++i) {
        const Particle& p = gpuParticles[i];
        out << i << ","
            << p.pos_radius.x << "," << p.pos_radius.y << "," << p.pos_radius.z << "," << p.pos_radius.w << ","
            << p.velocity.x   << "," << p.velocity.y   << "," << p.velocity.z   << "," << p.velocity.w   << ","
            << p.color.r      << "," << p.color.g      << "," << p.color.b      << "," << p.color.a
            << "\n";
    }

    out.close();
    glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    std::cout << "Saved " << particleCount << " particles to " << filename << "\n";
}

void initParticles() {
    //place particles in grid format

    int particlesPerRow = static_cast<int>(sqrt(INITIAL_PARTICLES));
    int particlesPerCol = (INITIAL_PARTICLES + particlesPerRow - 1) / particlesPerRow;
    float spacing = 20.0f; // Space between particles
    for(int i = 0; i < particlesPerCol; i++) {
        for(int j = 0; j < particlesPerRow; j++) {
            if(particles.size() >= INITIAL_PARTICLES) break;
            float x = (j - particlesPerRow / 2) * spacing;
            float y = (i - particlesPerCol / 2) * spacing;
            circle(x, y, 10.0f);
        }
    }
}

