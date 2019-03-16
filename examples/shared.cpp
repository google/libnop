// Copyright 2018 The Native Object Protocols Authors
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

//
// Example of using libnop from python by wrapping operations in a shared
// library. The shared library is loaded using the ctypes module.
//

#include <cstddef>
#include <cstdint>
#include <vector>

#include <nop/protocol.h>
#include <nop/serializer.h>
#include <nop/structure.h>
#include <nop/traits/is_fungible.h>
#include <nop/utility/buffer_reader.h>
#include <nop/utility/buffer_writer.h>
#include <nop/value.h>

namespace {

// Define the basic protocol types. In the real world this would be in a common
// header shared by all the code that wishes to speak this protocol.

// Three component vector of floats.
struct Vec3 {
  float x;
  float y;
  float z;
  NOP_STRUCTURE(Vec3, x, y, z);
};

// Triangle composed of three Vec3 structures.
struct Triangle {
  Vec3 a;
  Vec3 b;
  Vec3 c;
  NOP_STRUCTURE(Triangle, a, b, c);
};

// Polyhedron composed of a number of Triangle structres.
struct Polyhedron {
  std::vector<Triangle> triangles;
  NOP_VALUE(Polyhedron, triangles);
};

// This is a special type that is fungible with Polyhedron but fits the C ABI
// required to interface with python. The python ctypes library can't easily
// interface with C++ types, such as std::vector (which is more convenient for
// C++ code to use), so this type may be substituted in the API this library
// exports.
//
// This type is meant to be used as the header of a dynamic C array. C code
// might allocate the header + array as follows:
//
//  CPolyhedron* polyhedron =
//    malloc(offsetof(CPolyhedron, triangles) + array_size * sizeof(Triangles));
//
// The python code uses ctypes structures and arrays to achieve the equivalent.
//
struct CPolyhedron {
  size_t size;
  Triangle triangles[1];
  NOP_VALUE(CPolyhedron, (triangles, size));
  NOP_UNBOUNDED_BUFFER(CPolyhedron);
};

// Assert that Polyhedron and CPolyhedron have compatible wire formats.
static_assert(nop::IsFungible<Polyhedron, CPolyhedron>::value, "");

}  // anonymous namespace

// Fills the given buffer with a pre-defined Polyhedron.
extern "C" ssize_t GetSerializedPolyhedron(void* buffer, size_t buffer_size) {
  nop::Serializer<nop::BufferWriter> serializer{buffer, buffer_size};

  Polyhedron polyhedron{
      {Triangle{{1.f, 2.f, 3.f}, {4.f, 5.f, 6.f}, {7.f, 8.f, 9.f}},
       Triangle{{10.f, 11.f, 12.f}, {13.f, 14.f, 15.f}, {16.f, 17.f, 18.f}},
       Triangle{{19.f, 20.f, 21.f}, {22.f, 23.f, 24.f}, {25.f, 26.f, 27.f}}}};

  auto status = nop::Protocol<Polyhedron>::Write(&serializer, polyhedron);
  if (!status)
    return -static_cast<ssize_t>(status.error());
  else
    return serializer.writer().size();
}

// Serializes the given CPolyhedron into the given buffer.
extern "C" ssize_t SerializePolyhedron(const CPolyhedron* poly, void* buffer,
                                       size_t buffer_size) {
  nop::Serializer<nop::BufferWriter> serializer{buffer, buffer_size};
  auto status = nop::Protocol<Polyhedron>::Write(&serializer, *poly);
  if (!status)
    return -static_cast<ssize_t>(status.error());
  else
    return serializer.writer().size();
}

// Deserializes the given buffer into the given CPolyhedron. The CPolyhedron
// should be allocated with enough capacity to handle the expected number of
// triangles.
extern "C" ssize_t DeserializePolyhedron(CPolyhedron* poly, const void* buffer,
                                         size_t buffer_size) {
  nop::Deserializer<nop::BufferReader> deserializer{buffer, buffer_size};

  auto status = nop::Protocol<Polyhedron>::Read(&deserializer, poly);
  if (!status)
    return -static_cast<ssize_t>(status.error());
  else
    return 0;
}
