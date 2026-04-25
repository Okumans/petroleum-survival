#include "map_manager.hpp"

#include "graphics/mesh.hpp"
#include "resource/texture_manager.hpp"
#include "scene/game_object_manager.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <ranges>

namespace {
const TextureName TERRAIN_DIFFUSE_TEXTURE("terrain_grass_diffuse");
const TextureName TERRAIN_NORMAL_TEXTURE("terrain_grass_normal");
const TextureName TERRAIN_HEIGHT_TEXTURE("terrain_grass_height");
const TextureName TERRAIN_ROUGHNESS_TEXTURE("terrain_grass_roughness");
const TextureName TERRAIN_AO_TEXTURE("terrain_grass_ao");
} // namespace

void MapManager::setup() {
  if (m_isInitialized) {
    return;
  }

  auto fallbackDiffuse = TextureManager::copy(STATIC_WHITE_TEXTURE);
  auto fallbackNormal = TextureManager::copy(STATIC_NORMAL_TEXTURE);
  auto fallbackHeight = TextureManager::copy(STATIC_BLACK_TEXTURE);
  auto fallbackRoughness = TextureManager::copy(STATIC_PBR_DEFAULT_TEXTURE);
  auto fallbackAO = TextureManager::copy(STATIC_WHITE_TEXTURE);

  auto terrainDiffuse = TextureManager::tryGet(TERRAIN_DIFFUSE_TEXTURE)
                            ? TextureManager::copy(TERRAIN_DIFFUSE_TEXTURE)
                            : fallbackDiffuse;
  auto terrainNormal = TextureManager::tryGet(TERRAIN_NORMAL_TEXTURE)
                           ? TextureManager::copy(TERRAIN_NORMAL_TEXTURE)
                           : fallbackNormal;
  auto terrainHeight = TextureManager::tryGet(TERRAIN_HEIGHT_TEXTURE)
                           ? TextureManager::copy(TERRAIN_HEIGHT_TEXTURE)
                           : fallbackHeight;
  auto terrainRoughness = TextureManager::tryGet(TERRAIN_ROUGHNESS_TEXTURE)
                              ? TextureManager::copy(TERRAIN_ROUGHNESS_TEXTURE)
                              : fallbackRoughness;
  auto terrainAO = TextureManager::tryGet(TERRAIN_AO_TEXTURE)
                       ? TextureManager::copy(TERRAIN_AO_TEXTURE)
                       : fallbackAO;

  m_terrainMaterial = Material::builder()
                          .setDiffuse(std::move(terrainDiffuse))
                          .setNormal(std::move(terrainNormal))
                          .setHeight(std::move(terrainHeight))
                          .setRoughness(std::move(terrainRoughness))
                          .setAO(std::move(terrainAO))
                          .setMetallicFactor(0.0f)
                          .setRoughnessFactor(1.0f)
                          .setAOFactor(1.0f)
                          .create();

  m_isInitialized = true;
}

void MapManager::update(const glm::vec3 &focus_position) {
  if (!m_isInitialized) {
    return;
  }

  int center_chunk_x =
      static_cast<int>(std::floor(focus_position.x / s_chunkWorldSize));
  int center_chunk_z =
      static_cast<int>(std::floor(focus_position.z / s_chunkWorldSize));

  const int32_t visible_radius = static_cast<int32_t>(s_visibleRadius);

  for (int32_t z = -visible_radius; z <= visible_radius; ++z) {
    for (int32_t x = -visible_radius; x <= visible_radius; ++x) {
      _ensureChunk(center_chunk_x + x, center_chunk_z + z);
    }
  }

  _pruneChunks(center_chunk_x, center_chunk_z);
}

void MapManager::draw(const RenderContext &ctx) {
  if (!m_isInitialized) {
    return;
  }

  ctx.shader.setMat4("u_Model", glm::mat4(1.0f));

  for (auto &[_, chunk] : m_chunks) {
    chunk.mesh.draw(ctx);
  }
}

