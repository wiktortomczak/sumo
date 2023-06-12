
#include <cassert>
#include <string>
#include <vector>

#include "lib/tuples.h"

using namespace std;


template <typename T>
string ToString(T t);

template <>
string ToString<int>(int t) {
  assert(t == 1);
  return "1";
}

template <>
string ToString<bool>(bool t) {
  return t ? "true" : "false";
}


int main() {
  {
    vector<string> s;
    Tuples::ForEach(make_tuple(1, true), [&]<typename T>(const T& t) {
      s.push_back(ToString<T>(t));
    });
    assert(s == vector<string>({"1", "true"}));
  }

  // {
  //   const auto t = Tuples::Map(make_tuple(1, true), []<typename T>(const T& t) {
  //     return ToString<T>(t);
  //   });
  //   assert(t == make_tuple("1", "true"));
  // }
}
