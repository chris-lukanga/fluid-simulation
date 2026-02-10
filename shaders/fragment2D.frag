#version 430 core
out vec4 FragColor;

in vec2 LocalPos;
in vec4 particleColor;

uniform float time;

void main()
{
    // OPTIMIZATION: Use dot product to get distance squared.
    // This avoids the expensive sqrt() call found in length().
    float distSq = dot(LocalPos, LocalPos);

    // 1. SMOOTH CIRCLE (Optimized)
    // We must square our thresholds to match the squared distance.
    // Outer edge (Transparency 0): 0.5 * 0.5  = 0.25
    // Inner edge (Opacity 1):      0.48 * 0.48 = 0.2304
    // 
    // smoothstep returns 0.0 if distSq >= 0.25 and 1.0 if distSq <= 0.2304
    float alpha = smoothstep(0.25, 0.2304, distSq);

    // Early exit if transparent
    if (alpha < 0.01) discard;

   // FragColor = vec4(particleColor.rgb, alpha);
}