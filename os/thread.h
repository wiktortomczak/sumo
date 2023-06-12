
#pragma once

class Thread {
public:
  enum class Id : uint8_t {
    MAIN,
    INTERRUPT
  };

  static volatile Id id;

  static bool is_interrupt() {
    return id == Id::INTERRUPT;
  }

  class Indicator {
  public:
    Indicator(Thread::Id thread_id) : previous_thread_id_(Thread::id) {
      Thread::id = thread_id;
    }
    ~Indicator() { Thread::id = previous_thread_id_; }

  private:
    const Thread::Id previous_thread_id_;
  };
};

inline volatile Thread::Id Thread::id;

inline Thread::Indicator main_thread(Thread::Id::MAIN);
