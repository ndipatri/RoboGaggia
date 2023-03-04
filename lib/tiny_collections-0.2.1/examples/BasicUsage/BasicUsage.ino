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


