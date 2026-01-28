#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include <shader.h> 
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>


// Screen size
int SCR_WIDTH  = 800;
int SCR_HEIGHT = 600;

// Time variables
float deltaTime = 0.0f;
float lastFrame = 0.0f;

float factor = 1.0f;

// ---------------------------------------------------------
// Global Vectors (Must be vectors to grow dynamically)
// ---------------------------------------------------------
struct pointLight {
    glm::vec4 pos_radius;
    glm::vec4 color;
};

// We rely on .size() instead of a manual integer counter to be safe
std::vector<pointLight> lights;
std::vector<glm::vec2> quadOffsets;
std::vector<glm::vec3> quadColors;

// Forward declaration
void circle(float x, float y, float radius);

// Resize callback
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
    SCR_WIDTH = width;
    SCR_HEIGHT = height;
}

// Input processing
bool mousePressed = false; // Debounce variable
void processInput(GLFWwindow* window)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
        factor = glm::clamp(factor + 10.0f*deltaTime, 1.0f, 500.0f);
    
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
        factor = glm::clamp(factor - 10.0f*deltaTime, 1.0f, 500.0f);

    // MOUSE CLICK TO ADD CIRCLE
    if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        if (!mousePressed) {
            double xpos, ypos;
            glfwGetCursorPos(window, &xpos, &ypos);
            // Convert GLFW coordinates (Top-Left 0,0) to Center 0,0
            float centerX = (float)xpos - (SCR_WIDTH / 2.0f);
            float centerY = (SCR_HEIGHT / 2.0f) - (float)ypos; // Invert Y
            circle(centerX, centerY, 50.0f);
            mousePressed = true;
        }
    } else {
        mousePressed = false;
    }
}

