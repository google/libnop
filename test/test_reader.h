#ifndef LIBNOP_TEST_TEST_READER_H_
#define LIBNOP_TEST_TEST_READER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

#include <nop/base/encoding.h>
#include <nop/base/handle.h>
#include <nop/base/utility.h>

namespace nop {

class TestReader {
 public:
  TestReader() = default;

  Status<void> Ensure(std::size_t size) {
    if (data_.size() - index_ < size)
      return ErrorStatus(ENOBUFS);
    else
      return {};
  }

  Status<void> Read(EncodingByte* prefix) {
    if (index_ < data_.size()) {
      *prefix = static_cast<EncodingByte>(data_[index_++]);
      return {};
    } else {
      return ErrorStatus(ENOBUFS);
    }
  }

  template <typename IterBegin, typename IterEnd>
  Status<void> ReadRaw(IterBegin begin, IterEnd end) {
    const std::size_t length_bytes =
        std::distance(begin, end) *
        sizeof(typename std::iterator_traits<IterBegin>::value_type);

    if (length_bytes > (data_.size() - index_))
      return ErrorStatus(ENOBUFS);

    std::copy(&data_[index_], &data_[index_ + length_bytes],
              reinterpret_cast<std::uint8_t*>(&*begin));
    index_ += length_bytes;
    return {};
  }

  template <typename HandleType>
  Status<HandleType> GetHandle(HandleReference handle_reference) {
    if (handle_reference < 0)
      return {HandleType{}};
    else if (handle_reference < handles_.size())
      return {HandleType{handles_[handle_reference]}};
    else
      return ErrorStatus(EINVAL);
  }

  void Set(std::vector<std::uint8_t> data) {
    data_ = std::move(data);
    index_ = 0;
  }

  void SetHandles(std::vector<int> handles) { handles_ = std::move(handles); }

 private:
  std::vector<std::uint8_t> data_;
  std::vector<int> handles_;
  std::size_t index_{0};

  TestReader(const TestReader&) = delete;
  void operator=(const TestReader&) = delete;
};

}  // namespace nop

#endif  // LIBNOP_TEST_TEST_READER_H_
