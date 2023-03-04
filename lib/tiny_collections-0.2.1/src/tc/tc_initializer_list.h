// (c) Copyright 2022 Aaron Kimball
//
// If we are in a C++ environment without the C++ standard library (e.g. avr)
// implement the necessary subset for tiny collections.

#ifndef _TC_INITIALIZER_LIST
#define _TC_INITIALIZER_LIST

// If no #include<initializer_list> present:
#ifndef _INITIALIZER_LIST
#define _INITIALIZER_LIST

#include<stddef.h> // for size_t.

namespace std {
  template<class T> class initializer_list {
  public:
    // Standard member types.
    typedef T value_type;
    typedef const T& reference;
    typedef const T& const_reference;
    typedef size_t size_type;
    typedef const T* iterator;
    typedef const T* const_iterator;

    // Standard public API.
    constexpr initializer_list() noexcept: _first(NULL), _count(0) {};

    constexpr size_type size() const noexcept { return _count; };
    constexpr const_iterator begin() const noexcept { return _first; };
    constexpr const_iterator end() const noexcept { return _first + _count; };

  private:
    // Users should not create non-zero-len initializer lists directly.
    constexpr initializer_list(const value_type *start, size_type N) noexcept:
        _first(start), _count(N) {};

    iterator _first;
    size_t _count;
  }; // class initializer_list<T>

  template<class T> constexpr const T* begin(initializer_list<T> lst) noexcept {
    return lst.begin();
  };

  template<class T> constexpr const T* end(initializer_list<T> lst) noexcept {
    return lst.end();
  };
}; // namespace std

#endif // _INITIALIZER_LIST
#endif // _TC_INITIALIZER_LIST
