#pragma once

#include "graphics/idrawable.hpp"
#include "graphics/material.hpp"
#include "graphics/mesh.hpp"

#include <glm/glm.hpp>

#include <cstdint>
#include <unordered_map>

class MapManager {
private:
  struct TerrainChunk {
    glm::ivec2 coord;
    Mesh mesh;
  };

  static constexpr int s_chunkResolution = 32;
  static constexpr float s_chunkWorldSize = 32.0f;
  static constexpr int s_visibleRadius = 2;
  static constexpr float s_worldHalfSize = 512.0f;
  static constexpr float s_textureRepeat = 0.20f;

  bool m_isInitialized = false;
  Material m_terrainMaterial;
  std::unordered_map<int64_t, TerrainChunk> m_chunks;

public:
  MapManager() = default;

  void setup();
  void update(const glm::vec3 &focus_position);
  void draw(const RenderContext &ctx);

  [[nodiscard]] float sampleHeight(float world_x, float world_z) const;
  [[nodiscard]] glm::vec3 sampleNormal(float world_x, float world_z) const;
  [[nodiscard]] glm::vec3 snapToGround(const glm::vec3 &position,
                                       float base_offset = 0.0f) const;

private:
  static int64_t _encodeKey(int chunk_x, int chunk_z);
  static float _lerp(float a, float b, float t);
  static float _smoothStep(float t);
  static float _hashNoise(int x, int z);

  [[nodiscard]] float _valueNoise(float x, float z) const;
  [[nodiscard]] float _fbm(float x, float z) const;
  [[nodiscard]] float _terrainHeight(float world_x, float world_z) const;
  [[nodiscard]] glm::vec3 _terrainNormal(float world_x, float world_z) const;
  [[nodiscard]] glm::vec2 _chunkOrigin(int chunk_x, int chunk_z) const;
  [[nodiscard]] bool _isInsideWorld(float world_x, float world_z) const;

  void _ensureChunk(int chunk_x, int chunk_z);
  void _pruneChunks(int center_chunk_x, int center_chunk_z);
  [[nodiscard]] Mesh _buildChunkMesh(int chunk_x, int chunk_z) const;
};
