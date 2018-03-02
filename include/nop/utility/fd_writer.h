/*
 * Copyright 2018 The Native Object Protocols Authors
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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_FD_WRITER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_FD_WRITER_H_

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include <utility>

#include <nop/status.h>

namespace nop {

class FdWriter {
 public:
  FdWriter() = default;
  FdWriter(int fd) : fd_{fd} {}
  FdWriter(const FdWriter&) = delete;
  FdWriter(FdWriter&& other) { *this = std::move(other); }

  ~FdWriter() { Clear(); }

  FdWriter& operator=(const FdWriter&) = delete;
  FdWriter& operator=(FdWriter&& other) {
    if (this != &other) {
      Clear();
      std::swap(fd_, other.fd_);
    }
    return *this;
  }

  void Clear() {
    ::close(fd_);
    fd_ = -1;
  }

  int Release() {
    const int released_fd = fd_;
    fd_ = -1;
    return released_fd;
  }

  Status<void> Prepare(std::size_t) { return {}; }

  Status<void> Write(std::uint8_t byte) {
    while (true) {
      const int ret = ::write(fd_, &byte, sizeof(byte));
      if (ret == 1)
        return {};
      else if (ret == 0)
        return ErrorStatus::WriteLimitReached;
      else if (errno != EINTR)
        return ErrorStatus::IOError;
      else
        continue;  // Interrupted by signal.
    }
  }

  Status<void> Write(const void* begin, const void* end) {
    const std::uint8_t* begin_byte = static_cast<const std::uint8_t*>(begin);
    const std::uint8_t* end_byte = static_cast<const std::uint8_t*>(end);

    for (const std::uint8_t* byte = begin_byte; byte < end_byte; byte++) {
      auto status = Write(*byte);
      if (!status)
        return status;
    }

    return {};
  }

 private:
  int fd_{-1};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_FD_WRITER_H_
