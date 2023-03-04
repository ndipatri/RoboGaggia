// (c) Copyright 2022 Aaron Kimball

#include <tiny-collections.h>
#include <iostream>
#include <string>

using namespace tc;

using std::cout;
using std::endl;
using std::string;

static int testFailures = 0;
static int testRuns = 0;

// Log that a test case failed.
static void failCase() {
  testRuns++;
  testFailures++;
  cout << ": *** FAILED ***" << endl;
}

// Log that a test case passed.
static void passCase() {
  testRuns++;
  cout << ": OK" << endl;
}

template<typename T> static void evalTestcase(const string &testName, T expectedValue, T actualValue) {
  cout << testName << ": (expected=" << expectedValue << ", actual=" << actualValue << ")";
  if (expectedValue != actualValue) {
    failCase();
  } else {
    passCase();
  }
}

template<typename T> static void expectEqual(const string &testName, T expectedValue, T actualValue) {
  evalTestcase(testName, expectedValue, actualValue);
}

template<typename T> static void expectNotEqual(const string &testName, T stockValue, T actualValue) {
  cout << testName << ": (expected !=" << stockValue << ", actual=" << actualValue << ")";
  if (stockValue == actualValue) {
    failCase();
  } else {
    passCase();
  }
}

template<typename T> static void expectGreater(const string &testName, T stockValue, T actualValue) {
  cout << testName << ": (expected >" << stockValue << ", actual=" << actualValue << ")";
  if (actualValue > stockValue) {
    passCase();
  } else {
    failCase();
  }
}

template<typename T> static void expectGreaterOrEqual(
    const string &testName, T stockValue, T actualValue) {

  cout << testName << ": (expected >=" << stockValue << ", actual=" << actualValue << ")";
  if (actualValue >= stockValue) {
    passCase();
  } else {
    failCase();
  }
}

template<typename T> static void expectLess(const string &testName, T stockValue, T actualValue) {
  cout << testName << ": (expected <" << stockValue << ", actual=" << actualValue << ")";
  if (actualValue < stockValue) {
    passCase();
  } else {
    failCase();
  }
}

template<typename T> static void expectLessOrEqual(
    const string &testName, T stockValue, T actualValue) {

  cout << testName << ": (expected <=" << stockValue << ", actual=" << actualValue << ")";
  if (actualValue <= stockValue) {
    passCase();
  } else {
    failCase();
  }
}

static int numSuites = 0;
static void startTestSuite(const string &suiteName) {
  if (numSuites > 0) {
    cout << endl;
  }
  cout << "Test suite start: " << suiteName << endl;
  cout << "=============================================" << endl;
  numSuites++;
}

