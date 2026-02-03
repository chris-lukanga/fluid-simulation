#version 430 core
out vec4 FragColor;

in vec2 WorldPos;

// --------------------------------------------------------
// SSBO Definition
// --------------------------------------------------------
struct Particle {
    vec4 pos_radius;
    vec4 velocity;
    vec4 color;
};

layout(std430, binding = 0) buffer particlesBuffer {
    Particle particless[];
};

// --------------------------------------------------------
// Uniforms
// --------------------------------------------------------
uniform int particlesCount;
uniform float gravityConstant;  // Same G as in compute shader
uniform float fieldScale;       // Visualization scaling factor

// --------------------------------------------------------
// Heatmap Color Function
// --------------------------------------------------------
// Maps field strength [0, 1] to a color gradient
vec3 heatmap(float t) {
    // Clamp to ensure we stay in [0,1]
    t = clamp(t, 0.0, 1.0);
    
    // 5-color gradient: Blue -> Cyan -> Green -> Yellow -> Red
    vec3 color;
    if (t < 0.25) {
        // Blue to Cyan
        color = mix(vec3(0.0, 0.0, 0.5), vec3(0.0, 0.5, 1.0), t / 0.25);
    }
    else if (t < 0.5) {
        // Cyan to Green
        color = mix(vec3(0.0, 0.5, 1.0), vec3(0.0, 1.0, 0.0), (t - 0.25) / 0.25);
    }
    else if (t < 0.75) {
        // Green to Yellow
        color = mix(vec3(0.0, 1.0, 0.0), vec3(1.0, 1.0, 0.0), (t - 0.5) / 0.25);
    }
    else {
        // Yellow to Red
        color = mix(vec3(1.0, 1.0, 0.0), vec3(1.0, 0.0, 0.0), (t - 0.75) / 0.25);
    }
    
    return color;
}

void main()
{
    // --------------------------------------------------------
    // Calculate Gravitational Field Strength
    // --------------------------------------------------------
    vec2 fieldAccel = vec2(0.0);
    
    for (int i = 0; i < particlesCount; i++)
    {
        vec2 toParticle = particless[i].pos_radius.xy - WorldPos;
        float distSq = dot(toParticle, toParticle);
        
        // Skip if too far (optimization)
        const float maxDist = 500.0; // World units
        if (distSq > maxDist * maxDist)
            continue;
        
        // Softening to prevent singularities
        const float softening = 10.0;
        float softenedDistSq = distSq + softening * softening;
        
        // Calculate mass from radius (using volume: mass ∝ r³)
        float radius = particless[i].pos_radius.w;
        float mass = radius * radius * radius;
        
        // Gravitational field strength: g = G * m / r²
        float fieldMagnitude = gravityConstant * mass / softenedDistSq;
        
        // Direction (normalized)
        float dist = sqrt(distSq);
        if (dist > 1e-6) {
            vec2 direction = toParticle / dist;
            fieldAccel += direction * fieldMagnitude;
        }
    }
    
    // --------------------------------------------------------
    // Visualize Field as Heatmap
    // --------------------------------------------------------
    float fieldStrength = length(fieldAccel);
    
    // Logarithmic scaling for better visualization
    // (gravity falls off quickly, so log makes it more visible)
    float logStrength = log(1.0 + fieldStrength * fieldScale) / log(2.0);
    
    // Map to color
    vec3 fieldColor = heatmap(logStrength);
    
    // Optional: Add directional lines (field flow visualization)
    // Uncomment below for flow lines
    /*
    if (fieldStrength > 0.01) {
        vec2 fieldDir = normalize(fieldAccel);
        float linePattern = fract(dot(WorldPos * 0.05, fieldDir));
        fieldColor += vec3(0.1) * smoothstep(0.45, 0.55, linePattern);
    }
    */
    
    // --------------------------------------------------------
    // Optional: Overlay Particle Glow
    // --------------------------------------------------------
    vec3 glowColor = vec3(0.0);
    for (int i = 0; i < particlesCount; i++)
    {
        vec2 toParticle = particless[i].pos_radius.xy - WorldPos;
        float distSq = dot(toParticle, toParticle);
        float radius = particless[i].pos_radius.w;
        
        // Particle glow (only very close)
        float glowRadius = radius * 2.0;
        if (distSq < glowRadius * glowRadius) {
            float intensity = 0.5 / (distSq + 1.0);
            glowColor += particless[i].color.rgb * intensity;
        }
    }
    
    // Combine field and glow
    vec3 finalColor = fieldColor * 0.3 + glowColor; // 30% field, rest is glow
    
    // Tone mapping
    finalColor = finalColor / (finalColor + vec3(1.0));
    
    FragColor = vec4(finalColor, 1.0);
}