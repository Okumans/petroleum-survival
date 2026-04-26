#version 450 core

in vec4 ParticleColor;
in vec2 TexCoords;

out vec4 FragColor;

void main()
{
    // Create a circular particle from the square quad
    // Distance from the center (0.5, 0.5)
    vec2 center = vec2(0.5, 0.5);
    float dist = distance(TexCoords, center);
    
    // Smooth edges (soft circle)
    float alpha = 1.0 - smoothstep(0.3, 0.5, dist);
    
    if (alpha <= 0.05) discard;

    // Output glowing color
    FragColor = vec4(ParticleColor.rgb, ParticleColor.a * alpha);
}
