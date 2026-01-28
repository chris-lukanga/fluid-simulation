#version 430 core
out vec4 FragColor;

in vec2 WorldPos;

// --------------------------------------------------------
// 1. SSBO Definition
// --------------------------------------------------------
// This must match the 'pointLight' struct in C++ exactly.
// vec4 = 16 bytes. The struct is 32 bytes total.
// layout(std430) ensures tight packing similar to C++.
struct PointLight {
    vec4 pos_radius;
    vec4 color;
};

layout(std430, binding = 0) buffer LightBuffer {
    PointLight lights[]; // Dynamic array
};

// --------------------------------------------------------
// 2. Uniforms
// --------------------------------------------------------
uniform int lightCount;
uniform float factor; // Controls the "glow" strength

void main()
{
    vec3 finalColor = vec3(0.0);

    for (int i = 0; i < lightCount; i++)
    {
        vec2 toLight = lights[i].pos_radius.xy - WorldPos;
        float distSq = dot(toLight, toLight);
        float radius = lights[i].pos_radius.z;

        // 🚀 HARD CULL
        if (distSq > radius * radius*factor*factor)
            continue;

        float intensity = factor / (distSq + 1.0);
        finalColor += lights[i].color.rgb * intensity;
    }

    finalColor = finalColor / (finalColor + vec3(1.0));
    FragColor = vec4(finalColor, 1.0);
    
}