float MapManager::sampleHeight(float world_x, float world_z) const {
  if (!_isInsideWorld(world_x, world_z)) {
    world_x = std::clamp(world_x, -s_worldHalfSize, s_worldHalfSize);
    world_z = std::clamp(world_z, -s_worldHalfSize, s_worldHalfSize);
  }

  return _terrainHeight(world_x, world_z);
}

glm::vec3 MapManager::sampleNormal(float world_x, float world_z) const {
  if (!_isInsideWorld(world_x, world_z)) {
    world_x = std::clamp(world_x, -s_worldHalfSize, s_worldHalfSize);
    world_z = std::clamp(world_z, -s_worldHalfSize, s_worldHalfSize);
  }

  return _terrainNormal(world_x, world_z);
}

glm::vec3 MapManager::snapToGround(const glm::vec3 &position,
                                   float base_offset) const {
  glm::vec3 snapped = position;
  snapped.y = sampleHeight(position.x, position.z) + base_offset;
  return snapped;
}

bool MapManager::isPositionInLoadedChunk(const glm::vec3 &position) const {
  int chunk_x = static_cast<int>(std::floor(position.x / s_chunkWorldSize));
  int chunk_z = static_cast<int>(std::floor(position.z / s_chunkWorldSize));
  int64_t key = _encodeKey(chunk_x, chunk_z);

  return m_chunks.contains(key);
}

void MapManager::registerObject(const ObjectHandle &handle,
                                const glm::vec3 &position, bool is_static) {
  if (!handle.isValid()) {
    return;
  }

  int chunk_x = static_cast<int>(std::floor(position.x / s_chunkWorldSize));
  int chunk_z = static_cast<int>(std::floor(position.z / s_chunkWorldSize));
  int64_t chunk_key = _encodeKey(chunk_x, chunk_z);

  ChunkObjectSet &chunk_set = m_chunk_objects[chunk_key];
  std::vector<ObjectHandle> &target_vector =
      is_static ? chunk_set.static_objects : chunk_set.dynamic_objects;
  target_vector.push_back(handle);

  m_tracked_objects[_encodeHandleKey(handle)] =
      TrackedObjectState{.chunk_key = chunk_key, .is_static = is_static};
}

void MapManager::unregisterObject(const ObjectHandle &handle) {
  if (!handle.isValid())
    return;

  auto tracked_it = m_tracked_objects.find(_encodeHandleKey(handle));
  if (tracked_it == m_tracked_objects.end())
    return;

  int64_t chunk_key = tracked_it->second.chunk_key;
  bool is_static = tracked_it->second.is_static;

  if (auto chunk_it = m_chunk_objects.find(chunk_key);
      chunk_it != m_chunk_objects.end()) {

    std::vector<ObjectHandle> &target_vector =
        is_static ? chunk_it->second.static_objects
                  : chunk_it->second.dynamic_objects;

    std::erase(target_vector, handle);

    if (chunk_it->second.static_objects.empty() &&
        chunk_it->second.dynamic_objects.empty()) {
      m_chunk_objects.erase(chunk_it);
    }
  }

  m_tracked_objects.erase(tracked_it);
}

void MapManager::updateObjectChunk(const ObjectHandle &handle,
                                   const glm::vec3 &new_position) {
  if (!handle.isValid())
    return;

  auto tracked_it = m_tracked_objects.find(_encodeHandleKey(handle));
  if (tracked_it == m_tracked_objects.end() || tracked_it->second.is_static)
    return;

  int chunk_x = static_cast<int>(std::floor(new_position.x / s_chunkWorldSize));
  int chunk_z = static_cast<int>(std::floor(new_position.z / s_chunkWorldSize));
  int64_t next_chunk_key = _encodeKey(chunk_x, chunk_z);

  if (tracked_it->second.chunk_key == next_chunk_key)
    return;

  int64_t prev_chunk_key = tracked_it->second.chunk_key;
  if (auto prev_chunk_it = m_chunk_objects.find(prev_chunk_key);
      prev_chunk_it != m_chunk_objects.end()) {

    std::erase(prev_chunk_it->second.dynamic_objects, handle);

    if (prev_chunk_it->second.static_objects.empty() &&
        prev_chunk_it->second.dynamic_objects.empty()) {
      m_chunk_objects.erase(prev_chunk_it);
    }
  }

  m_chunk_objects[next_chunk_key].dynamic_objects.push_back(handle);
  tracked_it->second.chunk_key = next_chunk_key;
}

