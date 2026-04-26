#pragma once

#include "utility/name_hash.hpp"
#include <array>
#include <string_view>

namespace ShaderUniforms {

struct LightUniforms {
  std::array<char, 32> position;
  std::array<char, 32> color;
  std::array<char, 32> type;

  NameHash positionHash;
  NameHash colorHash;
  NameHash typeHash;
};

constexpr std::array<LightUniforms, 4> generateLightUniforms() {
  std::array<LightUniforms, 4> results = {};

  for (size_t i = 0; i < 4; ++i) {
    auto build_str = [&](const char *property,
                         std::array<char, 32> &out_arr) -> NameHash {
      char full_buf[32] = "u_Lights[";
      size_t p = 9;

      size_t val = i;
      if (val == 0)
        full_buf[p++] = '0';
      else {
        char temp[5];
        int tp = 0;
        while (val > 0) {
          temp[tp++] = (val % 10) + '0';
          val /= 10;
        }
        while (tp > 0)
          full_buf[p++] = temp[--tp];
      }

      full_buf[p++] = ']';
      full_buf[p++] = '.';

      while (*property)
        full_buf[p++] = *property++;

      full_buf[p] = '\0';

      // Copy to out_arr
      for (size_t j = 0; j <= p; ++j) {
        out_arr[j] = full_buf[j];
      }

      return NameHash(std::string_view(full_buf, p));
    };

    results[i].positionHash = build_str("position", results[i].position);
    results[i].colorHash = build_str("color", results[i].color);
    results[i].typeHash = build_str("type", results[i].type);
  }
  return results;
}

} // namespace ShaderUniforms
