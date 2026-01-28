#version 430 core
out vec4 FragColor;

in vec2 LocalPos;
uniform vec3 color;

void main()
{
    // Calculate distance from center (0,0)
    float dist = length(LocalPos);

    // 1. HARD CIRCLE (Simple way)
    // if (dist > 0.5)
    //     discard;
    // FragColor = vec4(color, 1.0);

    // 2. SMOOTH CIRCLE (Better way, Anti-aliased)
    // We use smoothstep to fade the edges slightly so they aren't jagged
    // 0.5 is the edge, 0.01 is the "blur" amount
    float alpha = smoothstep(0.5, 0.48, dist);

    // If fully transparent, don't draw
    if (alpha < 0.01) discard;

    FragColor = vec4(color, alpha);
}