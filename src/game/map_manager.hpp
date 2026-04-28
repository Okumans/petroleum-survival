#pragma once

#include "graphics/material.hpp"
#include "graphics/mesh.hpp"
#include "graphics/render_context.hpp"
#include "scene/game_object_manager.hpp"

#include <concepts>
#include <glm/glm.hpp>

#include <cstdint>
#include <unordered_map>
#include <vector>

class Renderer;

/** @brief Owns terrain generation and loaded-chunk membership.
 *
 * The manager does not advance game object behavior by itself. Callers are
 * expected to use the loaded-chunk helpers to update or draw object state when
 * needed.
 */
class MapManager {
private:
  struct TerrainChunk {
    glm::ivec2 coord;
    Mesh mesh;
  };

  struct ChunkObjectSet {
    std::vector<ObjectHandle> static_objects;
    std::vector<ObjectHandle> dynamic_objects;
  };

  struct TrackedObjectState {
    int64_t chunk_key;
    bool is_static;
  };

  // Terrain grid resolution used when generating each chunk mesh.
  static constexpr uint16_t s_chunkResolution = 32;
  // Number of chunks kept loaded around the focus position in each direction.
  static constexpr uint16_t s_visibleRadius = 2;
  // World-space size covered by a single terrain chunk.
  static constexpr float s_chunkWorldSize = 15.0f;
  // Half-width/half-depth of the playable world area in world units.
  static constexpr float s_worldHalfSize = 512.0f;
  // UV scale applied to terrain meshes to control texture tiling.
  static constexpr float s_textureRepeat = 0.20f;

  bool m_isInitialized = false;
  Material m_terrainMaterial;
  std::unordered_map<int64_t, TerrainChunk> m_chunks;
  std::unordered_map<int64_t, ChunkObjectSet> m_chunk_objects;
  std::unordered_map<uint64_t, TrackedObjectState> m_tracked_objects;

  // Heightmap cache for O(1) lookup
  std::vector<float> m_heightmap;
  static constexpr float s_gridResolution = 0.5f;
  uint32_t m_gridSize = 0;

public:
  MapManager() = default;

  void setup();
  void update(const glm::vec3 &focus_position);
  void submitToRenderer(Renderer &renderer);

  [[nodiscard]] float sampleHeight(float world_x, float world_z) const;
  [[nodiscard]] float sampleHeightNoCache(float world_x, float world_z) const;
  [[nodiscard]] glm::vec3 sampleNormal(float world_x, float world_z) const;
  [[nodiscard]] glm::vec3 snapToGround(const glm::vec3 &position,
                                       float base_offset = 0.0f) const;
  [[nodiscard]] glm::vec3 snapToGroundNoCache(const glm::vec3 &position,
                                              float base_offset = 0.0f) const;
  [[nodiscard]] bool isPositionInLoadedChunk(const glm::vec3 &position) const;

  void registerObject(const ObjectHandle &handle, const glm::vec3 &position,
                      bool is_static);
  void unregisterObject(const ObjectHandle &handle);
  /** @brief Updates chunk membership bookkeeping after the caller moves an
   * object. */
  void updateObjectChunk(const ObjectHandle &handle,
                         const glm::vec3 &new_position);

  enum class ObjectFilter : uint8_t {
    None = 0,
    Static = 1 << 0,
    Dynamic = 1 << 1,
    All = Static | Dynamic
  };

  /** @brief Collects every object attached to loaded chunks */
  void collectLoadedChunkHandles(std::vector<ObjectHandle> &out_handles,
                                 ObjectFilter filter = ObjectFilter::All) const;

  /** @brief Iterates every object attached to loaded chunks */
  void foreachLoadedChunkHandles(std::invocable<ObjectHandle> auto &&pred,
                                 ObjectFilter filter = ObjectFilter::All) const;
  void clearObjectTracking();

private:
  static int64_t _encodeKey(int chunk_x, int chunk_z);
  static uint64_t _encodeHandleKey(const ObjectHandle &handle);
  static float _lerp(float a, float b, float t);
  static float _smoothStep(float t);
  static float _hashNoise(int x, int z);

  [[nodiscard]] float _valueNoise(float x, float z) const;
  [[nodiscard]] float _fbm(float x, float z) const;
  [[nodiscard]] float _terrainHeight(float world_x, float world_z) const;
  [[nodiscard]] float _sampleHeightCached(float world_x, float world_z) const;
  [[nodiscard]] glm::vec3 _terrainNormal(float world_x, float world_z) const;
  [[nodiscard]] glm::vec2 _chunkOrigin(int chunk_x, int chunk_z) const;
  [[nodiscard]] bool _isInsideWorld(float world_x, float world_z) const;

  void _ensureChunk(int chunk_x, int chunk_z);
  void _pruneChunks(int center_chunk_x, int center_chunk_z);
  void _generateHeightmap();
  [[nodiscard]] Mesh _buildChunkMesh(int chunk_x, int chunk_z) const;
};

// Implementations

void MapManager::foreachLoadedChunkHandles(
    std::invocable<ObjectHandle> auto &&pred, ObjectFilter filter) const {
  for (const auto &[chunk_key, _] : m_chunks) {
    auto chunk_object_it = m_chunk_objects.find(chunk_key);

    if (chunk_object_it == m_chunk_objects.end())
      continue;

    const ChunkObjectSet &chunk_set = chunk_object_it->second;

    bool include_static = static_cast<uint8_t>(filter) &
                          static_cast<uint8_t>(ObjectFilter::Static);

    bool include_dynamic = static_cast<uint8_t>(filter) &
                           static_cast<uint8_t>(ObjectFilter::Dynamic);

    if (include_static) {
      for (const auto &h : chunk_set.static_objects)
        pred(h);
    }

    if (include_dynamic) {
      for (const auto &h : chunk_set.dynamic_objects)
        pred(h);
    }
  }
}
