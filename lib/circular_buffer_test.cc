
#include <utility>
#include <vector>

#define TEST_CRITICAL_SECTION(code) code  // TODO

#include "lib/circular_buffer.h"

using namespace std;


template <typename BufferT>
void AssertElements(BufferT& buffer, initializer_list<int> elements) {
  assert(std::equal(
    buffer.begin(), buffer.end(),
    elements.begin(), elements.end()));
}

int main() {
  {
    CircularBuffer<int, 3> buf;
    assert(buf.empty());
    assert(!buf.full());
    assert(buf.size() == 0);
    AssertElements(buf, {});
 
    buf.push_back(1);
    assert(buf.front() == 1);
    assert(buf.size() == 1);
    assert(!buf.empty());
    assert(!buf.full());
    AssertElements(buf, {1});

    buf.push_back(2);
    assert(buf.size() == 2);
    AssertElements(buf, {1, 2});

    buf.push_back(3);
    assert(buf.size() == 3);
    assert(!buf.empty());
    assert(buf.full());
    AssertElements(buf, {1, 2, 3});

    assert(buf.front() == 1);

    buf.pop_front();
    assert(buf.front() == 2);
    assert(buf.size() == 2);
    assert(!buf.empty());
    assert(!buf.full());
    AssertElements(buf, {2, 3});
    
    buf.pop_front();
    assert(buf.front() == 3);
    assert(buf.size() == 1);
    assert(!buf.empty());
    assert(!buf.full());
    AssertElements(buf, {3});

    buf.pop_front();
    assert(buf.size() == 0);
    assert(buf.empty());
    assert(!buf.full());
    AssertElements(buf, {});

    buf.push_back(4);
    buf.pop_front();
    assert(buf.size() == 0);
    assert(buf.empty());
    assert(!buf.full());
    AssertElements(buf, {});

    buf.push_back(5);
    buf.push_back(6);
    buf.push_back(7);
    assert(buf.front() == 5);
    assert(buf.size() == 3);
    assert(!buf.empty());
    assert(buf.full());
    AssertElements(buf, {5, 6, 7});
    
    buf.pop_front();
    assert(buf.front() == 6);
    assert(buf.size() == 2);
    assert(!buf.empty());
    assert(!buf.full());
    AssertElements(buf, {6, 7});

    buf.pop_front();
    assert(buf.front() == 7);
    assert(buf.size() == 1);
    assert(!buf.empty());
    assert(!buf.full());
    AssertElements(buf, {7});

    buf.pop_front();
    assert(buf.size() == 0);
    assert(buf.empty());
    assert(!buf.full());
    AssertElements(buf, {});
  }

  {
    CircularBuffer<int, 3> buf;
    buf.push_back(1);
    buf.push_back(2);
    buf.push_back(3);
    auto it = buf.begin();
    *it = 4;
    AssertElements(buf, {4, 2, 3});
    ++it; ++it;
    *(it++)= 5;
    AssertElements(buf, {4, 2, 5});
    assert(it == buf.end());
  }

  // TODO: Test multithreaded volatile.
  
  return 0;
}
