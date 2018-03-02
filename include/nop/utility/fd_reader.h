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

#ifndef LIBNOP_INCLUDE_NOP_UTILITY_FD_READER_H_
#define LIBNOP_INCLUDE_NOP_UTILITY_FD_READER_H_

#include <errno.h>
#include <sys/types.h>
#include <unistd.h>

#include <utility>

#include <nop/status.h>

namespace nop {

// FdReader is a reader type that wraps around a UNIX file descriptor. The
// reader takes ownerhip of the fd and automatically closes it when destroyed,
// unless it is released.
class FdReader {
 public:
  FdReader() = default;
  FdReader(int fd) : fd_{fd} {}
  FdReader(const FdReader&) = delete;
  FdReader(FdReader&& other) { *this = std::move(other); }

  ~FdReader() { Clear(); }

  FdReader& operator=(const FdReader&) = delete;
  FdReader& operator=(FdReader&& other) {
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

  Status<void> Ensure(std::size_t) { return {}; }

  Status<void> Read(std::uint8_t* byte) {
    while (true) {
      const int ret = ::read(fd_, byte, sizeof(*byte));
      if (ret == 1)
        return {};
      else if (ret == 0)
        return ErrorStatus::ReadLimitReached;
      else if (errno != EINTR)
        return ErrorStatus::IOError;
      else
        continue;  // Interrupted by signal.
    }
  }

  Status<void> Read(void* begin, void* end) {
    std::uint8_t* begin_byte = static_cast<std::uint8_t*>(begin);
    std::uint8_t* end_byte = static_cast<std::uint8_t*>(end);

    for (std::uint8_t* byte = begin_byte; byte < end_byte; byte++) {
      auto status = Read(byte);
      if (!status)
        return status;
    }

    return {};
  }

 private:
  int fd_{-1};
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_UTILITY_FD_READER_H_
