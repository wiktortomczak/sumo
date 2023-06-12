
#pragma once

#include "lib/stream_log.h"

#include <cstring>
#include <tuple>
#include <string_view>

#include "arduino-ext/pgm.h"
#include "lib/template_metaprogramming.h"
#include "lib/tuples.h"


enum class ValueType : uint8_t {
  UINT8 = 1,
  UINT16 = 2,
  UINT32 = 3,
  STRING = 4
};

namespace internal {
namespace binary_log {
struct BinaryFormat;
}  // namespace binary_log
}  // namespace internal


template <typename StreamT>
using BinaryLog = StreamLog<StreamT, internal::binary_log::BinaryFormat>;


namespace internal {
namespace binary_log {


template <typename T>
ValueType binary_value_type_v;

template <typename... Ts>
size_t MessageBinarySize(const Message<Ts...>& message);

template <typename T>
size_t BinarySize(const T& t);

template <typename StreamT, typename T>
void WriteBinaryToStream(const T& t);


struct BinaryFormat {
  template <typename StreamT, typename... Ts>
  static void WriteToStream(const Message<Ts...>& message) {
    WriteBinaryToStream<StreamT, uint16_t>(MessageBinarySize(message));

    WriteBinaryToStream<StreamT>(message.header.micros);
    WriteBinaryToStream<StreamT>(message.header.file_name);
    WriteBinaryToStream<StreamT>(message.header.line_number);
    WriteBinaryToStream<StreamT>(message.header.severity);

    const uint8_t num_args = sizeof...(Ts);
    WriteBinaryToStream<StreamT>(num_args);
    Tuples::ForEach(message.args, []<typename T>(const T& t) {
      WriteBinaryToStream<StreamT>(static_cast<char>(binary_value_type_v<T>));
      WriteBinaryToStream<StreamT, T>(t);
    });

    StreamT::Flush();
  }
};

template <typename... Ts>
size_t MessageBinarySize(const Message<Ts...>& message) {
  const size_t header_size = 
    BinarySize(message.header.micros)
    + BinarySize(message.header.file_name)
    + BinarySize(message.header.line_number)
    + BinarySize(message.header.severity);
  const size_t args_size = std::apply([](const Ts&... ts) {
      return (0 + ... + (BinarySize<Ts>(ts) + 1));
  }, message.args);
  return header_size + 1 + args_size;
}


template <typename T> struct BinaryValue;

template <typename T>
size_t BinarySize(const T& t) {
  return BinaryValue<T>::Size(t);
}

template <typename StreamT, typename T>
void WriteBinaryToStream(const T& t) {
  BinaryValue<T>::template WriteToStream<StreamT>(t);
}


template <typename T>
struct BinaryValue {
  static size_t Size(const T& arg) {
    return sizeof(arg);
  }

  template <typename StreamT>
  static void WriteToStream(const T& arg) {
    StreamT::Write(&arg, sizeof(arg));
  }
};

// Define only in test, to prevent non-PGM strings in production code.
#if defined TEST_PGM
template <>
struct BinaryValue<const char*> {
  static size_t Size(const char* arg) {
    return std::strlen(arg) + 1;
  }

  template <typename StreamT>
  static void WriteToStream(const char* arg) {
    const uint8_t len = std::strlen(arg);
    StreamT::Write(len);
    StreamT::Write(arg, len);
  }
};
#endif  // defined TEST_PGM

template <>
struct BinaryValue<char*> {
  static size_t Size(char* arg) {
    return std::strlen(arg) + 1;
  }

  template <typename StreamT>
  static void WriteToStream(char* arg) {
    const uint8_t len = std::strlen(arg);
    StreamT::Write(len);
    StreamT::Write(arg, len);
  }
};

#if ! defined TEST_PGM
template <>
struct BinaryValue<const PGM<char>*> {
  static size_t Size(const PGM<char>* arg) {
    return ::strlen(arg) + 1;
  }

  template <typename StreamT>
  static void WriteToStream(const PGM<char>* arg) {
    const uint8_t len = ::strlen(arg);
    char buf[len];
    ::memcpy_P(buf, PGM_rawptr(arg), len);  // TODO
    StreamT::Write(len);
    StreamT::Write(buf, len);
  }
};
#endif  // ! defined TEST_PGM

// template <>
// struct BinaryValue<std::string_view> {
//   static size_t Size(std::string_view arg) {
//     return arg.size() + 1;
//   }

//   template <typename StreamT>
//   static void WriteToStream(std::string_view arg) {
//     StreamT::Write(arg.size());
//     StreamT::Write(arg.data(), arg.size());
//   }
// };


template <typename T>
struct binary_value_type;

template <>
struct binary_value_type<uint8_t> {
  static constexpr ValueType value = ValueType::UINT8;
};

template <>
struct binary_value_type<uint16_t> {
  static constexpr ValueType value = ValueType::UINT16;
};

template <>
struct binary_value_type<uint32_t> {
  static constexpr ValueType value = ValueType::UINT32;
};

template <>
struct binary_value_type<char*> {
  static constexpr ValueType value = ValueType::STRING;
};

template <>
struct binary_value_type<const char*> {
  static constexpr ValueType value = ValueType::STRING;
};

#if ! defined TEST_PGM
template <>
struct binary_value_type<const PGM<char>*> {
  static constexpr ValueType value = ValueType::STRING;
};
#endif  // ! defined TEST_PGM

template <>
struct binary_value_type<std::string_view> {
  static constexpr ValueType value = ValueType::STRING;
};

template <typename T>
ValueType binary_value_type_v = binary_value_type<T>::value;


}  // namespace binary_log
}  // namespace internal