void MapManager::collectLoadedChunkHandles(
    std::vector<ObjectHandle> &out_handles) const {
  out_handles.clear();

  for (const auto &[chunk_key, _] : m_chunks) {
    auto chunk_object_it = m_chunk_objects.find(chunk_key);
    if (chunk_object_it == m_chunk_objects.end())
      continue;

    const ChunkObjectSet &chunk_set = chunk_object_it->second;
    out_handles.insert(out_handles.end(), chunk_set.static_objects.begin(),
                       chunk_set.static_objects.end());
    out_handles.insert(out_handles.end(), chunk_set.dynamic_objects.begin(),
                       chunk_set.dynamic_objects.end());
  }
}

void MapManager::clearObjectTracking() {
  m_chunk_objects.clear();
  m_tracked_objects.clear();
}

int64_t MapManager::_encodeKey(int chunk_x, int chunk_z) {
  return (static_cast<int64_t>(chunk_x) << 32) |
         static_cast<int64_t>(static_cast<uint32_t>(chunk_z));
}

uint64_t MapManager::_encodeHandleKey(const ObjectHandle &handle) {
  return (static_cast<uint64_t>(handle.id) << 32) |
         static_cast<uint64_t>(handle.generation);
}

float MapManager::_lerp(float a, float b, float t) { return a + (b - a) * t; }

float MapManager::_smoothStep(float t) { return t * t * (3.0f - 2.0f * t); }

float MapManager::_hashNoise(int x, int z) {
  uint32_t seed = static_cast<uint32_t>(x) * 374761393u +
                  static_cast<uint32_t>(z) * 668265263u + 0x9E3779B9u;
  seed = (seed ^ (seed >> 13u)) * 1274126177u;
  seed ^= seed >> 16u;
  return static_cast<float>(seed & 0x00FFFFFFu) /
         static_cast<float>(0x01000000u);
}

float MapManager::_valueNoise(float x, float z) const {
  int x0 = static_cast<int>(std::floor(x));
  int z0 = static_cast<int>(std::floor(z));
  int x1 = x0 + 1;
  int z1 = z0 + 1;

  float tx = _smoothStep(x - static_cast<float>(x0));
  float tz = _smoothStep(z - static_cast<float>(z0));

  float n00 = _hashNoise(x0, z0);
  float n10 = _hashNoise(x1, z0);
  float n01 = _hashNoise(x0, z1);
  float n11 = _hashNoise(x1, z1);

  float nx0 = _lerp(n00, n10, tx);
  float nx1 = _lerp(n01, n11, tx);
  return _lerp(nx0, nx1, tz) * 2.0f - 1.0f;
}

float MapManager::_fbm(float x, float z) const {
  float total = 0.0f;
  float amplitude = 1.0f;
  float frequency = 0.015f;
  float normalization = 0.0f;

  for (int octave = 0; octave < 5; ++octave) {
    total += _valueNoise(x * frequency, z * frequency) * amplitude;
    normalization += amplitude;
    amplitude *= 0.5f;
    frequency *= 2.0f;
  }

  return total / normalization;
}

float MapManager::_terrainHeight(float world_x, float world_z) const {
  float broad_wave = std::sin(world_x * 0.0075f) * std::cos(world_z * 0.0065f);
  float ridge_noise = 1.0f - std::abs(_fbm(world_x + 13.0f, world_z - 7.0f));
  float detail_noise = _fbm(world_x * 1.5f, world_z * 1.5f);

  float height = broad_wave * 3.0f;
  height += ridge_noise * 5.5f;
  height += detail_noise * 2.0f;
  return height;
}

