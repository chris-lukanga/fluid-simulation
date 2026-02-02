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

// Include your header files
// Make sure ComputeShader.h has setInt and setVec2 added as discussed!
#include "computeShader.h" 

#include "shader.h"
// ---------------------------------------------------------
// 2. Constants & Globals
// ---------------------------------------------------------
int SCR_WIDTH = 800;
int SCR_HEIGHT = 600;

float deltaTime = 0.0f;
float lastFrame = 0.0f;



float factor = 1.0f;
bool resendData = false; 

constexpr uint32_t MAX_PARTICLES = 10000;
constexpr uint32_t INITIAL_PARTICLES = 50;
constexpr float GRAVITY = 50.0f;

struct alignas(16) Particle {
    glm::vec4 pos_radius; // x,y,z, radius
    glm::vec4 velocity; 
    glm::vec4 color;
};

std::vector<Particle> particles;

// ---------------------------------------------------------
// 3. Helper Functions
// ---------------------------------------------------------
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
    newparticle.pos_radius = glm::vec4(x, y, radius, 1.0f);
    //newparticle.velocity = randomDirection2D() * 200.0f; // Speed 200
    newparticle.velocity = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f); // Start stationary
    newparticle.color = randomColour();
    
    particles.push_back(newparticle);
    resendData = true; // Flag to append this single particle
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height) {
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}

void processInput(GLFWwindow* window) {
    static bool mousePressed = false;
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        factor = glm::clamp(factor + 10.0f * deltaTime, 1.0f, 500.0f);
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        factor = glm::clamp(factor - 10.0f * deltaTime, 1.0f, 500.0f);

    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!mousePressed) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            float centerX = (float)xpos - (SCR_WIDTH / 2.0f);
            float centerY = (SCR_HEIGHT / 2.0f) - (float)ypos; 
            circle(centerX, centerY, 50.0f);
            mousePressed = true;
        }
    } else {
        mousePressed = false;
    }
}

// ---------------------------------------------------------
// 4. Main
// ---------------------------------------------------------
int main()
{
    srand(static_cast <unsigned> (time(0)));

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "Compute Shader Physics", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    // OpenGL State
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // Shaders
    Shader shader("shaders/vertex2D.vert", "shaders/fragment2D.frag");
    Shader bgShader("shaders/background.vert", "shaders/background.frag");
    ComputeShader computeShader("shaders/physics.comp");

    // Quad Geometry
    float quadVertices[] = { -0.5f,-0.5f, 0.5f,-0.5f, 0.5f,0.5f, -0.5f,-0.5f, 0.5f,0.5f, -0.5f,0.5f };
    unsigned int VAO, VBO, bgVAO, bgVBO;
    
    glGenVertexArrays(1, &VAO); glGenBuffers(1, &VBO);
    glBindVertexArray(VAO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glGenVertexArrays(1, &bgVAO); glGenBuffers(1, &bgVBO);
    glBindVertexArray(bgVAO); glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Initial Particles
    int initialCount = INITIAL_PARTICLES;
    for (int i = 0; i < initialCount; i++){
        circle(rand() % SCR_WIDTH - SCR_WIDTH / 2.0f, rand() % SCR_HEIGHT - SCR_HEIGHT / 2.0f, 250.0f);
    }

    // ------------------------------------------------------------
    // SSBO Setup 
    // ------------------------------------------------------------
    GLuint particlesSSBO;
    glGenBuffers(1, &particlesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, particlesSSBO);
    
    // CRITICAL: Allocate memory for MAX_PARTICLES upfront. 
    // This allows the buffer to exist permanently on the GPU.
    // We do NOT use NULL here; we upload the initial data immediately.
    // If particles.size() < MAX_PARTICLES, the rest is garbage/zero, which is fine as we don't draw it.
    
    // 1. Create a temporary vector filled with zeros to initialize the full GPU buffer cleanly
    std::vector<Particle> initialBuffer(MAX_PARTICLES);
    // 2. Copy our actual particles into it
    std::copy(particles.begin(), particles.end(), initialBuffer.begin());
    
    // 3. Upload the full MAX sized buffer
    glBufferData(GL_SHADER_STORAGE_BUFFER, MAX_PARTICLES * sizeof(Particle), initialBuffer.data(), GL_DYNAMIC_DRAW);
    
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, particlesSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0); // Unbind

    // ------------------------------------------------------------
    // Render Loop
    // ------------------------------------------------------------
    float fpsTimer = 0.0f;
    int fpsFrameCount = 0;

    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        fpsTimer += deltaTime;
        fpsFrameCount++;
        if (fpsTimer >= 1.0f) {
            std::cout << "FPS: " << fpsFrameCount << " | Particles: " << particles.size() << "\r";
            std::cout.flush();
            fpsTimer = 0.0f;
            fpsFrameCount = 0;
        }

        processInput(window);
        if (SCR_WIDTH == 0 || SCR_HEIGHT == 0) { glfwWaitEvents(); continue; }

        // ---------------------------------------------------------
        // 1. UPLOAD NEW DATA (Only if changed)
        // ---------------------------------------------------------
        if (resendData) {
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, particlesSSBO);
            
            // CRITICAL FIX: Do NOT call glBufferData again!
            // We only want to update the single new particle at the end of the list.
            
            // Index of the new particle is the last one in the vector
            int newIndex = static_cast<int>(particles.size()) - 1;
            
            // Calculate byte offset: index * sizeof(Particle)
            GLintptr offset = newIndex * sizeof(Particle);
            GLsizeiptr size = sizeof(Particle);
            
            // Upload only that specific struct
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, offset, size, &particles[newIndex]);
            
            glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
            resendData = false;
        }

        // ---------------------------------------------------------
        // 2. COMPUTE SHADER STEP
        // ---------------------------------------------------------
        computeShader.use();
        computeShader.setFloat("deltaTime", deltaTime);
        computeShader.setInt("numParticles", static_cast<int>(particles.size())); // Uses setInt now
        computeShader.setFloat("gravity", GRAVITY);
        computeShader.setVec2("dimensions", (float)SCR_WIDTH, (float)SCR_HEIGHT); // Uses setVec2 now
        
        // Dispatch groups
        // (total + 255) / 256 ensures we have enough groups to cover all particles
        computeShader.dispatch((static_cast<unsigned int>(particles.size()) + 255) / 256, 1, 1);
        
        // MEMORY BARRIER: Wait for compute to finish writing before drawing
        computeShader.memoryBarrier();

        // ---------------------------------------------------------
        // 3. RENDER STEP
        // ---------------------------------------------------------
        glClear(GL_COLOR_BUFFER_BIT);
        glm::mat4 projection = glm::ortho(-(float)SCR_WIDTH / 2.0f, (float)SCR_WIDTH / 2.0f, -(float)SCR_HEIGHT / 2.0f, (float)SCR_HEIGHT / 2.0f);

        // Background
        bgShader.use();
        bgShader.setInt("particlesCount", (int)particles.size());
        bgShader.setFloat("factor", factor);
        glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(SCR_WIDTH, SCR_HEIGHT, 1.0f));
        bgShader.setMat4("uModel", glm::value_ptr(model));
        bgShader.setMat4("uProjection", glm::value_ptr(projection));
        glBindVertexArray(bgVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // Particles
        shader.use();
        shader.setMat4("projection", glm::value_ptr(projection));
        shader.setFloat("scale", factor);
        glBindVertexArray(VAO);
        
        if (!particles.empty()) {
            glDrawArraysInstanced(GL_TRIANGLES, 0, 6, static_cast<GLsizei>(particles.size()));
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}