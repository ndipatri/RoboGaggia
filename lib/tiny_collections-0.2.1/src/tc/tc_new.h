// (c) Copyright 2022 Aaron Kimball
//
// Define signatures for placement new; avoid requiring #include<new>.

#ifndef _TC_NEW_H
#define _TC_NEW_H

#include<stddef.h> // for size_t.

// Placement-new operator definition used within tc::vector<T>. As long as we do not
// construct the (impossible) tc::vector<void> this can be used in tc::vector<>T without
// us needing to `#include<new>`, but also does not conflict with the placement
// new function defined in <new> in case the user includes that file elsewhere.
template<typename T> inline void* operator new(size_t size, T *pMem) noexcept {
  return pMem;
}

// Forward-declare the void-specialized alternative found in <new> without defining it,
// to ensure the template above is not instantiated for T=void.
template<> inline void* operator new<void>(size_t size, void *pMem) noexcept;

#endif

