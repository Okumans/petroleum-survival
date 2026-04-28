#version 450 core
layout(location = 0) in vec2 aPos;

struct ParticleData {
    vec4 position_size;
    vec4 direction_stretch;
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
    vec3 particleDirection = normalize(p.direction_stretch.xyz);
    float stretch = max(1.0, p.direction_stretch.w);

    // Extract the camera's right and up vectors from the view matrix
    // for billboarding. This makes the quad always face the camera.
    vec3 cameraRight = vec3(u_View[0][0], u_View[1][0], u_View[2][0]);
    vec3 cameraUp = vec3(u_View[0][1], u_View[1][1], u_View[2][1]);
    vec3 cameraForward = normalize(cross(cameraRight, cameraUp));

    vec2 projectedDirection = vec2(dot(particleDirection, cameraRight),
                                   dot(particleDirection, cameraUp));
    vec3 tangent = cameraUp;
    if (length(projectedDirection) > 0.001) {
        tangent = normalize(cameraRight * projectedDirection.x +
                            cameraUp * projectedDirection.y);
    }
    vec3 bitangent = normalize(cross(cameraForward, tangent));

    vec3 vertexPos = centerPos + tangent * aPos.x * size * stretch + bitangent * aPos.y * size;

    gl_Position = u_Projection * u_View * vec4(vertexPos, 1.0);
    
    ParticleColor = p.color;
    TexCoords = aPos + vec2(0.5); // Convert from [-0.5, 0.5] to [0.0, 1.0]
}
