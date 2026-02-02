#version 430 core

layout (location = 0) in vec2 aPos;

struct Particle {
    vec4 pos_radius; // xy = position, w = radius
    vec4 velocity;
    vec4 color;
};

layout(std430, binding = 0) buffer particlesBuffer {
    Particle particles[];
};

uniform float scale;
uniform mat4 projection;

out vec2 LocalPos;
out vec4 particleColor;

void main()
{
    Particle particle = particles[gl_InstanceID];

    // Per-instance scale
    float r = particle.pos_radius.w * scale;

    // Scale quad and translate to particles position
    vec2 worldPos = aPos * r + particle.pos_radius.xy;

    LocalPos   = aPos;        // stays in -0.5 .. 0.5
    particleColor = particle.color; // pass to fragment shader

    gl_Position = projection * vec4(worldPos, 0.0, 1.0);
}
