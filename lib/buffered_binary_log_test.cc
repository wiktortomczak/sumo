
#include <cstring>
#include <vector>

#include "arduino-ext/testing/test_pgm.h"
#include "lib/binary_log.h"
#include "lib/buffered_log.h"

using namespace std;


struct Stream {
  static void Write(char c) {
    *(p_++) = c;
  }

  static void Write(const void* s, size_t len) {
    std::memcpy(p_, s, len);
    p_ += len;
  }

  static void Reset() {
    p_ = buf_;
  }

  static void Assert(vector<uint8_t> expected) {
    assert(vector<uint8_t>(buf_, p_) == expected);
  }

// private:
  static char buf_[1024];
  static char* p_;
};

char Stream::buf_[1024];
char* Stream::p_;


int main() {
  BinaryLog<Stream> binary_log;
  
  {
    Stream::Reset();
    binary_log.BeginMessage(Severity::INFO, 65535, "dir/file.cc", 15)
      << static_cast<uint8_t>(1);
    Stream::Assert({
      0x16, 0x00,
      0xFF, 0xFF, 0x00, 0x00,
      0x0B, 'd', 'i', 'r', '/', 'f', 'i', 'l', 'e', '.', 'c', 'c',
      0x0F, 0x00,
      0x02, 0x01,
      static_cast<uint8_t>(ValueType::UINT8), 0x01
    });
  }

  {
    BufferedLog<BinaryLog<Stream>, 1024> buffered_log;
    Stream::Reset();

    buffered_log.BeginMessage(Severity::INFO, 65535, "dir/file.cc", 15)
      << static_cast<uint8_t>(1);
    Stream::Assert({});
    buffered_log.Flush();
    Stream::Assert({
      0x16, 0x00,
      0xFF, 0xFF, 0x00, 0x00,
      0x0B, 'd', 'i', 'r', '/', 'f', 'i', 'l', 'e', '.', 'c', 'c',
      0x0F, 0x00,
      0x02, 0x01,
      static_cast<uint8_t>(ValueType::UINT8), 0x01
    });

    Stream::Reset();
    uint16_t _2 = 2;
    uint16_t& _2ref = _2;
    const string_view sv = "sv";
    buffered_log.BeginMessage(Severity::FATAL, 65535, "dir/file.cc", 15)
      << "abc" << sv << _2ref;
    Stream::Assert({});
    buffered_log.Flush();
    Stream::Assert({
      0x20, 0x00,
      0xFF, 0xFF, 0x00, 0x00,
      0x0B, 'd', 'i', 'r', '/', 'f', 'i', 'l', 'e', '.', 'c', 'c',
      0x0F, 0x00,
      0x01, 0x03,
      static_cast<uint8_t>(ValueType::STRING), 0x03, 'a', 'b', 'c',
      static_cast<uint8_t>(ValueType::STRING), 0x02, 's', 'v',
      static_cast<uint8_t>(ValueType::UINT16), 0x02, 0x00
    });
  }

  {
    BufferedLog<BinaryLog<Stream>, 32> buffered_log;
    Stream::Reset();

    buffered_log.BeginMessage(Severity::INFO, 65535, "dir/file.cc", 15)
      << static_cast<uint8_t>(1);
    Stream::Assert({});
    buffered_log.BeginMessage(Severity::INFO, 65535, "dir/file.cc", 15)
      << static_cast<uint8_t>(2);
    Stream::Assert({
      0x16, 0x00,
      0xFF, 0xFF, 0x00, 0x00,
      0x0B, 'd', 'i', 'r', '/', 'f', 'i', 'l', 'e', '.', 'c', 'c',
      0x0F, 0x00,
      0x02, 0x01,
      static_cast<uint8_t>(ValueType::UINT8), 0x01
    });
    
    Stream::Reset();
    buffered_log.Flush();
    Stream::Assert({
      0x16, 0x00,
      0xFF, 0xFF, 0x00, 0x00,
      0x0B, 'd', 'i', 'r', '/', 'f', 'i', 'l', 'e', '.', 'c', 'c',
      0x0F, 0x00,
      0x02, 0x01,
      static_cast<uint8_t>(ValueType::UINT8), 0x02
    });
  }

  return 0;
}
