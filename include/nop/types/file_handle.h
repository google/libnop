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

#ifndef LIBNOP_INCLUDE_NOP_TYPES_FILE_HANDLE_H_
#define LIBNOP_INCLUDE_NOP_TYPES_FILE_HANDLE_H_

#include <fcntl.h>
#include <unistd.h>

#include <string>

#include <nop/types/handle.h>

namespace nop {

//
// Types for managing UNIX file handles.
//

// UniqueHandle policy for file handles.
struct FileHandlePolicy {
  using Type = int;
  enum : int { kEmptyHandle = -1 };

  static constexpr int Default() { return kEmptyHandle; }
  static bool IsValid(int fd) { return fd >= 0; }

  static void Close(int* fd) {
    ::close(*fd);
    *fd = kEmptyHandle;
  }
  static int Release(int* fd) {
    int temp = kEmptyHandle;
    std::swap(*fd, temp);
    return temp;
  }

  // TODO(eieio): Define handle type enum.
  static constexpr std::uint64_t HandleType() { return 1; }
};

// Alias for unmanaged file handles.
using FileHandle = Handle<FileHandlePolicy>;

// Represents a managed file handle with defined lifetime.
class UniqueFileHandle : public UniqueHandle<FileHandlePolicy> {
 public:
  using Base = UniqueHandle<FileHandlePolicy>;
  using Base::Base;

  // Named constructor that opens a UniqueFileHandle given a path, flags, and
  // optional file mode.
  static UniqueFileHandle Open(const std::string& path, int flags,
                               mode_t mode = 0) {
    return UniqueFileHandle{::open(path.c_str(), flags, mode)};
  }

  // Named constructor that opens a UniqueFileHandle relative to the given
  // directory with the given path, flags, and optional file mode.
  static UniqueFileHandle OpenAt(FileHandle directory_handle,
                                 const std::string& path, int flags,
                                 mode_t mode = 0) {
    return UniqueFileHandle{
        ::openat(directory_handle.get(), path.c_str(), flags, mode)};
  }

  // Named constructor that duplicated the given file handle.
  static UniqueFileHandle AsDuplicate(FileHandle handle) {
    return UniqueFileHandle{::dup(handle.get())};
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_FILE_HANDLE_H_
