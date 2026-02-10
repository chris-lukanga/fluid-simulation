#version 430 core
out vec4 FragColor;

in vec2 WorldPos;

// --------------------------------------------------------
// Buffers & Uniforms
// --------------------------------------------------------
// We don't need the Particle buffer here anymore! 
// We only need the field data.

layout(std430, binding = 3) buffer Screen {
    vec2 fields[];
};

uniform float fieldScale;       
uniform vec2 dimensions;       

// --------------------------------------------------------
// Heatmap Color Function
// --------------------------------------------------------
vec3 heatmap(float t) {
    t = clamp(t, 0.0, 1.0);
    vec3 color;
    // (Your gradient logic is fine, keeping it condensed here)
    if (t < 0.25) color = mix(vec3(0.0,0.0,0.5), vec3(0.0,0.5,1.0), t/0.25);
    else if (t < 0.5) color = mix(vec3(0.0,0.5,1.0), vec3(0.0,1.0,0.0), (t-0.25)/0.25);
    else if (t < 0.75) color = mix(vec3(0.0,1.0,0.0), vec3(1.0,1.0,0.0), (t-0.5)/0.25);
    else color = mix(vec3(1.0,1.0,0.0), vec3(1.0,0.0,0.0), (t-0.75)/0.25);
    return color;
}

void main()
{
    // 1. Calculate Index
    int width = int(dimensions.x);
    int height = int(dimensions.y);
    int x = int(gl_FragCoord.x);
    int y = int(gl_FragCoord.y);
    
    // Safety Check: Don't read outside the buffer!
    if (x < 0 || x >= width || y < 0 || y >= height) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    int cellID = x + (y * width);

    // 2. Read Field
    // Assuming 'fields' was populated by a compute shader matching screen res
    vec2 fieldAccel = fields[cellID];
    float fieldStrength = length(fieldAccel);

    // 3. Colorize
    // Logarithmic scaling is perfect for gravity (1/r^2 falls off fast)
    float logStrength = log(1.0 + fieldStrength * fieldScale) / log(2.0);
    vec3 fieldColor = heatmap(logStrength);

    // 4. Final Output
    // No particle loop. Just the field.
    FragColor = vec4(fieldColor, 1.0);
}