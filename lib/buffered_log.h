
#pragma once

#include <utility>

#include "lib/contiguous_buffer.h"
#include "lib/log_interface.h"

#include "lib/buffered_log-private.h"


template <typename LogT, size_t buffer_size>
class BufferedLog : public LogInterface<BufferedLog<LogT, buffer_size>> {
public:
  template <typename... Ts>
  void LogMessage(Message<Ts...>&& message) {  // TODO: volatile
    if (buf_.free_capacity() < internal::buffered_log::SizeInBuffer(message)) {
      Flush();
    }
    internal::buffered_log::WriteToBuffer<LogT>(std::move(message), &buf_);
  }

  void Flush() {
    while (!buf_.empty()) {
      internal::buffered_log::MoveFromBufferToLog(&buf_);
    }
    buf_.Reset();
  }

private:
  ContiguousBuffer<buffer_size> buf_;
};