void testTypeTraits() {
  startTestSuite("Type Traits");
  evalTestcase("True type value", true, tc_true_type::value());
  evalTestcase("False type value", false, tc_false_type::value());

  evalTestcase("Conditional true value", true,
      tc_conditional<true, tc_true_type, tc_false_type>::type::value());
  evalTestcase("Conditional false value", false,
      tc_conditional<false, tc_true_type, tc_false_type>::type::value());

  evalTestcase("AND()", true, tc_conjunction<>::value());
  evalTestcase("AND(true)", true, tc_conjunction<tc_true_type>::value());
  evalTestcase("AND(false)", false, tc_conjunction<tc_false_type>::value());
  evalTestcase("AND(true, true)", true,
      tc_conjunction<tc_true_type, tc_true_type>::value());
  evalTestcase("AND(true, false)", false,
      tc_conjunction<tc_true_type, tc_false_type>::value());
  evalTestcase("AND(false, true)", false,
      tc_conjunction<tc_false_type, tc_true_type>::value());
  evalTestcase("AND(false, false)", false,
      tc_conjunction<tc_false_type, tc_false_type>::value());
  evalTestcase("AND(true, true, true)", true,
      tc_conjunction<tc_true_type, tc_true_type, tc_true_type>::value());
  evalTestcase("AND(true, true, false)", false,
      tc_conjunction<tc_true_type, tc_true_type, tc_false_type>::value());
  evalTestcase("AND(true, false, true)", false,
      tc_conjunction<tc_true_type, tc_false_type, tc_true_type>::value());
  evalTestcase("AND(false, true, true)", false,
      tc_conjunction<tc_false_type, tc_true_type, tc_true_type>::value());

  evalTestcase("OR()", false, tc_disjunction<>::value());
  evalTestcase("OR(true)", true, tc_disjunction<tc_true_type>::value());
  evalTestcase("OR(false)", false, tc_disjunction<tc_false_type>::value());
  evalTestcase("OR(true, true)", true,
      tc_disjunction<tc_true_type, tc_true_type>::value());
  evalTestcase("OR(true, false)", true,
      tc_disjunction<tc_true_type, tc_false_type>::value());
  evalTestcase("OR(false, true)", true,
      tc_disjunction<tc_false_type, tc_true_type>::value());
  evalTestcase("OR(false, false)", false,
      tc_disjunction<tc_false_type, tc_false_type>::value());
  evalTestcase("OR(true, true, true)", true,
      tc_disjunction<tc_true_type, tc_true_type, tc_true_type>::value());
  evalTestcase("OR(true, true, false)", true,
      tc_disjunction<tc_true_type, tc_true_type, tc_false_type>::value());
  evalTestcase("OR(true, false, true)", true,
      tc_disjunction<tc_true_type, tc_false_type, tc_true_type>::value());
  evalTestcase("OR(false, true, true)", true,
      tc_disjunction<tc_false_type, tc_true_type, tc_true_type>::value());
  evalTestcase("OR(false, false, false)", false,
      tc_disjunction<tc_false_type, tc_false_type, tc_false_type>::value());


  evalTestcase("IsArithmetic(int)", true, tc_is_arithmetic_type<int>::value());
  evalTestcase("IsArithmetic(char)", true, tc_is_arithmetic_type<char>::value());
  evalTestcase("IsArithmetic(unsigned int)", true, tc_is_arithmetic_type<unsigned int>::value());
  evalTestcase("IsArithmetic(unsigned char)", true, tc_is_arithmetic_type<unsigned char>::value());
  evalTestcase("IsArithmetic(long)", true, tc_is_arithmetic_type<long>::value());
  evalTestcase("IsArithmetic(unsigned long)", true, tc_is_arithmetic_type<unsigned long>::value());
  evalTestcase("IsArithmetic(float)", true, tc_is_arithmetic_type<float>::value());
  evalTestcase("IsArithmetic(const int)", true, tc_is_arithmetic_type<const int>::value());
  evalTestcase("IsArithmetic(volatile int)", true, tc_is_arithmetic_type<volatile int>::value());
  evalTestcase("IsArithmetic(const volatile int)", true, tc_is_arithmetic_type<const volatile int>::value());
  evalTestcase("IsArithmetic(volatile const int)", true, tc_is_arithmetic_type<volatile const int>::value());

  evalTestcase("IsArithmetic(int*)", false, tc_is_arithmetic_type<int*>::value());
  evalTestcase("IsArithmetic(int&)", false, tc_is_arithmetic_type<int&>::value());
  evalTestcase("IsArithmetic(string*)", false, tc_is_arithmetic_type<string*>::value());
  evalTestcase("IsArithmetic(const string &)", false, tc_is_arithmetic_type<const string &>::value());
  evalTestcase("IsArithmetic(void*)", false, tc_is_arithmetic_type<void *>::value());

  enum fooEnum { A, B, C };
  evalTestcase("IsArithmetic(enum fooEnum)", false, tc_is_arithmetic_type<fooEnum>::value());
  evalTestcase("IsEnum(enum fooEnum)", true, tc_is_enum<fooEnum>::value());

  evalTestcase("IsPointer(int*)", true, tc_is_pointer<int*>::value());
  evalTestcase("IsPointer(int**)", true, tc_is_pointer<int**>::value());
  evalTestcase("IsPointer(int&)", false, tc_is_pointer<int&>::value());
  evalTestcase("IsPointer(string*)", true, tc_is_pointer<string*>::value());
  evalTestcase("IsPointer(const string*)", true, tc_is_pointer<const string*>::value());
  evalTestcase("IsPointer(const string *const)", true, tc_is_pointer<const string *const>::value());
  evalTestcase("IsPointer(string *const)", true, tc_is_pointer<string *const>::value());
  evalTestcase("IsPointer(const volatile string*)", true, tc_is_pointer<const volatile string*>::value());
  evalTestcase("IsPointer(volatile const string*)", true, tc_is_pointer<volatile const string*>::value());
  evalTestcase("IsPointer(volatile string*)", true, tc_is_pointer<volatile string*>::value());
  evalTestcase("IsPointer(const string &)", false, tc_is_pointer<const string &>::value());
  evalTestcase("IsPointer(void*)", true, tc_is_pointer<void *>::value());

  evalTestcase("IsNullPointer(nullptr_t)", true, tc_is_null_pointer<nullptr_t>::value());
  evalTestcase("IsNullPointer(const nullptr_t)", true, tc_is_null_pointer<const nullptr_t>::value());
  evalTestcase("IsNullPointer(volatile nullptr_t)", true, tc_is_null_pointer<volatile nullptr_t>::value());
  evalTestcase("IsNullPointer(const volatile nullptr_t)", true,
      tc_is_null_pointer<const volatile nullptr_t>::value());
  evalTestcase("IsNullPointer(volatile const nullptr_t)", true,
      tc_is_null_pointer<volatile const nullptr_t>::value());


  evalTestcase("IsScalar(int)", true, tc_is_scalar<int>::value());;
  evalTestcase("IsScalar(int*)", true, tc_is_scalar<int*>::value());;
  evalTestcase("IsScalar(string*)", true, tc_is_scalar<string*>::value());;
  evalTestcase("IsScalar(const string*)", true, tc_is_scalar<const string*>::value());;
  evalTestcase("IsScalar(string&)", false, tc_is_scalar<string&>::value());;
  evalTestcase("IsScalar(class string)", false, tc_is_scalar<string>::value());;
  evalTestcase("IsScalar(enum fooEnum)", true, tc_is_scalar<fooEnum>::value());;
  evalTestcase("IsScalar(nullptr_t)", true, tc_is_scalar<nullptr_t>::value());;
  evalTestcase("IsScalar(const nullptr_t)", true, tc_is_scalar<const nullptr_t>::value());;
}