int main()
{

    srand(static_cast <unsigned> (time(0)));

    // ------------------------------------------------------------
    // 1. GLFW + OpenGL setup
    // ------------------------------------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4); // SSBOs require 4.3+, usually 4.6 is safe
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "OpenGL Dynamic SSBO", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "Failed to create window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cerr << "Failed to init GLAD\n";
        return -1;
    }

    // ------------------------------------------------------------
    // 2. OpenGL state
    // ------------------------------------------------------------
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

    // ------------------------------------------------------------
    // 3. Shaders
    // ------------------------------------------------------------
    Shader shader("shaders/vertex2D.vert", "shaders/fragment2D.frag");
    Shader bgShader("shaders/background.vert", "shaders/background.frag");

    // ------------------------------------------------------------
    // 4. Quad geometry
    // ------------------------------------------------------------
    float quadVertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,

        -0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f
    };

    unsigned int VAO, VBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    unsigned int bgVAO, bgVBO;
    glGenVertexArrays(1, &bgVAO);
    glGenBuffers(1, &bgVBO);
    glBindVertexArray(bgVAO);
    glBindBuffer(GL_ARRAY_BUFFER, bgVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // ------------------------------------------------------------
    // 5. Initial Random Lights
    // ------------------------------------------------------------
    int initialCount = 500; 
    for (int i = 0; i < initialCount; i++){
        float x = rand() % (SCR_WIDTH) - SCR_WIDTH / 2.0f;
        float y = rand() % (SCR_HEIGHT) - SCR_HEIGHT / 2.0f;
        circle(x, y, 250.0f); // Use our new function to init
    }

    // ------------------------------------------------------------
    // 6. SSBO Setup
    // ------------------------------------------------------------
    GLuint lightSSBO;
    glGenBuffers(1, &lightSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
    // Initial data upload
    glBufferData(GL_SHADER_STORAGE_BUFFER, lights.size() * sizeof(pointLight), lights.data(), GL_DYNAMIC_DRAW);
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, lightSSBO);
    glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

    // ------------------------------------------------------------
    // 7. Render loop
    // ------------------------------------------------------------
    while (!glfwWindowShouldClose(window))
    {
        float currentFrame = (float)glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        std::cout << "Frame rate: " << 1.0f / deltaTime << " FPS\r"<< std::endl;
        
        processInput(window);

        // Resize Logic
        if (SCR_WIDTH == 0 || SCR_HEIGHT == 0) { glfwWaitEvents(); continue; }
        
        glm::mat4 projection = glm::ortho(
            -(float)SCR_WIDTH / 2.0f, (float)SCR_WIDTH / 2.0f,
            -(float)SCR_HEIGHT / 2.0f, (float)SCR_HEIGHT / 2.0f
        );

        glClear(GL_COLOR_BUFFER_BIT);
        float time = (float)glfwGetTime();

        // ---------------------------------------------------------
        // UPDATE LIGHT POSITIONS (CPU SIDE)
        // ---------------------------------------------------------
        for (size_t i = 0; i < lights.size(); i++)
        {
            // Simple animation logic
            // lights[i].position = glm::vec4(quadOffsets[i], 0.0f, 1.0f);
            
            // Example color pulse
            lights[i].color = glm::vec4(
                (sin(time + i) + 1.0f) / 2.0f,
                (cos(time + i) + 1.0f) / 2.0f,
                (sin(time * 0.5f + i) + 1.0f) / 2.0f,
                1.0f
            );
        }

        // ---------------------------------------------------------
        // UPLOAD TO GPU (SSBO)
        // ---------------------------------------------------------
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, lightSSBO);
        // CRITICAL FIX: Use glBufferData to re-allocate memory for the new size
        glBufferData(GL_SHADER_STORAGE_BUFFER, 
                     lights.size() * sizeof(pointLight), 
                     lights.data(), 
                     GL_DYNAMIC_DRAW);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

        // ---------------------------------------------------------
        // RENDER BACKGROUND
        // ---------------------------------------------------------
        bgShader.use();
        bgShader.setInt("lightCount", (int)lights.size());
        bgShader.setFloat("factor", factor);
        
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::scale(model, glm::vec3(SCR_WIDTH, SCR_HEIGHT, 1.0f));
        
        bgShader.setMat4("uModel", glm::value_ptr(model));
        bgShader.setMat4("uProjection", glm::value_ptr(projection));
        bgShader.setMat4("uView", glm::value_ptr(glm::mat4(1.0f)));
        
        glBindVertexArray(bgVAO);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        // ---------------------------------------------------------
        // DRAW THE QUADS
        // ---------------------------------------------------------
        shader.use();
        shader.setMat4("projection", glm::value_ptr(projection));
        glBindVertexArray(VAO);

        for (size_t i = 0; i < lights.size(); i++)
        {
            glm::mat4 quadModel = glm::mat4(1.0f);
            quadModel = glm::translate(quadModel, glm::vec3(lights[i].pos_radius.x, lights[i].pos_radius.y, 0.0f)); 
            quadModel = glm::scale(quadModel, glm::vec3(2 * factor, 2 * factor, 1.0f));

            shader.setMat4("model", glm::value_ptr(quadModel));
            shader.setVec3("color", quadColors[i].x, quadColors[i].y, quadColors[i].z);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// ---------------------------------------------------------
// Function Definition
// ---------------------------------------------------------
void circle(float x, float y, float radius) {
    // 1. Add to main Light vector (GPU Data)
    pointLight newLight;
    newLight.pos_radius = glm::vec4(x, y, radius, 1.0f);
    newLight.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    lights.push_back(newLight);

    // 2. Add to Animation Helper vectors
    quadOffsets.push_back(glm::vec2(x, y));

    // 3. Add to Color vector
    quadColors.push_back(glm::vec3(
        static_cast<float>(rand()) / static_cast<float>(RAND_MAX),
        static_cast<float>(rand()) / static_cast<float>(RAND_MAX),
        static_cast<float>(rand()) / static_cast<float>(RAND_MAX)
    ));
}