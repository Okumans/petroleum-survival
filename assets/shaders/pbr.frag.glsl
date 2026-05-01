#version 450 core
out vec4 FragColor;

in vec2 TexCoords;
in vec3 WorldPos;
in vec3 Normal;
in mat3 TBN;
in vec4 FragPosLightSpace;
in vec3 Emission;

// PBR Texture Samplers
uniform sampler2D u_DiffuseTex;
uniform sampler2D u_NormalTex;
uniform sampler2D u_HeightTex;
uniform sampler2D u_MetallicTex;
uniform sampler2D u_RoughnessTex;
uniform sampler2D u_AOTex;
uniform samplerCube u_SpecularEnvMap; // Pre-filtered map (Specular IBL)
uniform samplerCube u_IrradianceMap; // Diffuse IBL
uniform sampler2D u_ShadowMap;

// Fallbacks & Factors
uniform vec3 u_BaseColor;
uniform float u_Opacity;
uniform float u_MetallicFactor;
uniform float u_RoughnessFactor;
uniform float u_AOFactor;
uniform float u_HeightScale;
uniform float u_AmbientIntensity;
uniform bool u_UsePackedMR;
uniform vec3 u_EmissionColor;
uniform bool u_EnableTerrainTint;
uniform vec3 u_TerrainTintLow;
uniform vec3 u_TerrainTintHigh;
uniform float u_TerrainTintScale;
uniform float u_TerrainTintStrength;

// Lights
struct Light {
  vec3 position;
  vec3 color;
  int type;
};
#define MAX_LIGHTS 4
uniform Light u_Lights[MAX_LIGHTS];
uniform int u_NumLights;
uniform vec3 u_CameraPos;

const float PI = 3.14159265359;

const vec2 poissonDisk[4] = vec2[](
    vec2(-0.94201624, -0.39906216),
    vec2(0.94558609, -0.76890725),
    vec2(-0.094184101, -0.92938870),
    vec2(0.34495938, 0.29387760)
  );

float hash12(vec2 p)
{
  vec3 p3 = fract(vec3(p.xyx) * 0.1031);
  p3 += dot(p3, p3.yzx + 33.33);
  return fract((p3.x + p3.y) * p3.z);
}

float valueNoise(vec2 p)
{
  vec2 i = floor(p);
  vec2 f = fract(p);
  vec2 u = f * f * (3.0 - 2.0 * f);

  float n00 = hash12(i + vec2(0.0, 0.0));
  float n10 = hash12(i + vec2(1.0, 0.0));
  float n01 = hash12(i + vec2(0.0, 1.0));
  float n11 = hash12(i + vec2(1.0, 1.0));

  float nx0 = mix(n00, n10, u.x);
  float nx1 = mix(n01, n11, u.x);
  return mix(nx0, nx1, u.y);
}

float fbm2(vec2 p)
{
  float value = 0.0;
  float amplitude = 0.5;
  for (int i = 0; i < 4; ++i) {
    value += amplitude * valueNoise(p);
    p *= 2.0;
    amplitude *= 0.5;
  }
  return value;
}

// ----------------------------------------------------------------------------
vec2 ParallaxMapping(vec2 texCoords, vec3 viewDir)
{
  const float minLayers = 8.0;
  const float maxLayers = 32.0;
  float numLayers = mix(maxLayers, minLayers, abs(dot(vec3(0.0, 0.0, 1.0), viewDir)));
  float layerDepth = 1.0 / numLayers;
  float currentLayerDepth = 0.0;

  vec2 p = viewDir.xy / max(viewDir.z, 0.01) * u_HeightScale;
  vec2 deltaTexCoords = p / numLayers;

  vec2 currentTexCoords = texCoords;
  float currentDepthMapValue = texture(u_HeightTex, currentTexCoords).r;

  while (currentLayerDepth < currentDepthMapValue) {
    currentTexCoords -= deltaTexCoords;
    currentDepthMapValue = texture(u_HeightTex, currentTexCoords).r;
    currentLayerDepth += layerDepth;
  }

  vec2 prevTexCoords = currentTexCoords + deltaTexCoords;
  float afterDepth = currentDepthMapValue - currentLayerDepth;
  float beforeDepth = texture(u_HeightTex, prevTexCoords).r - currentLayerDepth + layerDepth;
  float weight = afterDepth / (afterDepth - beforeDepth);
  return prevTexCoords * weight + currentTexCoords * (1.0 - weight);
}

// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
  float a = roughness * roughness;
  float a2 = a * a;
  float NdotH = max(dot(N, H), 0.0);
  float NdotH2 = NdotH * NdotH;
  return a2 / (PI * pow(NdotH2 * (a2 - 1.0) + 1.0, 2.0));
}

float GeometrySchlickGGX(float NdotV, float roughness)
{
  float r = (roughness + 1.0);
  float k = (r * r) / 8.0;
  return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
  return GeometrySchlickGGX(max(dot(N, V), 0.0), roughness) * GeometrySchlickGGX(max(dot(N, L), 0.0), roughness);
}

vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
  return F0 + (1.0 - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

vec3 fresnelSchlickRoughness(float cosTheta, vec3 F0, float roughness)
{
  return F0 + (max(vec3(1.0 - roughness), F0) - F0) * pow(clamp(1.0 - cosTheta, 0.0, 1.0), 5.0);
}

// ----------------------------------------------------------------------------
float ShadowCalculation(vec4 fragPosLightSpace, vec3 normal, vec3 lightDir)
{
  vec3 projCoords = fragPosLightSpace.xyz / fragPosLightSpace.w;
  projCoords = projCoords * 0.5 + 0.5;
  if (projCoords.z > 1.0) return 0.0;

  float currentDepth = projCoords.z;
  float bias = max(0.005 * (1.0 - dot(normal, lightDir)), 0.0001);
  float shadow = 0.0;
  vec2 texelSize = 1.0 / textureSize(u_ShadowMap, 0);

  // 4-sample PCF for better performance
  for (int x = -1; x <= 0; ++x) {
    for (int y = -1; y <= 0; ++y) {
      float pcfDepth = texture(u_ShadowMap, projCoords.xy + vec2(x, y) * texelSize).r;
      shadow += currentDepth - bias > pcfDepth ? 1.0 : 0.0;
    }
  }
  return shadow / 4.0;
}

// ----------------------------------------------------------------------------
void main()
{
  vec3 V = normalize(u_CameraPos - WorldPos);
  vec3 tangentViewDir = normalize(transpose(TBN) * V);
  
  vec2 texCoords = TexCoords;
  if (u_HeightScale > 0.0) {
    texCoords = ParallaxMapping(TexCoords, tangentViewDir);
  }

  // Early alpha discard to save expensive PBR and shadow calculations
  vec4 diffuseSample = texture(u_DiffuseTex, texCoords);
  float finalAlpha = diffuseSample.a * u_Opacity;
  if (finalAlpha < 0.5) discard;

  vec3 albedo = pow(diffuseSample.rgb, vec3(2.2)) * u_BaseColor;

  if (u_EnableTerrainTint) {
    float tintNoise = fbm2(WorldPos.xz * u_TerrainTintScale + vec2(17.0, -9.0));
    vec3 tint = mix(u_TerrainTintLow, u_TerrainTintHigh, clamp(tintNoise, 0.0, 1.0));
    albedo = mix(albedo, albedo * tint, clamp(u_TerrainTintStrength, 0.0, 1.0));
  }

  // Sampling separate or packed Metallic and Roughness maps
  float metallic, roughness;
  if (u_UsePackedMR) {
    vec3 mrSample = texture(u_MetallicTex, texCoords).rgb;
    metallic = mrSample.b * u_MetallicFactor;
    roughness = mrSample.g * u_RoughnessFactor;
  } else {
    metallic = texture(u_MetallicTex, texCoords).r * u_MetallicFactor;
    roughness = texture(u_RoughnessTex, texCoords).r * u_RoughnessFactor;
  }

  roughness = clamp(roughness, 0.005, 1.0);
  metallic = clamp(metallic, 0.0, 1.0);
  float ao = texture(u_AOTex, texCoords).r * u_AOFactor;

  vec3 N = normalize(TBN * (texture(u_NormalTex, texCoords).xyz * 2.0 - 1.0));
  vec3 R = reflect(-V, N);

  vec3 F0 = mix(vec3(0.04), albedo, metallic);
  vec3 Lo = vec3(0.0);

  for (int i = 0; i < min(u_NumLights, MAX_LIGHTS); ++i)
  {
    vec3 L = (u_Lights[i].type == 1) ? normalize(-u_Lights[i].position) : normalize(u_Lights[i].position - WorldPos);
    float attenuation = (u_Lights[i].type == 1) ? 1.0 : 1.0 / (length(u_Lights[i].position - WorldPos) * length(u_Lights[i].position - WorldPos));
    vec3 radiance = u_Lights[i].color * attenuation;

    vec3 H = normalize(V + L);
    float NDF = DistributionGGX(N, H, roughness);
    float G = GeometrySmith(N, V, L, roughness);
    vec3 F = fresnelSchlick(max(dot(H, V), 0.0), F0);

    vec3 numerator = NDF * G * F;
    float denominator = 4.0 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0) + 0.0001;
    vec3 specular = numerator / denominator;

    vec3 kS = F;
    vec3 kD = (vec3(1.0) - kS) * (1.0 - metallic);

    float NdotL = max(dot(N, L), 0.0);

    float shadow = (i == 0) ? ShadowCalculation(FragPosLightSpace, N, L) : 0.0;
    shadow *= 0.8;
    Lo += (1.0 - shadow) * (kD * albedo / PI + specular) * radiance * NdotL;
  }

  // --- simplified Image Based Lighting (IBL) ---
  // Use roughness-adjusted Fresnel for the ambient specular weight
  vec3 F = fresnelSchlickRoughness(max(dot(N, V), 0.0), F0, roughness);
  vec3 kS_ambient = F;
  vec3 kD_ambient = (1.0 - kS_ambient) * (1.0 - metallic);

  // Indirect Diffuse (Irradiance Cubemap)
  vec3 irradiance = texture(u_IrradianceMap, N).rgb;
  vec3 ambient_diffuse = irradiance * albedo * kD_ambient;

  // Indirect Specular (Pre-filtered Cubemap)
  // Without the BRDF LUT, we use F directly to scale the pre-filtered color.
  const float MAX_REFLECTION_LOD = 4.0;
  vec3 prefilteredColor = textureLod(u_SpecularEnvMap, R, roughness * MAX_REFLECTION_LOD).rgb;
  vec3 ambient_specular = prefilteredColor * kS_ambient;

  vec3 ambient = (ambient_diffuse + ambient_specular) * ao * u_AmbientIntensity;
  vec3 color = ambient + Lo + u_EmissionColor + Emission;

  // Tone Mapping & Gamma Correction
  color = color / (color + vec3(1.0));
  color = pow(color, vec3(1.0 / 2.2));

  FragColor = vec4(color, finalAlpha);
}
