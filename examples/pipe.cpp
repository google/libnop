// Copyright 2017 The Native Object Protocols Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <errno.h>
#include <unistd.h>

#include <iostream>
#include <string>
#include <tuple>
#include <utility>

#include <nop/serializer.h>
#include <nop/status.h>
#include <nop/structure.h>
#include <nop/types/file_handle.h>
#include <nop/types/result.h>
#include <nop/utility/die.h>
#include <nop/utility/stream_reader.h>
#include <nop/utility/stream_writer.h>

#include "stream_utilities.h"
#include "string_to_hex.h"

using nop::Deserializer;
using nop::ErrorStatus;
using nop::Result;
using nop::Serializer;
using nop::Status;
using nop::StreamReader;
using nop::StreamWriter;
using nop::StringToHex;
using nop::UniqueFileHandle;

namespace {

auto Die(const char* error_message = "Error") {
  return nop::Die(std::cerr, error_message);
}

using Reader = StreamReader<FdInputStream>;
using Writer = StreamWriter<FdOutputStream>;

// Utility class to combine a serializer and deserializer into a single
// bi-directional entity.
struct Channel {
  Serializer<std::unique_ptr<Writer>> serializer;
  Deserializer<std::unique_ptr<Reader>> deserializer;

  template <typename T>
  Status<void> Read(T* value) {
    return deserializer.Read(value);
  }

  template <typename T>
  Status<void> Write(const T& value) {
    return serializer.Write(value);
  }
};

// Defines the format of request messages sent from parent to child.
struct Request {
  std::uint32_t request_bytes{0};
  NOP_STRUCTURE(Request, request_bytes);
};

// Defines the errors that may be returned from child to parent.
enum class Error {
  None,
  InvalidRequest,
  InternalError,
};

// Defines a subclass of nop::Result for the error type used in this example.
template <typename T>
struct Return : Result<Error, T> {
  using Base = Result<Error, T>;
  using Base::Base;

  std::string GetErrorMessage() const {
    switch (this->error()) {
      case Error::None:
        return "No Error";
      case Error::InvalidRequest:
        return "Invalid Request";
      case Error::InternalError:
        return "Internal Error";
      default:
        return "Unknown Error";
    }
  }
};

// Prints the Return<T> to the given stream.
template <typename T>
std::ostream& operator<<(std::ostream& stream, const Return<T>& result) {
  stream << result.GetErrorMessage();
  return stream;
}

// Response type returned from child to parent.
using Response = Return<std::string>;

// Builds a pair of channels connected by a pair of pipes.
Status<std::pair<Channel, Channel>> MakeChannels() {
  int pipe_fds[2];
  int ret = pipe(pipe_fds);
  if (ret < 0)
    return ErrorStatus::SystemError;

  // The first fd is the read end, the second is the write end.
  auto reader_a = std::make_unique<Reader>(pipe_fds[0]);
  auto writer_b = std::make_unique<Writer>(pipe_fds[1]);

  ret = pipe(pipe_fds);
  if (ret < 0)
    return ErrorStatus::SystemError;

  // The first fd is the read end, the second is the write end.
  auto reader_b = std::make_unique<Reader>(pipe_fds[0]);
  auto writer_a = std::make_unique<Writer>(pipe_fds[1]);

  // Criss-cross the pipe ends to make two bi-directional channels.
  return {{Channel{std::move(writer_a), std::move(reader_a)},
           Channel{std::move(writer_b), std::move(reader_b)}}};
}

int HandleChild(Channel channel) {
  std::cout << "Child waiting for message..." << std::endl;

  Request request;
  auto status = channel.Read(&request) || Die("Child failed to read request");

  std::cout << "Child received a request for " << request.request_bytes
            << " bytes." << std::endl;

  UniqueFileHandle handle = UniqueFileHandle::Open("/dev/urandom", O_RDONLY);
  if (!handle) {
    const int error = errno;
    std::cerr << "Child failed to open file: " << strerror(error) << std::endl;
    status = channel.Write(Response{Error::InternalError});
  } else {
    std::string data(request.request_bytes, '\0');
    int count = read(handle.get(), &data[0], data.size());
    if (count < 0) {
      status = ErrorStatus::IOError;
    } else {
      data.resize(count);

      std::cout << "Client replying with    : " << StringToHex(data)
                << std::endl;
      status = channel.Write(Response{std::move(data)});
    }
  }

  if (!status) {
    std::cerr << "Child failed to write response: " << status.GetErrorMessage()
              << std::endl;
    return -1;
  }

  return 0;
}

int HandleParent(Channel channel) {
  std::cout << "Parent sending message..." << std::endl;

  const std::uint32_t kRequestBytes = 32;
  channel.Write(Request{kRequestBytes}) ||
      Die("Parent failed to write request");

  Response response;
  channel.Read(&response) || Die("Parent failed to read response");
  if (response) {
    std::cout << "Parent received " << response.get().size()
              << " bytes: " << StringToHex(response.get()) << std::endl;
  } else {
    std::cout << "Parent received error: " << response << std::endl;
  }

  return 0;
}

}  // anonymous namespace

int main(int /*argc*/, char** /*argv*/) {
  auto pipe_status = MakeChannels() || Die("Failed to create pipe");

  Channel parent_channel, child_channel;
  std::tie(parent_channel, child_channel) = pipe_status.take();

  pid_t pid = fork();
  if (pid == -1) {
    const int error = errno;
    std::cerr << "Failed to fork child: " << strerror(error) << std::endl;
    return -error;
  } else if (pid == 0) {
    return HandleChild(std::move(child_channel));
  } else {
    return HandleParent(std::move(parent_channel));
  }
}
