#ifndef LIBNOP_TEST_TEST_READER_H_
#define LIBNOP_TEST_TEST_READER_H_

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <type_traits>
#include <vector>

#include <nop/base/encoding.h>
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

  Status<void> Read(std::uint8_t* value) {
    if (index_ < data_.size()) {
      *value = static_cast<std::uint8_t>(data_[index_++]);
      return {};
    } else {
      return ErrorStatus(ENOBUFS);
    }
  }

  Status<void> Read(std::uint16_t* value) {
    std::uint8_t low, high;

    auto status = Read(&low);
    if (!status)
      return status;

    status = Read(&high);
    if (!status)
      return status;

    *value = (static_cast<std::uint16_t>(high) << 8) |
             static_cast<std::uint16_t>(low);
    return {};
  }

  Status<void> Read(std::uint32_t* value) {
    std::uint16_t low, high;

    auto status = Read(&low);
    if (!status)
      return status;

    status = Read(&high);
    if (!status)
      return status;

    *value = (static_cast<std::uint32_t>(high) << 16) |
             static_cast<std::uint32_t>(low);
    return {};
  }

  Status<void> Read(std::uint64_t* value) {
    std::uint32_t low, high;

    auto status = Read(&low);
    if (!status)
      return status;

    status = Read(&high);
    if (!status)
      return status;

    *value = (static_cast<std::uint64_t>(high) << 32) |
             static_cast<std::uint64_t>(low);
    return {};
  }

  Status<void> Read(std::int8_t* value) {
    return Read(reinterpret_cast<std::uint8_t*>(value));
  }

  Status<void> Read(std::int16_t* value) {
    return Read(reinterpret_cast<std::uint16_t*>(value));
  }

  Status<void> Read(std::int32_t* value) {
    return Read(reinterpret_cast<std::uint32_t*>(value));
  }

  Status<void> Read(std::int64_t* value) {
    return Read(reinterpret_cast<std::uint64_t*>(value));
  }

  Status<void> Read(char* value) {
    return Read(reinterpret_cast<std::uint8_t*>(value));
  }

  template <typename T, typename U, typename Enabled = EnableIfIntegral<T, U>>
  Status<void> Read(U* value) {
    T temp;

    auto status = Read(&temp);
    if (!status)
      return status;

    *value = static_cast<U>(temp);
    return {};
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
