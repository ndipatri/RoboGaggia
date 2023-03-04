
tiny-collections
================

A minimalist templated collections library for embedded C++ applications.

Compiling
---------

Compilation is not necessary; just `#include <tiny-collections.h>` in your code.

Unit tests exist in the `test/` subdir. Run `make test` in this directory to run
the unit test suite.

Classes
-------

This library includes stripped down versions of STL template classes that may be useful
for embedded programming where compiled code space is at a premium.

* **tc::vector&lt;T&gt;** - A variable-length, array-backed collection of elements of type `T`
  with O(1) random and sequential access, O(1) item-append time, and O(n) mid-list
  insertion and removal time. If necessary, the backing array may be resized by allocating a larger
  array and relocating existing elements with `memmove()`.
* **tc::const_array&lt;T&gt;** - A fixed-length immutable array of `const` items of type `T`
  Like `vector`, this class supports indexed access, iteration, and reporting on its size.
  Since access is only given to elements as `const T&`, and the `const_array` class does
  not assume ownership of the `T` elements (i.e., its destructor will not call `~T()` on
  its elements) this can be created as a `constexpr` object that wraps a brace-initialized
  list of items.

Usage
-----

Both classes are in the `tc` namespace. You should refer to the fully-qualified class name
(`tc::vector<T>`) or include the statement `using namespace tc;` after including the
library.

These classes never deliberately `throw` any exception. Indexed dereference
(`myVector[i]`) is an unchecked operation performed directly against the underlying array.
* Dereference of an index greater than or equal to `size()` (or less than 0) is undefined
  behavior.
* Because it does not include any methods that throw exceptions, this does not include the
  `std::vector<T>::at(size_t i)` function that performs checked element access. Instead, use
  `bool tc::vector<T>::in_range(size_t i)` to check whether it is safe to access the
  `i`'th element.

Because these are template classes, including this library has no immediate impact on
compiled code size. However, each distinct template instantiation (`vector<int>`,
`vector<long>`, `vector<String>`, etc...) will generate additional code and compiled
sketch size. Only the methods you use/need will be included in the final compiled sketch;
e.g. if you never use the `erase()` method in a particular instantiation of `vector<T>`,
its code will not add any size to your compiled sketch.

Example
-------

```c++
#include <Arduino.h>
#include <tiny-collections.h>

using namespace tc; // Import tc::* into the global namespace

// Create a const int array and wrap it in a useful interface.
// This is similar to declaring `const int myFixedArray[4] = { 1, 2, 3, 4 };`
// but lets you use methods like `size()`.
constexpr const_array<int> myFixedArray = { 1, 2, 3, 4 };

void useImmutableArray() {
  // This prints the following to the terminal:
  //
  //   Fixed array includes 4 items:
  //   1 2 3 4

  Serial.print("Fixed array includes ");
  Serial.print(myFixedArray.size());
  Serial.println(" items:");

  // const_array<T> supports the iterator interface:
  for (int *it = myFixedArray.begin(); it != myFixedArray.end(); it++) {
    Serial.print(*it);
    Serial.print(" ");
  }
  Serial.println("");

}

// Create an expandable array of Strings.
// const_array<T>, vector<T> work with any basic scalar types or classes.
vector<String> someStrings;

void setup() {
  // Add strings to the `someStrings` vector.
  someStrings.push_back(String("foo")); // appends the string
  someStrings.push_back("bar"); // invokes String(const char*) constructor and appends it.

  Serial.print("Number of strings: ");
  Serial.println(someStrings.size()); // number of dereferenceable items

  Serial.print("Capacity: ");
  Serial.println(someStrings.capacity()); // total backing array size, may be larger.

  Serial.print("First string: ");
  Serial.println(someStrings[0]); // dereference works like an array.

  // Alternate iteration interface using 'foreach' syntax.
  Serial.println("All strings:");
  for (auto str: someStrings) {
    Serial.println(str);
  }

  useConstantArray();
}

void loop() {
  delay(10);
}
```

