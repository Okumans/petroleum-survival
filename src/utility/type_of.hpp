// Source - https://stackoverflow.com/a/68523970
// Posted by Desmond Gold, modified by community. See post 'Timeline' for change
// history Retrieved 2026-04-15, License - CC BY-SA 4.0

#include <source_location>
#include <string_view>

namespace Utility {

template <typename T> consteval auto func_name() {
  const auto &loc = std::source_location::current();
  return loc.function_name();
}

template <typename T> consteval std::string_view type_of_impl_() {
  constexpr std::string_view functionName = func_name<T>();
  // since func_name_ is 'consteval auto func_name() [with T = ...]'
  // we can simply get the subrange
  // because the position after the equal will never change since
  // the same function name is used

  // another notice: these magic numbers will not work on MSVC
  return {functionName.begin() + 37, functionName.end() - 1};
}

template <typename T> constexpr auto type_of(T &&arg) {
  return type_of_impl_<decltype(arg)>();
}

template <typename T> constexpr auto type_of() { return type_of_impl_<T>(); }

} // namespace Utility
