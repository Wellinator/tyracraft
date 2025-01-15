#pragma once
// =======================================================
// Simple singleton class
//
// Based on Scott Bilas's Singleton from Game Programming Gems:
// http://scottbilas.com/publications/gem-singleton/
//
// Usage:
//  class FooManager : public Singleton<FooManager>
//  {
//      ...
//  }
//
// =======================================================

#include <cassert>

template <typename T>
class Singleton {
  static T* ms_singleton;

 public:
  Singleton() {
    assert(!ms_singleton);
    ms_singleton = static_cast<T*>(this);
  }

  ~Singleton() {
    assert(ms_singleton);
    ms_singleton = 0;
  }

  static T& gerInstaceRef() {
    assert(ms_singleton);
    return *ms_singleton;
  }

  static T* getInstance() { return ms_singleton; }
};

template <typename T>
T* Singleton<T>::ms_singleton = 0;
