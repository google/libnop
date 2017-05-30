#ifndef LIBNOP_INCLUDE_NOP_BASE_MACROS_H_
#define LIBNOP_INCLUDE_NOP_BASE_MACROS_H_

// Macros to apply other macros over all elements in a list.
//
// For example, for a macro A(x) and B(x, y):
// - FOR_EACH(A, 1, 2, 3) -> A(1) A(2) A(3).
// - FOR_EACH_BINARY(B, z, 1, 2, 3) -> B(z, 1) B(z, 2) B(z, 3)
// - FOR_EACH_LIST(A, 1, 2, 3) -> A(1), B(2), C(3)
// - FOR_EACH_BINARY_LIST(B, z, 1, 2, 3) -> B(z, 1), B(z, 2), B(z, 3)
//
// Empty lists are supported and will produce no output.

// Recursive expansion macros.
#define _NOP_EXPAND0(...) __VA_ARGS__
#define _NOP_EXPAND1(...) _NOP_EXPAND0(_NOP_EXPAND0(_NOP_EXPAND0(__VA_ARGS__)))
#define _NOP_EXPAND2(...) _NOP_EXPAND1(_NOP_EXPAND1(_NOP_EXPAND1(__VA_ARGS__)))
#define _NOP_EXPAND3(...) _NOP_EXPAND2(_NOP_EXPAND2(_NOP_EXPAND2(__VA_ARGS__)))
#define _NOP_EXPAND4(...) _NOP_EXPAND3(_NOP_EXPAND3(_NOP_EXPAND3(__VA_ARGS__)))
#define _NOP_EXPAND(...) _NOP_EXPAND4(_NOP_EXPAND4(_NOP_EXPAND4(__VA_ARGS__)))

// Required to workaround a bug in the VC++ preprocessor.
#define _NOP_INDIRECT_EXPAND(macro, args) macro args

// Defines a step separation for macro expansion.
#define _NOP_SEPARATOR

// Clears any remaining contents wrapped in parentheses.
#define _NOP_CLEAR(...)

// Introduces a first dummy argument and _NOP_CLEAR as second argument.
#define _NOP_CLEAR_IF_LAST() _, _NOP_CLEAR

// Returns the first argument of a list.
#define _NOP_FIRST_ARG(first, ...) first

// Returns the second argument of a list.
#define _NOP_SECOND_ARG(_, second, ...) second

// Expands the arguments and introduces a separator.
#define _NOP_EXPAND_NEXT_FUNC(_, next_func, ...)        \
  _NOP_INDIRECT_EXPAND(_NOP_SECOND_ARG, (_, next_func)) \
  _NOP_SEPARATOR

// Returns next_func if the next element is not (), or _NOP_CLEAR
// otherwise.
//
// _NOP_CLEAR_IF_LAST inserts an extra first dummy argument if peek is ().
#define _NOP_NEXT_FUNC(next_element, next_func) \
  _NOP_EXPAND_NEXT_FUNC(_NOP_CLEAR_IF_LAST next_element, next_func)

// Macros for the unary version of NOP_FOR_EACH.

// Applies the unary macro. Duplicated for macro recursive expansion.
#define _NOP_APPLY_1(macro, head, next, ...) \
  macro(head) _NOP_NEXT_FUNC(next, _NOP_APPLY_2)(macro, next, __VA_ARGS__)

// Applies the unary macro. Duplicated for macro recursive expansion.
#define _NOP_APPLY_2(macro, head, next, ...) \
  macro(head) _NOP_NEXT_FUNC(next, _NOP_APPLY_1)(macro, next, __VA_ARGS__)

// Stops expansion if __VA_ARGS__ is empty, calling _NOP_APPLY_1
// otherwise.
#define _NOP_HANDLE_EMPTY_ARGS(macro, ...)                    \
  _NOP_NEXT_FUNC(_NOP_FIRST_ARG(__VA_ARGS__()), _NOP_APPLY_1) \
  (macro, __VA_ARGS__, ())

// Applies a unary macro over all the elements in a list.
#define NOP_FOR_EACH(macro, ...) \
  _NOP_EXPAND(_NOP_HANDLE_EMPTY_ARGS(macro, __VA_ARGS__))

// Applies the unary macro at the end of a list. Duplicated for macro recursive
// expansion.
#define _NOP_APPLY_LIST_1(macro, head, next, ...) \
  , macro(head)                                   \
        _NOP_NEXT_FUNC(next, _NOP_APPLY_LIST_2)(macro, next, __VA_ARGS__)