Interface
---------

### tc::const\_array&lt;T&gt;

```c++
template<typename T> class const_array {
public:
  /** Construct a zero-length array. */
  constexpr const_array();

  /** Copy constructor. */
  constexpr const_array(const const_array<T> &other);

  /** Copy constructor from explicit array. */
  template<size_t N> constexpr const_array(const T (&arr)[N]);

  /** Copy constructor from brace-initialized list. */
  constexpr const_array(std::initializer_list<T> lst);

  /** Move constructor; essentially the same as "copy" since `other` remains immutable. */
  constexpr const_array(const_array<T> &&other) noexcept;

  // No copy operator; array components are fixed.
  const_array<T> &operator=(const_array<T> &copy_src) = delete;

  // No move operator; array components are fixed.
  const_array<T> &operator=(const_array<T> &&move_src) = delete;

  // Indexed dereference to `const` elements.
  const T& operator[](size_t idx) const;

  size_t size() const; // number of valid elements.
  size_t capacity() const; // same as size().
  bool empty() const; // true if size() == 0.

  /** Return true if you can use this subscript index safely. */
  bool in_range(size_t idx) const;

  // C++ iterator interface.
  const T* begin() const;
  const T* end() const;
};

```

### tc::vector&lt;T&gt;

```c++
template<typename T> class vector {
public:
  vector();
  vector(size_t initial_cap);
  vector(const vector<T> &other); /** Copy constructor. */
  template<size_t N> vector(const T (&arr)[N]); /** Copy constructor from explicit array. */
  vector(const const_array<T> &arr); /** Copy constructor from const_array<T>. */
  vector(std::initializer_list<T> lst); /** Copy constructor from brace-initialized list. */
  vector(vector<T> &&other) noexcept; /** Move constructor. */
  ~vector();

  // Move operator; assume move_src's data.
  vector<T> &operator=(vector<T> &&move_src) noexcept;
  // Copy operator: duplicate the elements of another vector<T>.
  vector<T> &operator=(const vector<T> &copy_src);
  // Copy operator: duplicate the elements of a const_array<T>
  vector<T> &operator=(const const_array<T> &copy_src);

  // Indexed dereference
  const T& operator[](size_t idx) const;
  T& operator[](size_t idx);

  size_t size() const; // number of valid elements.
  size_t capacity() const; // number of memory slots provisioned in array.
  bool empty() const; // true if size() == 0
  void reserve(size_t minCapacity); // Ensure minimum size of backing array.
  void clear(); // discard elements so size() == 0

  // Return true if you can use this subscript index safely.
  // tc::vector<T> does not include the `at(size_t i)` function from std::vector,
  // since it never throws exceptions.
  bool in_range(size_t idx) const;

  // Append item to the vector, allocating more space if necessary. Returns the
  // index where the item was stored.
  size_t push_back(const T& item);

  // Insert item at position `pos`, moving `pos`..`_count-1` down by 1.
  size_t insert_at(const T& item, size_t pos);

  // C++ iterator interface.
  const T* begin() const;
  T* begin();
  const T* end() const;

  // Erase the value at the iterator location.
  void erase(T* atIter);
};
```

Compatibility
=============

This library has no hardware-specific requirements or constraints on its usage. This
should be compatible with any modern `g++` or `avr-g++` implementation or MCU.

While most dependencies from the C++ STL have been eliminated, some are unavoidable,
like `std::initializer_list<T>`.  A port of `std::initializer_list<T>` is included and used if
the `__AVR__` preprocessor macro is defined. You should not otherwise include
`tc/tc_initializer_list.h` yourself.

License
-------

This library is available under the terms of the BSD 3-clause license. See the
[LICENSE.txt](https://github.com/kimballa/tiny-collections/blob/main/LICENSE.txt) file for
complete details.
