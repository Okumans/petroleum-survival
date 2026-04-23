#pragma once

#include <any>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <vector>

class EventBus {
private:
  struct QueuedEvent {
    std::type_index type;
    std::any payload;
  };

  struct HandlerSlot {
    std::size_t id;
    std::function<void(const std::any &)> call;
  };

  std::unordered_map<std::type_index, std::vector<HandlerSlot>> m_handlers;
  std::vector<QueuedEvent> m_queue;
  std::size_t m_nextHandlerId = 1;

public:
  template <typename EventType, typename Handler>
  std::size_t subscribe(Handler &&handler) {
    static_assert(std::is_invocable_v<Handler, const EventType &>,
                  "Handler must accept const EventType&");

    const std::size_t id = m_nextHandlerId++;
    auto &slots = m_handlers[std::type_index(typeid(EventType))];

    slots.push_back({
        .id = id,
        .call = [wrapped = std::forward<Handler>(handler)](const std::any &evt) {
          wrapped(std::any_cast<const EventType &>(evt));
        },
    });

    return id;
  }

  template <typename EventType> void emit(EventType event) {
    m_queue.push_back(
        {.type = std::type_index(typeid(EventType)), .payload = std::move(event)});
  }

  template <typename EventType> void unsubscribe(std::size_t id) {
    auto it = m_handlers.find(std::type_index(typeid(EventType)));
    if (it == m_handlers.end()) {
      return;
    }

    auto &slots = it->second;
    slots.erase(
        std::remove_if(slots.begin(), slots.end(),
                       [id](const HandlerSlot &slot) { return slot.id == id; }),
        slots.end());
  }

  void flush() {
    std::size_t cursor = 0;
    while (cursor < m_queue.size()) {
      QueuedEvent &queued = m_queue[cursor++];
      auto it = m_handlers.find(queued.type);
      if (it == m_handlers.end()) {
        continue;
      }

      const auto &slots = it->second;
      for (const auto &slot : slots) {
        slot.call(queued.payload);
      }
    }

    m_queue.clear();
  }

  void clear() {
    m_queue.clear();
    m_handlers.clear();
    m_nextHandlerId = 1;
  }
};