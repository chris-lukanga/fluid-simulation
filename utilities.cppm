module;

#include <cstdlib>
#include <ctime>
#include <iostream>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

export module utilities;

export glm::vec2 randomDirection2D() {
    float angle = static_cast<float>(rand()) / static_cast<float>(RAND_MAX) * 2.0f * 3.14159265f;
    return glm::vec2(cos(angle), sin(angle));
}

export void sayHello() {
    std::cout << "Hello from utilities module!" << std::endl;
}