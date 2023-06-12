#pragma once


namespace internal {
namespace buffered_log {

template <typename LogT, typename... Ts>
size_t WriteToLog(const Message<Ts...>& message);

using WriteToLog_t = size_t(*)(const void*);

template <typename... Ts>
size_t SizeInBuffer(const Message<Ts...>& message) {
  return sizeof(WriteToLog_t) + sizeof(message);
}

template <typename LogT, typename BufferT, typename... Ts>
void WriteToBuffer(Message<Ts...>&& message, BufferT* buf) {
  buf->push_back(&WriteToLog<LogT, Ts...>);
  buf->push_back(message);
}

template <typename BufferT>
void MoveFromBufferToLog(BufferT* buf) {
  const WriteToLog_t write_to_log_func =
    buf->template front<WriteToLog_t>();
  const void* const message = &buf->peek(sizeof(write_to_log_func));

  const size_t message_size_in_buffer = write_to_log_func(message);

  buf->pop_front(message_size_in_buffer);
}

template <typename LogT, typename... Ts>
size_t WriteToLog(const Message<Ts...>& message) {
  LogT::template LogMessage(message);
  return SizeInBuffer(message);
}

}  // namespace buffered_log
}  // namespace internal
