
#pragma once

#include <cstring>
#include <string_view>

#include "arduino-ext/pgm.h"
#include "lib/log_interface.h"
#include "lib/stream_log.h"


namespace internal {
namespace text_log {

struct TextFormat;

}  // namespace text_log
}  // namespace internal


template <typename StreamT>
using TextLog = StreamLog<StreamT, internal::text_log::TextFormat>;


namespace internal {
namespace text_log {

template <typename StreamT>
struct TextStream;

struct TextFormat {
  template <typename StreamT, typename... Ts>
  static void WriteToStream(const Message<Ts...>& message) {
    TextStream<StreamT>::Write(message.header.severity);
    TextStream<StreamT>::WriteMicros(message.header.micros);
    TextStream<StreamT>::WriteThread(message.header.thread_id);
    StreamT::Write(' ');
    TextStream<StreamT>::Write(message.header.file_name);
    StreamT::Write(':');
    TextStream<StreamT>::Write(message.header.line_number);
    StreamT::Write(':');
    StreamT::Write(' ');
    Tuples::ForEach(message.args, []<typename T>(const T& t) {
      TextStream<StreamT>::Write(t);
    });
    StreamT::Write('\n');
    StreamT::Flush();
  }
};

template <typename StreamT>
struct TextStream {
  static void Write(int16_t i) {
    char message[7];
    const size_t len = ::snprintf(message, sizeof(message), P("%d"), i);
    StreamT::Write(message, len);
  }

  static void Write(uint16_t i) {
    char message[6];
    const size_t len = ::snprintf(message, sizeof(message), P("%u"), i);
    StreamT::Write(message, len);
  }

  static void Write(uint32_t i) {
    char message[11];
    const size_t len = ::snprintf(message, sizeof(message), P("%lu"), i);
    StreamT::Write(message, len);
  }

  static void Write(const char* message) {
    StreamT::Write(message, ::strlen(message));
  }

  // static void Write(std::string_view sv) {
  //   StreamT::Write(sv.data(), sv.size());
  // }

  static void Write(const PGM<char>* arg) {
    const uint8_t len = ::strlen(arg);
    char buf[len];
    // TODO: 
    // ::memcpy(buf, arg, len);
    ::memcpy_P(buf, PGM_rawptr(arg), len);
    StreamT::Write(buf, len);
  }

  static void Write(char c) {
    StreamT::Write(c);
  }

  static void Write(Severity severity) {
    switch (severity) {
    case Severity::INFO:
      StreamT::Write('I');
      break;
    case Severity::FATAL:
      StreamT::Write('F');
      break;
    }
    LOG(FATAL) << "Internal error";
  }

  static void WriteMicros(uint32_t micros) {
    char message[12];
    const size_t len = ::snprintf(
      message, sizeof(message), P("%04u.%06lu"),
      static_cast<uint16_t>(micros / 1000000L), micros % 1000000L);
    StreamT::Write(message, len);
  }

  static void WriteThread(Thread::Id thread_id) {
    StreamT::Write(thread_id == Thread::Id::INTERRUPT ? '*' : ' ');
  }

  static void Write(void* p) {
    char buf[16];
    const size_t len = ::snprintf(buf, sizeof(buf), P("%p"), p);
    StreamT::Write(buf, len);
  }
};

}  // namespace text_log
}  // namespace internal
