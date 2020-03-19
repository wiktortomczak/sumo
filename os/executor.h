
#pragma once


// Interface that provides the capacity to run a given piece of code,
// not necessarily in the current thread / call stack.
// Abstraction of a share of CPU time, eg. a thread.
class Executor {
public:
  // Runs given callable asynchronously - the callable may be called after
  // RunAsync() returns. If a return value / completion notification is needed,
  // the callable must implement it itself.
  // Note: All pointers / references used by the callable must be valid until
  // the callable is called, which will happen at some unspecified time
  // following the call to RunAsync(). A common cause of segmentation fault is
  // the callable taking a reference to an automatic variable from its own
  // enclosing scope, deallocated from the stack before the callable is called.
  template <typename F, typename... Args>
  void RunAsync(F&& callable, Args&&... args);
};