glm::vec3 MapManager::_terrainNormal(float world_x, float world_z) const {
  const float epsilon = 0.5f;

  float height_l = _terrainHeight(world_x - epsilon, world_z);
  float height_r = _terrainHeight(world_x + epsilon, world_z);
  float height_d = _terrainHeight(world_x, world_z - epsilon);
  float height_u = _terrainHeight(world_x, world_z + epsilon);

  glm::vec3 normal(height_l - height_r, 2.0f * epsilon, height_d - height_u);
  return glm::normalize(normal);
}

glm::vec2 MapManager::_chunkOrigin(int chunk_x, int chunk_z) const {
  return {static_cast<float>(chunk_x) * s_chunkWorldSize,
          static_cast<float>(chunk_z) * s_chunkWorldSize};
}

bool MapManager::_isInsideWorld(float world_x, float world_z) const {
  return std::abs(world_x) <= s_worldHalfSize &&
         std::abs(world_z) <= s_worldHalfSize;
}

void MapManager::_ensureChunk(int chunk_x, int chunk_z) {
  if (std::abs(chunk_x) * s_chunkWorldSize > s_worldHalfSize ||
      std::abs(chunk_z) * s_chunkWorldSize > s_worldHalfSize) {
    return;
  }

  int64_t key = _encodeKey(chunk_x, chunk_z);
  if (m_chunks.contains(key))
    return;

  m_chunks.emplace(key,
                   TerrainChunk{.coord = {chunk_x, chunk_z},
                                .mesh = _buildChunkMesh(chunk_x, chunk_z)});
}

void MapManager::_pruneChunks(int center_chunk_x, int center_chunk_z) {
  std::erase_if(m_chunks, [center_chunk_x, center_chunk_z](const auto &pair) {
    glm::ivec2 coord = pair.second.coord;

    int dx = std::abs(coord.x - center_chunk_x);
    int dz = std::abs(coord.y - center_chunk_z);

    return (dx > s_visibleRadius + 1 || dz > s_visibleRadius + 1);
  });
}

Mesh MapManager::_buildChunkMesh(int chunk_x, int chunk_z) const {
  const size_t vertices_per_side = static_cast<size_t>(s_chunkResolution) + 1;
  const size_t vertex_count = vertices_per_side * vertices_per_side;
  const size_t index_count = s_chunkResolution * s_chunkResolution * 6;

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  vertices.reserve(vertex_count);
  indices.reserve(index_count);

  glm::vec2 chunk_origin = _chunkOrigin(chunk_x, chunk_z);
  const float step = s_chunkWorldSize / static_cast<float>(s_chunkResolution);

  for (size_t z = 0; z < vertices_per_side; ++z) {
    for (size_t x = 0; x < vertices_per_side; ++x) {
      float world_x = chunk_origin.x + static_cast<float>(x) * step;
      float world_z = chunk_origin.y + static_cast<float>(z) * step;
      float height = _terrainHeight(world_x, world_z);

      Vertex vertex{};
      vertex.position = {world_x, height, world_z};
      vertex.normal = _terrainNormal(world_x, world_z);
      vertex.texCoords = {world_x * s_textureRepeat, world_z * s_textureRepeat};
      vertex.tangent = {1.0f, 0.0f, 0.0f};
      vertex.bitangent = {0.0f, 0.0f, 1.0f};

      vertices.push_back(vertex);
    }
  }

  for (size_t z = 0; z < s_chunkResolution; ++z) {
    for (size_t x = 0; x < s_chunkResolution; ++x) {
      uint32_t topLeft = static_cast<uint32_t>(z * vertices_per_side + x);
      uint32_t topRight = topLeft + 1;
      uint32_t bottomLeft =
          static_cast<uint32_t>((z + 1) * vertices_per_side + x);
      uint32_t bottomRight = bottomLeft + 1;

      indices.push_back(topLeft);
      indices.push_back(bottomLeft);
      indices.push_back(topRight);

      indices.push_back(topRight);
      indices.push_back(bottomLeft);
      indices.push_back(bottomRight);
    }
  }

  return Mesh(std::move(vertices), std::move(indices), m_terrainMaterial,
              glm::vec3(1.0f), 1.0f);
}