// Applies the unary macro at the end of a list. Duplicated for macro recursive
// expansion.
#define _NOP_APPLY_LIST_2(macro, head, next, ...) \
  , macro(head)                                   \
        _NOP_NEXT_FUNC(next, _NOP_APPLY_LIST_1)(macro, next, __VA_ARGS__)

// Applies the unary macro at the start of a list.
#define _NOP_APPLY_LIST_0(macro, head, next, ...) \
  macro(head) _NOP_NEXT_FUNC(next, _NOP_APPLY_LIST_1)(macro, next, __VA_ARGS__)

// Stops expansion if __VA_ARGS__ is empty, calling _NOP_APPLY_LIST_0
// otherwise.
#define _NOP_HANDLE_EMPTY_LIST(macro, ...)                         \
  _NOP_NEXT_FUNC(_NOP_FIRST_ARG(__VA_ARGS__()), _NOP_APPLY_LIST_0) \
  (macro, __VA_ARGS__, ())

// Applies a unary macro over all the elements in a list.
#define NOP_FOR_EACH_LIST(macro, ...) \
  _NOP_EXPAND(_NOP_HANDLE_EMPTY_LIST(macro, __VA_ARGS__))

// Macros for the binary version of NOP_FOR_EACH.

// Applies the binary macro. Duplicated for macro recursive expansion.
#define _NOP_APPLY_BINARY_1(macro, arg, head, next, ...) \
  macro(arg, head)                                       \
      _NOP_NEXT_FUNC(next, _NOP_APPLY_BINARY_2)(macro, arg, next, __VA_ARGS__)

// Applies the binary macro. Duplicated for macro recursive expansion.
#define _NOP_APPLY_BINARY_2(macro, arg, head, next, ...) \
  macro(arg, head)                                       \
      _NOP_NEXT_FUNC(next, _NOP_APPLY_BINARY_1)(macro, arg, next, __VA_ARGS__)

// Version of _NOP_HANDLE_EMPTY_ARGS that takes 1 fixed argument for a
// binary macro.
#define _NOP_HANDLE_EMPTY_ARGS_BINARY(macro, arg, ...)               \
  _NOP_NEXT_FUNC(_NOP_FIRST_ARG(__VA_ARGS__()), _NOP_APPLY_BINARY_1) \
  (macro, arg, __VA_ARGS__, ())

// Applies a binary macro over all the elements in a list and a given argument.
#define NOP_FOR_EACH_BINARY(macro, arg, ...) \
  _NOP_EXPAND(_NOP_HANDLE_EMPTY_ARGS_BINARY(macro, arg, __VA_ARGS__))

// Applies the binary macro at the end of a list. Duplicated for macro recursive
// expansion.
#define _NOP_APPLY_BINARY_LIST_1(macro, arg, head, next, ...)        \
  , macro(arg, head) _NOP_NEXT_FUNC(next, _NOP_APPLY_BINARY_LIST_2)( \
        macro, arg, next, __VA_ARGS__)

// Applies the binary macro at the end of a list. Duplicated for macro recursive
// expansion.
#define _NOP_APPLY_BINARY_LIST_2(macro, arg, head, next, ...)        \
  , macro(arg, head) _NOP_NEXT_FUNC(next, _NOP_APPLY_BINARY_LIST_1)( \
        macro, arg, next, __VA_ARGS__)

// Applies the binary macro at the start of a list. Duplicated for macro
// recursive expansion.
#define _NOP_APPLY_BINARY_LIST_0(macro, arg, head, next, ...)      \
  macro(arg, head) _NOP_NEXT_FUNC(next, _NOP_APPLY_BINARY_LIST_1)( \
      macro, arg, next, __VA_ARGS__)

// Version of _NOP_HANDLE_EMPTY_LIST that takes 1 fixed argument for a
// binary macro.
#define _NOP_HANDLE_EMPTY_LIST_BINARY(macro, arg, ...)                    \
  _NOP_NEXT_FUNC(_NOP_FIRST_ARG(__VA_ARGS__()), _NOP_APPLY_BINARY_LIST_0) \
  (macro, arg, __VA_ARGS__, ())

// Applies a binary macro over all the elements in a list and a given argument.
#define NOP_FOR_EACH_BINARY_LIST(macro, arg, ...) \
  _NOP_EXPAND(_NOP_HANDLE_EMPTY_LIST_BINARY(macro, arg, __VA_ARGS__))

#endif  // LIBNOP_INCLUDE_NOP_BASE_MACROS_H_
