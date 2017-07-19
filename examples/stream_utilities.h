#ifndef LIBNOP_EXAMPLES_STREAM_UTILITIES_H_
#define LIBNOP_EXAMPLES_STREAM_UTILITIES_H_

#include <array>
#include <iostream>
#include <vector>

#include <unistd.h>

namespace {

// Prints a std::vector<T> to the given stream. This template will work for any
// type T that has an operator<< overload.
template <typename T, typename Allocator>
std::ostream& operator<<(std::ostream& stream,
                         const std::vector<T, Allocator>& vector) {
  stream << "vector{";

  const std::size_t length = vector.size();
  std::size_t count = 0;

  for (const auto& element : vector) {
    stream << element;
    if (count < length - 1)
      stream << ", ";
    count++;
  }

  stream << "}";
  return stream;
}

// Prints a std::array<T, Size> to the given stream. This template will work for
// any type T that has an operator<< overload.
template <typename T, std::size_t Size>
std::ostream& operator<<(std::ostream& stream,
                         const std::array<T, Size>& array) {
  stream << "array{";

  const std::size_t length = array.size();
  std::size_t count = 0;

  for (const auto& element : array) {
    stream << element;
    if (count < length - 1)
      stream << ", ";
    count++;
  }

  stream << "}";
  return stream;
}

class FdOutputBuffer : public std::streambuf {
 public:
  FdOutputBuffer(int fd) : fd_{fd} {}
  ~FdOutputBuffer() { close(fd_); }

  int_type overflow(int_type c) override {
    if (c != EOF) {
      char value = c;
      if (write(fd_, &value, sizeof(value)) != 1)
        return EOF;
    }
    return c;
  }

  std::streamsize xsputn(const char* data, std::streamsize size) override {
    return write(fd_, data, size);
  }

 private:
  int fd_;

  FdOutputBuffer(const FdOutputBuffer&) = delete;
  void operator=(const FdOutputBuffer&) = delete;
};

class FdOutputStream : public std::ostream {
 public:
  FdOutputStream(int fd) : std::ostream{0}, buffer_{fd} { rdbuf(&buffer_); }

 private:
  FdOutputBuffer buffer_;

  FdOutputStream(const FdOutputStream&) = delete;
  void operator=(const FdOutputStream&) = delete;
};

class FdInputBuffer : public std::streambuf {
 public:
  FdInputBuffer(int fd) : fd_{fd} {
    setg(&buffer_[4], &buffer_[4], &buffer_[4]);
  }
  ~FdInputBuffer() { close(fd_); }

  int_type underflow() override {
    // Is read position before end of buffer?
    if (gptr() < egptr()) {
      return traits_type::to_int_type(*gptr());
    }

    int num_putback;
    num_putback = gptr() - eback();
    if (num_putback > 4)
      num_putback = 4;

    std::memmove(&buffer_[4 - num_putback], gptr() - num_putback, num_putback);

    const int count = read(fd_, &buffer_[4], kBufferSize - 4);
    if (count <= 0)
      return EOF;

    setg(&buffer_[4 - num_putback], &buffer_[4], &buffer_[4 + count]);

    return traits_type::to_int_type(*gptr());
  }

 private:
  enum : std::size_t { kBufferSize = 10 };

  int fd_;
  char buffer_[kBufferSize];

  FdInputBuffer(const FdInputBuffer&) = delete;
  void operator=(const FdInputBuffer&) = delete;
};

class FdInputStream : public std::istream {
 public:
  FdInputStream(int fd) : std::istream{nullptr}, buffer_{fd} {
    rdbuf(&buffer_);
  }

 private:
  FdInputBuffer buffer_;

  FdInputStream(const FdInputStream&) = delete;
  void operator=(const FdInputStream&) = delete;
};

}  // anonymous namespace

#endif  // LIBNOP_EXAMPLES_STREAM_UTILITIES_H_
