#version 450 core
layout(location = 0) in vec2 aPos;

struct ParticleData {
    vec4 position_size;
    vec4 color;
};

layout(std430, binding = 2) readonly buffer ParticleBuffer {
    ParticleData particles[];
};

uniform mat4 u_View;
uniform mat4 u_Projection;

out vec4 ParticleColor;
out vec2 TexCoords;

void main()
{
    ParticleData p = particles[gl_InstanceID];
    vec3 centerPos = p.position_size.xyz;
    float size = p.position_size.w;

    // Extract the camera's right and up vectors from the view matrix
    // for billboarding. This makes the quad always face the camera.
    vec3 cameraRight = vec3(u_View[0][0], u_View[1][0], u_View[2][0]);
    vec3 cameraUp = vec3(u_View[0][1], u_View[1][1], u_View[2][1]);

    vec3 vertexPos = centerPos + cameraRight * aPos.x * size + cameraUp * aPos.y * size;

    gl_Position = u_Projection * u_View * vec4(vertexPos, 1.0);
    
    ParticleColor = p.color;
    TexCoords = aPos + vec2(0.5); // Convert from [-0.5, 0.5] to [0.0, 1.0]
}
