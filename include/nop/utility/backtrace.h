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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_BACKTRACE_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_BACKTRACE_H_

#include <execinfo.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace nop {

//
// Backtrace utility based on the UNIX backtrace API.
//
// Provides a simple mechanism to generate a backtrace with symbol lookup.
//

class Backtrace {
 public:
  Backtrace() = default;
  Backtrace(const Backtrace&) = default;
  Backtrace& operator=(const Backtrace&) = default;
  Backtrace(Backtrace&&) = default;
  Backtrace& operator=(Backtrace&&) = default;

  enum : std::size_t { kMaxStackFrames = 1024 };

  static Backtrace Create(
      std::size_t max_stack_frames = kMaxStackFrames) {
    std::vector<void*> stack_frames(max_stack_frames);

    const int count = backtrace(stack_frames.data(), stack_frames.size());
    if (count < 0)
      return Backtrace{};

    stack_frames.resize(count);
    return Backtrace{std::move(stack_frames)};
  }

  std::string to_string() const {
    std::stringstream stream;
    stream << *this;
    return stream.str();
  }

  friend std::ostream& operator<<(std::ostream& stream,
                                  const Backtrace& backtrace) {
    auto strings = backtrace.GetStrings();
    if (!strings) {
      stream << "<backtrace failed>" << std::endl;
    } else {
      for (std::size_t i = 0; i < backtrace.size(); i++)
        stream << strings[i] << std::endl;
    }

    return stream;
  }

  void* operator[](std::size_t index) const { return stack_frames_[index]; }

  std::size_t size() const { return stack_frames_.size(); }
  void clear() { stack_frames_.clear(); }

  explicit operator bool() const { return size() > 0; }

 private:
  Backtrace(std::vector<void*> stack_frames)
      : stack_frames_{std::move(stack_frames)} {}

  std::unique_ptr<char* [], decltype(std::free) *> GetStrings() const {
    return {backtrace_symbols(stack_frames_.data(), stack_frames_.size()),
            std::free};
  }

  std::vector<void*> stack_frames_;
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_BACKTRACE_H_
