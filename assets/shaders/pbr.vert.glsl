#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoords;
layout(location = 3) in vec3 aTangent;
layout(location = 4) in vec3 aBitangent;

// Bone data for skeletal animations
layout(location = 5) in ivec4 boneIds;
layout(location = 6) in vec4 weights;

out vec2 TexCoords;
out vec3 WorldPos;
out vec3 Normal;
out mat3 TBN;
out vec4 FragPosLightSpace;

layout(std430, binding = 0) readonly buffer InstanceBuffer {
  mat4 modelMatrices[];
};

uniform mat4 u_View;
uniform mat4 u_Projection;
uniform mat4 u_LightSpaceMatrix;
uniform vec2 u_UVOffset;
uniform vec2 u_UVScale = vec2(1.0, 1.0);

// Animation uniforms
const int MAX_BONES = 200;
const int MAX_BONE_INFLUENCE = 4;

layout(std430, binding = 1) readonly buffer BoneBuffer {
  mat4 boneMatrices[];
};

uniform bool u_HasAnimation;

void main()
{
  TexCoords = (aTexCoords * u_UVScale) + u_UVOffset;

  mat4 u_Model = modelMatrices[gl_InstanceID];
  mat4 boneTransform = mat4(0.0f);
  if (u_HasAnimation) {
    int boneOffset = gl_InstanceID * MAX_BONES;
    for (int i = 0; i < MAX_BONE_INFLUENCE; i++) {
      if (boneIds[i] >= 0) {
        boneTransform += boneMatrices[boneOffset + boneIds[i]] * weights[i];
      }
    }
  } else {
    boneTransform = mat4(1.0f);
  }

  // Safety fallback
  if (boneTransform == mat4(0.0f)) {
    boneTransform = mat4(1.0f);
  }

  vec4 localPosition = boneTransform * vec4(aPos, 1.0f);
  WorldPos = vec3(u_Model * localPosition);

  mat3 normalMat = mat3(transpose(inverse(u_Model * boneTransform)));
  Normal = normalMat * aNormal;

  vec3 T = normalize(normalMat * aTangent);
  vec3 B = normalize(normalMat * aBitangent);
  vec3 N = normalize(normalMat * aNormal);
  TBN = mat3(T, B, N);

  FragPosLightSpace = u_LightSpaceMatrix * vec4(WorldPos, 1.0);
  gl_Position = u_Projection * u_View * vec4(WorldPos, 1.0);
}
