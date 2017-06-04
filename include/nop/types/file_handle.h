#ifndef LIBNOP_INCLUDE_NOP_TYPES_FILE_HANDLE_H_
#define LIBNOP_INCLUDE_NOP_TYPES_FILE_HANDLE_H_

#include <fcntl.h>
#include <unistd.h>

#include <string>

#include <nop/types/handle.h>

namespace nop {

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

using FileHandle = Handle<FileHandlePolicy>;

class UniqueFileHandle : public UniqueHandle<FileHandlePolicy> {
 public:
  using Base = UniqueHandle<FileHandlePolicy>;
  using Base::Base;

  static UniqueFileHandle Open(const std::string& path, int flags,
                               mode_t mode = 0) {
    return UniqueFileHandle{::open(path.c_str(), flags, mode)};
  }

  static UniqueFileHandle OpenAt(FileHandle directory_handle,
                                 const std::string& path, int flags,
                                 mode_t mode = 0) {
    return UniqueFileHandle{
        ::openat(directory_handle.get(), path.c_str(), flags, mode)};
  }

  static UniqueFileHandle AsDuplicate(FileHandle handle) {
    return UniqueFileHandle{::dup(handle.get())};
  }
};

}  // namespace nop

#endif  // LIBNOP_INCLUDE_NOP_TYPES_FILE_HANDLE_H_
