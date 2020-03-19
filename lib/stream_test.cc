
#include "os/testing/sequential_executor.h"
#include "lib/stream.h"


struct StreamSource {
  Stream<int> GetStream() const { return stream_; }
  void Put(int i) { stream_.Put(i); }
  Stream<int> stream_;
};


int main() {
  {
    StreamSource source;
    Stream<int> stream = source.GetStream();
    int a = 1;
    stream.ThenEvery([&a](int i) { a += i; });
    source.Put(2);
    assert(a == 1);
    executor_.Loop();
    assert(a == 3);
    source.Put(3);
    executor_.Loop();
    assert(a == 6);
  }
  
  return 0;
}
