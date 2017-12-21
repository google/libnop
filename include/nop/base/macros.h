/*
 * Copyright 2017 The Native Object Protocols Authors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef LIBNOP_INCLUDE_NOP_BASE_MACROS_H_
#define LIBNOP_INCLUDE_NOP_BASE_MACROS_H_

// Macros to apply other macros over all elements in a list.

// Recursive expansion macros.
#define _NOP_EXPAND0(...) __VA_ARGS__
#define _NOP_EXPAND1(...) _NOP_EXPAND0(_NOP_EXPAND0(_NOP_EXPAND0(__VA_ARGS__)))
#define _NOP_EXPAND2(...) _NOP_EXPAND1(_NOP_EXPAND1(_NOP_EXPAND1(__VA_ARGS__)))
#define _NOP_EXPAND3(...) _NOP_EXPAND2(_NOP_EXPAND2(_NOP_EXPAND2(__VA_ARGS__)))
#define _NOP_EXPAND4(...) _NOP_EXPAND3(_NOP_EXPAND3(_NOP_EXPAND3(__VA_ARGS__)))
#define _NOP_EXPAND5(...) _NOP_EXPAND4(_NOP_EXPAND4(_NOP_EXPAND4(__VA_ARGS__)))
#define _NOP_EXPAND(...)  _NOP_EXPAND5(_NOP_EXPAND5(_NOP_EXPAND5(__VA_ARGS__)))

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

#define _NOP_REST_ARG(_, ...) __VA_ARGS__

// Returns the second argument of a list.
#define _NOP_SECOND_ARG(_, second, ...) second

#define _NOP_CAT(a, ...) a##__VA_ARGS__

#define _NOP_IS_PROBE(...) _NOP_SECOND_ARG(__VA_ARGS__, 0)
#define _NOP_PROBE() ~, 1

#define _NOP_IS_PAREN(...) _NOP_IS_PROBE(_NOP_IS_PAREN_PROBE __VA_ARGS__)
#define _NOP_IS_PAREN_PROBE(...) _NOP_PROBE()

#define _NOP_NOT(x) _NOP_IS_PROBE(_NOP_CAT(_NOP_NOT_, x))
#define _NOP_NOT_0 _NOP_PROBE()

#define _NOP_BOOL(x) _NOP_NOT(_NOP_NOT(x))

#define _NOP_IF_ELSE(condition) __NOP_IF_ELSE(_NOP_BOOL(condition))
#define __NOP_IF_ELSE(condition) _NOP_CAT(_NOP_IF_, condition)

#define _NOP_IF_1(...) __VA_ARGS__ _NOP_IF_1_ELSE
#define _NOP_IF_0(...) _NOP_IF_0_ELSE

#define _NOP_IF_1_ELSE(...)
#define _NOP_IF_0_ELSE(...) __VA_ARGS__

#define _NOP_HAS_ARGS(...) \
  _NOP_BOOL(_NOP_FIRST_ARG(_NOP_END_OF_ARGUMENTS_ __VA_ARGS__)())
#define _NOP_END_OF_ARGUMENTS_(...) _NOP_IS_PAREN(__VA_ARGS__)

#define _NOP_EMPTY()
#define _NOP_DEFER1(m) m _NOP_EMPTY()
#define _NOP_DEFER2(m) m _NOP_EMPTY _NOP_EMPTY()()
#define _NOP_DEFER3(m) m _NOP_EMPTY _NOP_EMPTY _NOP_EMPTY()()()
#define _NOP_DEFER4(m) m _NOP_EMPTY _NOP_EMPTY _NOP_EMPTY _NOP_EMPTY()()()()
#define _NOP_DEFER5(m) \
  m _NOP_EMPTY _NOP_EMPTY _NOP_EMPTY _NOP_EMPTY _NOP_EMPTY()()()()()

#define _NOP_REMOVE_PARENS(...)            \
  _NOP_IF_ELSE(_NOP_IS_PAREN(__VA_ARGS__)) \
    (_NOP_STRIP_PARENS __VA_ARGS__)        \
    (__VA_ARGS__)

#define _NOP_STRIP_PARENS(...) __VA_ARGS__

#define NOP_MAP(...) _NOP_EXPAND(_NOP_MAP_FIRST(__VA_ARGS__))

#define _NOP_MAP_FIRST(m, ...)                         \
  _NOP_IF_ELSE(_NOP_HAS_ARGS(__VA_ARGS__)) (           \
    m(_NOP_REMOVE_PARENS(_NOP_FIRST_ARG(__VA_ARGS__))) \
    _NOP_MAP_NEXT(m, _NOP_REST_ARG(__VA_ARGS__))       \
  )(/* done */)

#define _NOP_MAP_NEXT(m, ...)                                    \
  _NOP_IF_ELSE(_NOP_HAS_ARGS(__VA_ARGS__)) (                     \
    , m(_NOP_REMOVE_PARENS(_NOP_FIRST_ARG(__VA_ARGS__)))         \
    _NOP_DEFER3(__NOP_MAP_NEXT)()(m, _NOP_REST_ARG(__VA_ARGS__)) \
  )(/* done */)

#define __NOP_MAP_NEXT() _NOP_MAP_NEXT

#define NOP_MAP_ARGS(...) _NOP_EXPAND(_NOP_MAP_FIRST_ARGS(__VA_ARGS__))
#define _NOP_MAP_ARGS() NOP_MAP_ARGS

#define _NOP_MAP_FIRST_ARGS(m, args, ...)                                        \
  _NOP_IF_ELSE(_NOP_HAS_ARGS(__VA_ARGS__)) (                                     \
    m(_NOP_REMOVE_PARENS(args), _NOP_REMOVE_PARENS(_NOP_FIRST_ARG(__VA_ARGS__))) \
    _NOP_MAP_NEXT_ARGS(m, args, _NOP_REST_ARG(__VA_ARGS__))                      \
  )(/* done */)

#define _NOP_MAP_NEXT_ARGS(m, args, ...)                                           \
  _NOP_IF_ELSE(_NOP_HAS_ARGS(__VA_ARGS__)) (                                       \
    , m(_NOP_REMOVE_PARENS(args), _NOP_REMOVE_PARENS(_NOP_FIRST_ARG(__VA_ARGS__))) \
    _NOP_DEFER3(__NOP_MAP_NEXT_ARGS)()(m, args, _NOP_REST_ARG(__VA_ARGS__))        \
  )(/* done */)

#define __NOP_MAP_NEXT_ARGS() _NOP_MAP_NEXT_ARGS


#endif  // LIBNOP_INCLUDE_NOP_BASE_MACROS_H_
