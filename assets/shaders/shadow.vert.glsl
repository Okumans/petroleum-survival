#version 450 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoords;

// Bone data
layout(location = 5) in ivec4 boneIds;
layout(location = 6) in vec4 weights;

out vec2 TexCoords;

uniform mat4 u_LightSpaceMatrix;
struct InstanceData {
  mat4 model;
  vec4 emission;
};

layout(std430, binding = 0) readonly buffer InstanceBuffer {
  InstanceData instances[];
};

// Animation uniforms
const int MAX_BONES = 200;
const int MAX_BONE_INFLUENCE = 4;
layout(std430, binding = 1) readonly buffer BoneBuffer {
  mat4 boneMatrices[];
};
uniform bool u_HasAnimation;
uniform int u_BaseInstance;

void main()
{
  TexCoords = aTexCoords;

  int instanceIndex = u_BaseInstance + gl_InstanceID;
  mat4 u_Model = instances[instanceIndex].model;
  mat4 boneTransform = mat4(0.0f);
  if (u_HasAnimation) {
      int boneOffset = instanceIndex * MAX_BONES;
      for(int i = 0 ; i < MAX_BONE_INFLUENCE ; i++) {
          if(boneIds[i] >= 0) {
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
  gl_Position = u_LightSpaceMatrix * u_Model * localPosition;
}
