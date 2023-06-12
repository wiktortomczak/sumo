
#pragma once

#include <utility>

#include "lib/log_interface.h"


template <typename StreamT, typename MessageFormatT>
class StreamLog : public LogInterface<StreamLog<StreamT, MessageFormatT>> {
public:
  template <typename... Ts>
  void LogMessage(Message<Ts...>&& message) volatile {
    MessageFormatT::template WriteToStream<StreamT>(std::move(message));
  }

  // TODO: Remove.
  template <typename... Ts>
  static void LogMessage(const Message<Ts...>& message) {
    MessageFormatT::template WriteToStream<StreamT>(message);
  }
};