void testVector() {
  startTestSuite("Vector");

  vector<int> intVec;
  expectEqual("Start vec size", (size_t)0, intVec.size());
  expectGreaterOrEqual("vec cap >= size", intVec.size(), intVec.capacity());
  intVec.push_back(42);
  expectEqual("1 item vec size", (size_t)1, intVec.size());
  expectEqual("first item deref", 42, intVec[0]);
  intVec.push_back(211);
  intVec.push_back(312);
  expectEqual("first item deref #2", 42, intVec[0]); // Doesn't change.
  expectEqual("third item deref", 312, intVec[2]); // Correct item.
  intVec[2] = 411;
  expectEqual("reassigned third item", 411, intVec[2]); // Correct overwrite.
  expectEqual("first item deref #3", 42, intVec[0]); // Doesn't change.
  intVec.clear();
  expectEqual("Cleared vec size", (size_t)0, intVec.size());

  vector<string> stringVec;
  expectEqual("String vec start size", (size_t)0, stringVec.size());
  stringVec.push_back(string("meep"));
  expectEqual("First String", string("meep"), stringVec[0]);
  stringVec[0] = "boop";
  expectEqual("Reassigned first string", string("boop"), stringVec[0]);
  stringVec.push_back("foo");
  stringVec.push_back("bar");
  // Add enough strings that this triggers a reallocate.
  for (int i = 0; i < 10; i++) {
    stringVec.push_back("abc");
  }

  expectEqual("13 item size", (size_t)13, stringVec.size());
  expectGreaterOrEqual("13 item cap > size", stringVec.size(), stringVec.capacity());
  expectEqual("moved first item", string("boop"), stringVec[0]);
  expectEqual("moved third item", string("bar"), stringVec[2]);

  vector<string> newStringVec;
  expectEqual("New string vec start size", (size_t)0, newStringVec.size());
  newStringVec = stringVec;
  expectEqual("New string vec post-assign size", stringVec.size(), newStringVec.size());
  expectEqual("New string vec post-assign size #2", (size_t)13, newStringVec.size());
  expectGreaterOrEqual("New string vec post-assign cap", newStringVec.size(), newStringVec.capacity());
  expectEqual("New string vec first elem", string("boop"), newStringVec[0]);
  expectEqual("Old string vec first elem", string("boop"), stringVec[0]);

  vector<long> longVec = {1, 2, 3, 4};
  expectEqual("brace-initialized vector size", (size_t)4, longVec.size());
  expectGreaterOrEqual("brace-initialized vector capacity", longVec.size(), longVec.capacity());
  expectEqual("first long", (long)1, longVec[0]);
  expectEqual("last long", (long)4, longVec[3]);

  vector<long> longVecCopy(longVec);
  expectEqual("copy of brace-initialized vector size", (size_t)4, longVecCopy.size());
  expectEqual("copy of brace-initialized vector 1st val", (long)1, longVecCopy[0]);
  longVecCopy[2] = -42;
  expectEqual("Modified copy[2] = -42", (long)-42, longVecCopy[2]);
  expectEqual("Original copy[2] = 3", (long)3, longVec[2]); // shouldn't change.


}

int main(int argc, char **argv) {
  cout << std::boolalpha; // pretty-print 'true' and 'false'.

  // reinitialize counters.
  testFailures = 0;
  testRuns = 0;
  numSuites = 0;

  testTypeTraits();
  testVector();

  cout << "Ran " << testRuns << " test cases." << endl;
  if (testFailures == 0) {
    cout << "Tests PASSED" << endl;
    return 0;
  } else {
    cout << "*** " << testFailures << " test cases FAILED." << endl;
    return 1;
  }
}
