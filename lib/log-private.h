
#pragma once

namespace internal {
namespace log {

struct NullStream {
  template <typename... Args>
  const NullStream& BeginMessage(Args&&... args) const { return *this; }

  template <typename... Args>
  const NullStream& BeginAsyncMessage(Args&&... args) const { return *this; }

  template <typename T>
  const NullStream& operator<<(T&& t) const { return *this; }
};

}  // namespace log
}  // namespace internal
