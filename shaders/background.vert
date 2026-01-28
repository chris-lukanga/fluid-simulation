#version 430 core
layout (location = 0) in vec2 aPos;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

out vec2 WorldPos;

void main()
{
    // Calculate the position on screen
    gl_Position = uProjection * uView * uModel * vec4(aPos, 0.0, 1.0);
    
    // Pass the actual world coordinate to the fragment shader
    // We multiply aPos by the scale (which is stored in uModel) implicitly 
    // because aPos is -0.5 to 0.5, but uModel scales it to screen size.
    WorldPos = (uModel * vec4(aPos, 0.0, 1.0)).xy; 
}