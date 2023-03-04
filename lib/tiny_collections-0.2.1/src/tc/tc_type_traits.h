// (c) Copyright 2022 Aaron Kimball
//
// Type traits necessary for instantiation of tc::vector<T>, etc.
// without `#include <type_traits>` (since AVR lacks this header).

#ifndef _TC_TYPE_TRAITS_H
#define _TC_TYPE_TRAITS_H

#include<stddef.h> // for nullptr_t.

namespace tc {
  // compile_time_const<someT, someV> is a type that represents the value someV in the compiler.
  template<typename T, T val> class compile_time_const {
  public:
    typedef T type;
    static constexpr const T value() { return val; };
  };

  typedef compile_time_const<bool, true> tc_true_type;  // A type that represents the value 'true'
  typedef compile_time_const<bool, false> tc_false_type; // A type that represents the value 'false'

  // Template logic that performs a boolean test
  // tc_conditional<bool, X, Y>::type == X only if bool is true, and == Y only if bool is false.

  template<bool exprVal, typename ifTrue, typename ifFalse> class tc_conditional {
  public:
    // Since there are only two possibilities for exprVal, and false is specialized below,
    // the 'more generic' version of this template is always/only used for exprVal == true.
    using type = ifTrue;
    constexpr static bool value() { return type::value(); };
  };

  // When specialized on exprVal==false, tc_conditional::type uses the ifFalse type.
  template<typename ifTrue, typename ifFalse> class tc_conditional<false, ifTrue, ifFalse> {
  public:
    using type = ifFalse;
    constexpr static bool value() { return type::value(); };
  };

  // Conjunction: AND over N different clauses. (a /\ b /\ c /\ ...)
  template<typename...> class tc_conjunction: public tc_true_type {};
  template<typename C_> class tc_conjunction<C_>: public C_ {};
  // If C1::value() is true, then check the rest; o/w C1 is false so use it for the false type.
  template<typename C1_, typename ...Cn_> class tc_conjunction<C1_, Cn_...>:
      public tc_conditional<C1_::value(), tc_conjunction<Cn_...>, C1_> {};

  // Disjunction: OR over N different clauses. (a \/ b \/ c \/ ...)
  template<typename...> class tc_disjunction: public tc_false_type {};
  template<typename D_> class tc_disjunction<D_>: public D_ {};
  // If D1::value is true, then short-circuit to 'true' answer (and D1 must be the 'true' type).
  // Otherwise, keep looking for a 'true' in the rest of the list.
  template<typename D1_, typename ...Dn_> class tc_disjunction<D1_, Dn_...>:
      public tc_conditional<D1_::value(), D1_, tc_disjunction<Dn_...> > {};


  // tc_is_arithmetic_type<T>: Is type T a number?
  template<typename T> class tc_is_arithmetic_type: public tc_false_type {};
  template<> class tc_is_arithmetic_type<char>: public tc_true_type {};
  template<> class tc_is_arithmetic_type<short>: public tc_true_type {};
  template<> class tc_is_arithmetic_type<int>: public tc_true_type {};
  template<> class tc_is_arithmetic_type<long int>: public tc_true_type {};
  template<> class tc_is_arithmetic_type<long long int>: public tc_true_type {};
  template<> class tc_is_arithmetic_type<unsigned char>: public tc_true_type {};
  template<> class tc_is_arithmetic_type<unsigned short>: public tc_true_type {};
  template<> class tc_is_arithmetic_type<unsigned int>: public tc_true_type {};
  template<> class tc_is_arithmetic_type<unsigned long int>: public tc_true_type {};
  template<> class tc_is_arithmetic_type<unsigned long long int>: public tc_true_type {};
  template<> class tc_is_arithmetic_type<float>: public tc_true_type {};
  template<> class tc_is_arithmetic_type<double>: public tc_true_type {};
  template<> class tc_is_arithmetic_type<long double>: public tc_true_type {};
  template<typename T2> class tc_is_arithmetic_type<const T2>: public tc_is_arithmetic_type<T2> {};
  template<typename T2> class tc_is_arithmetic_type<volatile T2>: public tc_is_arithmetic_type<T2> {};
  template<typename T2> class tc_is_arithmetic_type<const volatile T2>: public tc_is_arithmetic_type<T2> {};

  template<typename T> class tc_is_integral_constant: public tc_false_type {};
  template<typename T2, T2 v2> class tc_is_integral_constant<compile_time_const<T2, v2> >:
      public tc_true_type {};

  // g++ has built-in __is_enum(T) that returns true if T is an enum
  template<typename T> class tc_is_enum: public compile_time_const<bool, __is_enum(T)> {};

  template<typename T> class tc_is_pointer: public tc_false_type {};
  template<typename T> class tc_is_pointer<T*>: public tc_true_type {};
  template<typename T> class tc_is_pointer<T* const>: public tc_true_type {};
  template<typename T> class tc_is_pointer<T* volatile>: public tc_true_type {};
  template<typename T> class tc_is_pointer<T* const volatile>: public tc_true_type {};

  template<typename T> class tc_is_member_pointer: public tc_false_type {};
  template<class C, typename F> class tc_is_member_pointer<F C::*>: public tc_true_type {};
  template<class C, typename F> class tc_is_member_pointer<const F C::*>:
    public tc_is_member_pointer<F C::*> {};
  template<class C, typename F> class tc_is_member_pointer<volatile F C::*>:
    public tc_is_member_pointer<F C::*> {};
  template<class C, typename F> class tc_is_member_pointer<const volatile F C::*>:
    public tc_is_member_pointer<F C::*> {};

  template<typename T> class tc_is_null_pointer: public tc_false_type {};
  template<> class tc_is_null_pointer<nullptr_t>: public tc_true_type {};
  template<typename T2> class tc_is_null_pointer<const T2>: public tc_is_null_pointer<T2> {};
  template<typename T2> class tc_is_null_pointer<volatile T2>: public tc_is_null_pointer<T2> {};
  template<typename T2> class tc_is_null_pointer<const volatile T2>: public tc_is_null_pointer<T2> {};


  // is_scalar<T>: a type representing 'true' iff the type T in question represents an
  // ordinary in-memory scalar value type.
  template<class T> class tc_is_scalar: public tc_disjunction<
      tc_is_arithmetic_type<T>,
      tc_is_integral_constant<T>,
      tc_is_enum<T>,
      tc_is_pointer<T>,
      tc_is_member_pointer<T>,
      tc_is_null_pointer<T> > {};

}; // namespace tc

#endif // _TC_TYPE_TRAITS_H

