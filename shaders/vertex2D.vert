#version 430 core
layout (location = 0) in vec2 aPos;

uniform mat4 model;
uniform mat4 projection;

// Output to fragment shader
out vec2 LocalPos;

void main()
{
    // Pass the local position (-0.5 to 0.5) to the fragment shader
    LocalPos = aPos;
    
    gl_Position = projection * model * vec4(aPos, 0.0, 1.0);
}