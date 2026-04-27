#pragma once

#include <glm/glm.hpp>
#include <random>

class Random {
public:
  Random() = delete;

  /**
   * @brief Generates a random integer in range [min, max]
   * Constrained to integral types (int, size_t, uint32_t, etc.)
   */
  template <std::integral T = int> static T randInt(T min, T max) {
    std::uniform_int_distribution<T> dist(min, max);
    return dist(s_engine);
  }

  /**
   * @brief Generates a random float in range [min, max]
   * Constrained to floating point types (float, double, long double)
   */
  template <std::floating_point T = float> static T randFloat(T min, T max) {
    std::uniform_real_distribution<T> dist(min, max);
    return dist(s_engine);
  }

  /**
   * @brief Generates a random float between 0 and 1
   * Constrained to floating point types (float, double, long double)
   */
  template <std::floating_point T = float> static T randFloat() {
    std::uniform_real_distribution<T> dist(0, 1);
    return dist(s_engine);
  }

  /**
   * @brief Generates a random integer in range [min, max] using custom weights.
   * The weights vector size must equal (max - min + 1).
   */
  template <std::integral T = int, std::ranges::input_range R>
  static T randWeighted(T min, T max, const R &weights) {
    // weights[0] corresponds to min, weights[1] to min + 1, etc.
    std::discrete_distribution<T> dist(std::begin(weights), std::end(weights));
    return min + dist(s_engine);
  }

  template <std::integral T = int>
  static T randWeighted(T min, T max, std::initializer_list<double> weights) {
    return randWeighted<T, std::initializer_list<double>>(min, max, weights);
  }

  /**
   * @brief Returns true based on a probability P where 0 <= P <= 1
   */
  static bool randChance(std::floating_point auto prob) {
    return randFloat<decltype(prob)>() < prob;
  }

  static glm::vec3 randVec3(float min, float max) {
    return {randFloat(min, max), randFloat(min, max), randFloat(min, max)};
  }

  static void setSeed(unsigned int seed) { s_engine.seed(seed); }

private:
  // Using mt19937_64 for better compatibility with 64-bit types like size_t
  inline static std::mt19937_64 s_engine{std::random_device{}()};
};
