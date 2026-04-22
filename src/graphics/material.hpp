#pragma once

#include "graphics/texture.hpp"
#include "resource/texture_manager.hpp"

#include <cassert>
#include <memory>

class MaterialBuilder;

class Material {
private:
  std::shared_ptr<Texture> m_diffuse;
  std::shared_ptr<Texture> m_normal;
  std::shared_ptr<Texture> m_height;
  std::shared_ptr<Texture> m_metallic;
  std::shared_ptr<Texture> m_roughness;
  std::shared_ptr<Texture> m_ao;

  float m_metallicFactor;
  float m_roughnessFactor;
  float m_aoFactor;

public:
  Material(std::shared_ptr<Texture> diffuse, std::shared_ptr<Texture> normal,
           std::shared_ptr<Texture> height, std::shared_ptr<Texture> metallic,
           std::shared_ptr<Texture> roughness, std::shared_ptr<Texture> ao,
           float metallicFactor = 0.0f, float roughnessFactor = 1.0f,
           float aoFactor = 1.0f)
      : m_diffuse(std::move(diffuse)), m_normal(std::move(normal)),
        m_height(std::move(height)), m_metallic(std::move(metallic)),
        m_roughness(std::move(roughness)), m_ao(std::move(ao)),
        m_metallicFactor(metallicFactor), m_roughnessFactor(roughnessFactor),
        m_aoFactor(aoFactor) {}

  Material(Material &&other) noexcept
      : m_diffuse(std::move(other.m_diffuse)),
        m_normal(std::move(other.m_normal)),
        m_height(std::move(other.m_height)),
        m_metallic(std::move(other.m_metallic)),
        m_roughness(std::move(other.m_roughness)), m_ao(std::move(other.m_ao)),
        m_metallicFactor(other.m_metallicFactor),
        m_roughnessFactor(other.m_roughnessFactor),
        m_aoFactor(other.m_aoFactor) {}

  Material() = default;
  Material(const Material &other) = default;
  Material &operator=(const Material &other) = default;

  const std::shared_ptr<Texture> &getDiffuse() const { return m_diffuse; }
  const std::shared_ptr<Texture> &getNormal() const { return m_normal; }
  const std::shared_ptr<Texture> &getHeight() const { return m_height; }
  const std::shared_ptr<Texture> &getMetallic() const { return m_metallic; }
  const std::shared_ptr<Texture> &getRoughness() const { return m_roughness; }
  const std::shared_ptr<Texture> &getAO() const { return m_ao; }

  float getMetallicFactor() const { return m_metallicFactor; }
  float getRoughnessFactor() const { return m_roughnessFactor; }
  float getAOFactor() const { return m_aoFactor; }

  static MaterialBuilder builder();
  static MaterialBuilder builder(const Material &ref_material);
};

class MaterialBuilder {
private:
  std::shared_ptr<Texture> m_diffuse = nullptr;
  std::shared_ptr<Texture> m_normal = nullptr;
  std::shared_ptr<Texture> m_height = nullptr;
  std::shared_ptr<Texture> m_metallic = nullptr;
  std::shared_ptr<Texture> m_roughness = nullptr;
  std::shared_ptr<Texture> m_ao = nullptr;

  float m_metallicFactor = 0.0f;
  float m_roughnessFactor = 1.0f;
  float m_aoFactor = 1.0f;

public:
  MaterialBuilder()
      : m_diffuse(nullptr), m_normal(nullptr), m_height(nullptr),
        m_metallic(nullptr), m_roughness(nullptr), m_ao(nullptr) {}

  MaterialBuilder(const Material &ref_material)
      : m_diffuse(ref_material.getDiffuse()),
        m_normal(ref_material.getNormal()), m_height(ref_material.getHeight()),
        m_metallic(ref_material.getMetallic()),
        m_roughness(ref_material.getRoughness()), m_ao(ref_material.getAO()),
        m_metallicFactor(ref_material.getMetallicFactor()),
        m_roughnessFactor(ref_material.getRoughnessFactor()),
        m_aoFactor(ref_material.getAOFactor()) {}

  MaterialBuilder &setDiffuse(std::shared_ptr<Texture> diffuse) {
    m_diffuse = std::move(diffuse);
    return *this;
  }

  MaterialBuilder &setNormal(std::shared_ptr<Texture> normal) {
    m_normal = std::move(normal);
    return *this;
  }

  MaterialBuilder &setHeight(std::shared_ptr<Texture> height) {
    m_height = std::move(height);
    return *this;
  }

  MaterialBuilder &setMetallic(std::shared_ptr<Texture> metallic) {
    m_metallic = std::move(metallic);
    return *this;
  }

  MaterialBuilder &setRoughness(std::shared_ptr<Texture> roughness) {
    m_roughness = std::move(roughness);
    return *this;
  }

  MaterialBuilder &setAO(std::shared_ptr<Texture> ao) {
    m_ao = std::move(ao);
    return *this;
  }

  MaterialBuilder &setMetallicFactor(float factor) {
    m_metallicFactor = factor;
    return *this;
  }

  MaterialBuilder &setRoughnessFactor(float factor) {
    m_roughnessFactor = factor;
    return *this;
  }

  MaterialBuilder &setAOFactor(float factor) {
    m_aoFactor = factor;
    return *this;
  }

  Material create() {
    assert(TextureManager::exists(STATIC_WHITE_TEXTURE));
    assert(TextureManager::exists(STATIC_BLACK_TEXTURE));
    assert(TextureManager::exists(STATIC_NORMAL_TEXTURE));

    // Diffuse fallback
    if (m_diffuse == nullptr)
      m_diffuse = TextureManager::copy(STATIC_WHITE_TEXTURE);

    // Metallic fallback
    if (m_metallic == nullptr)
      m_metallic = TextureManager::copy(STATIC_PBR_DEFAULT_TEXTURE);

    // Roughness fallback (also use the PBR default for consistency)
    if (m_roughness == nullptr)
      m_roughness = TextureManager::copy(STATIC_PBR_DEFAULT_TEXTURE);

    // Ambient Occlusion fallback
    if (m_ao == nullptr)
      m_ao = TextureManager::copy(STATIC_WHITE_TEXTURE);

    // Normal fallback
    if (m_normal == nullptr)
      m_normal = TextureManager::copy(STATIC_NORMAL_TEXTURE);

    // Height fallback
    if (m_height == nullptr)
      m_height = TextureManager::copy(STATIC_BLACK_TEXTURE);

    return Material(std::move(m_diffuse), std::move(m_normal),
                    std::move(m_height), std::move(m_metallic),
                    std::move(m_roughness), std::move(m_ao), m_metallicFactor,
                    m_roughnessFactor, m_aoFactor);
  }
};

inline MaterialBuilder Material::builder() { return MaterialBuilder(); }
inline MaterialBuilder Material::builder(const Material &ref_material) {
  return MaterialBuilder(ref_material);
}
