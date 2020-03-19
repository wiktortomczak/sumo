
#include <cstdio>
#include <memory>

#include "os/testing/sequential_executor.h"
#include "lib/promise.h"


int main() {
  {
    int a = 1;
    PromiseWithResolve<int> p;
    p.ThenVoid([&a](int i) { a += i; });
    p.Resolve(2);
    executor_.Loop();
    assert(a == 3);
  }

  {
    bool resolved = false;
    PromiseWithResolve<void> p;
    p.ThenVoid([&resolved]() { resolved = true; });
    p.Resolve();
    executor_.Loop();
    assert(resolved);
  } 

  {
    int a = 0;
    PromiseWithResolve<int> p1;
    p1.Then<int>([](int i) { return i + 1; })
      .ThenVoid([&a](int i) { a = i; });
    executor_.Loop();
    assert(a == 0);
    p1.Resolve(1);
    executor_.Loop();
    assert(a == 2);
  }

  {
    bool resolved = false;
    PromiseWithResolve<void> p1;
    p1.Then<void>([]() { })
      .ThenVoid([&resolved]() { resolved = true; });
    executor_.Loop();
    assert(!resolved);
    p1.Resolve();
    executor_.Loop();
    assert(resolved);
  }

  {
    int a = 1;
    PromiseWithResolve<int> p1;
    p1.Then<int>([](int i) {
      return Promise<int>::Resolved(i);
    }).ThenVoid([&a](int i) {
      a += i;
    });
    executor_.Loop();
    assert(a == 1);
    p1.Resolve(2);
    executor_.Loop();
    assert(a == 3);
  }

  {
    bool resolved = false;
    PromiseWithResolve<void> p1;
    // TODO: Remove static_cast.
    p1.Then<void>(static_cast<std::function<Promise<void>()>>(
      []() { return Promise<void>::Resolved(); }))
      .ThenVoid([&resolved]() { resolved = true; });
    executor_.Loop();
    assert(!resolved);
    p1.Resolve();
    executor_.Loop();
    assert(resolved);
  }

  {
    int a = 1;
    PromiseWithResolve<int> p1;
    PromiseWithResolve<int> p2;
    p1.Then<int>([&p2](int) {
      // TODO: Remove the 2 extra copy (?) constructor calls.
      return p2;
    }).ThenVoid([&a](int i) { a += i; });
    executor_.Loop();
    assert(a == 1);
    p1.Resolve(-1);
    executor_.Loop();
    assert(a == 1);
    p2.Resolve(2);
    executor_.Loop();
    assert(a == 3);
  }

  {
    bool resolved = false;
    PromiseWithResolve<void> p1;
    PromiseWithResolve<void> p2;
    // TODO: Remove static_cast.
    p1.Then<void>(static_cast<std::function<Promise<void>()>>(
      [&p2]() {
        // TODO: Remove the 2 extra copy (?) constructor calls.
        return p2;
      }))
      .ThenVoid([&resolved]() { resolved = true; });
    executor_.Loop();
    assert(!resolved);
    p1.Resolve();
    executor_.Loop();
    assert(!resolved);
    p2.Resolve();
    executor_.Loop();
    assert(resolved);
  }
 
  {
    int a = 0;
    Promise<int>::Resolved(1).ThenVoid([&a](int i) {
      a = i;
    });
    assert(a == 0);
    executor_.Loop();
    assert(a == 1);
  }

  {
    bool resolved = false;
    Promise<void>::Resolved().ThenVoid([&resolved]() {
      resolved = true;
    });
    assert(!resolved);
    executor_.Loop();
    assert(resolved);
  }

  {
    int i = 1; int *ip = &i;
    auto promise = std::make_unique<PromiseWithResolve<void>>();
    promise
      ->Then<void>([&promise]() { promise.reset(); })
      .ThenVoid([ip]() { assert(ip); *ip = 2; });
    promise->Resolve();
    executor_.Loop();
    assert(i == 2);
  }

  {
    int a = 0;
    Promise<int>::Resolved(1).Then<void>([&a](int i) { a = i; });
    executor_.Loop();
    assert(a == 1);
  }

  return 0;
}
