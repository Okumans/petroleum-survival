#pragma once

#include "utility/not_initialized.hpp"

#include <concepts>

template <typename T, typename Counter = float>
  requires std::integral<Counter> || std::floating_point<Counter>
struct AnimationState {
  NotInitialized<Counter, "duration"> duration;
  Counter timer{};

  T target{};
  T start{};

  bool animationStarted = false;

  AnimationState() = default;
  AnimationState(Counter duration) { this->duration.init(duration); }

  Counter updateTimer(Counter delta_time) {
    timer += delta_time;
    return timer;
  };

  void startAnimation(T start, T target) {
    this->start = start;
    this->target = target;
    this->timer = 0;
    this->animationStarted = true;
  }

  void reset() {
    animationStarted = false;
    timer = 0;
  }

  [[nodiscard]] Counter getProgress() const noexcept {
    if (duration.ensureInitialized() <= 0)
      return 1.0f;

    Counter t = timer / duration.ensureInitialized();

    if (t > 1.0)
      return 1.0;
    if (t < 0.0)
      return 0.0;
    return t;
  }

  [[nodiscard]] Counter isFinished() const noexcept {
    return getProgress() == static_cast<Counter>(1.0);
  }
};

template <typename Counter> struct AnimationState<void, Counter> {
  NotInitialized<Counter, "duration"> duration;
  Counter timer{};
  bool animationStarted = false;

  AnimationState() = default;
  AnimationState(Counter d) { this->duration.init(d); }

  Counter updateTimer(Counter delta_time) {
    timer += delta_time;
    return timer;
  }

  void startAnimation() {
    this->timer = 0;
    this->animationStarted = true;
  }

  void reset() {
    animationStarted = false;
    timer = 0;
  }

  [[nodiscard]] Counter getProgress() const noexcept {
    Counter d = duration.ensureInitialized();
    if (d <= 0)
      return static_cast<Counter>(1.0);

    Counter t = timer / d;
    if (t > static_cast<Counter>(1.0))
      return static_cast<Counter>(1.0);
    if (t < static_cast<Counter>(0.0))
      return static_cast<Counter>(0.0);
    return t;
  }

  [[nodiscard]] Counter isFinished() const noexcept {
    return getProgress() == static_cast<Counter>(1.0);
  }
};
