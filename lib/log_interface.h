
#pragma once

#include <tuple>
#include <utility>

#include "arduino-ext/pgm.h"
#include "lib/tuples.h"
#include "os/thread.h"


enum class Severity : uint8_t {
  FATAL = 1,
  INFO = 2
};


struct MessageHeader {
  uint32_t micros;
  const PGM<char>* file_name;
  uint16_t line_number;
  Thread::Id thread_id;
  Severity severity;
} __attribute__((packed));

template <typename... Ts>
struct Message {
  MessageHeader header;
  std::tuple<Ts...> args;
};



template <typename MessageSinkT>
class LogInterface {
public:
  template <typename... Ts>
  class MessageBuilder;

  MessageBuilder<> BeginMessage(
    Severity severity, uint32_t micros, Thread::Id thread_id,
    const PGM<char>* file_name, uint16_t line_number) volatile {
    return MessageBuilder<>(
      this, severity, micros, thread_id,
      file_name, line_number, false /* is_async */);
  }

  MessageBuilder<> BeginAsyncMessage(
    Severity severity, uint32_t micros, Thread::Id thread_id,
    const PGM<char>* file_name, uint16_t line_number) volatile {
    return MessageBuilder<>(
      this, severity, micros, thread_id,
      file_name, line_number, true /* is_async */);
  }

  template <typename... Ts>
  class MessageBuilder {
  public:
    template <typename T>
    MessageBuilder<Ts..., std::decay_t<T>> operator<<(T&& t) && {
      return MessageBuilder<Ts..., std::decay_t<T>>(
        move_ptr(log_), std::move(header_),
        (std::move(args_).template Add<T, true>(std::forward<T>(t))),
        is_async_);
    }

    ~MessageBuilder() {
      if (log_) {  // This MessageBuilder instance is the last one in sequence.
        if (!is_async_) {
          static_cast<volatile MessageSinkT*>(log_)->
            template LogMessage(BuildMessage());
        } else {
          // // TODO
          // RunAsync([log = log_, message = BuildMessage()]() {
          //   static_cast<volatile MessageSinkT*>(log)->
          //     template LogMessage(std::move(message));
          // });
        }
      }
    }

  private:
    MessageBuilder(volatile LogInterface* log,
                   Severity severity, uint32_t micros, Thread::Id thread_id,
                   const PGM<char>* file_name, uint16_t line_number,
                   bool is_async)
      : log_(log),
        header_({micros, file_name, line_number, thread_id, severity}),
        is_async_(is_async) {}

    MessageBuilder(volatile LogInterface* log, const MessageHeader& header,
                   TupleBuilder<Ts...>&& args, bool is_async)
      : log_(log), header_(header), args_(args), is_async_(is_async) {}

    Message<Ts...> BuildMessage() {
      return {header_, args_.Build()};
    }

    template <typename T>
    static T* move_ptr(T*& ptr) {
      T* const ptr_copy = ptr;
      ptr = nullptr;
      return ptr_copy;
    }

    volatile LogInterface* log_;
    const MessageHeader header_;
    TupleBuilder<Ts...> args_;
    const bool is_async_;

    friend class LogInterface;
  };

  template <typename... Ts>
  friend class MessageBuilder;
};
