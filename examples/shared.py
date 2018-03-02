# Copyright 2018 The Native Object Protocols Authors
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

#
# Example of using libnop from python by wrapping operations in a shared
# library. The shared library is loaded using the ctypes module.
#

from ctypes import *
import sys
import os

# Define ctypes structures that mirror the basic protocol types.

# Three component vector of floats.
class Vec3(Structure):
  _fields_ = (('x', c_float), ('y', c_float), ('z', c_float))

  def __str__(self):
    return '(%.1f, %.1f, %.1f)' % (self.x, self.y, self.z)

# Triangle composed of three Vec3 structures.
class Triangle(Structure):
  _fields_ = (('a', Vec3), ('b', Vec3), ('c', Vec3))

  def __str__(self):
    return 'Triangle{%s, %s, %s}' % (self.a, self.b, self.c)

# Base Polyhedron structure. This carries the size of the Triangle array
# appended to the end of the structure.
class PolyhedronBase(Structure):
  _fields_ = (('size', c_size_t),)

# Builds a Polyhedron given either a list of Triangles or a capacity to reserve.
#
# This is equivalent to the following C header struct + dynamic array idiom:
#
# struct Polyhedron {
#   size_t size;
#   Triangle triangles[1];
# };
#
# Polyhedron* p = malloc(offsetof(Polyhedron, triangles) + reserve * sizeof(Triangle));
#
def Polyhedron(elements=[], reserve=None):
  if reserve is None:
    reserve = len(elements)

  # Triangle array of the required size.
  class ArrayType(Array):
    _type_ = Triangle
    _length_ = reserve

  # A subclass of PolyhedronBase with an array of Triangles appended.
  class PolyhedronType(PolyhedronBase):
    _fields_ = (('triangles', ArrayType),)

    def __str__(self):
      return 'Polyhedron{' + ', '.join(
        [str(self.triangles[i]) for i in range(self.size)]
      ) + '}'

  # Return an initialized instance of the Polyhedron.
  return PolyhedronType(reserve, ArrayType(*elements))

def LoadProtocolLibrary():
  global ProtocolLibrary
  global GetSerializedPolyhedron
  global SerializePolyhedron
  global DeserializePolyhedron

  # Load the shared library.
  ProtocolLibrary = cdll.LoadLibrary('out/shared_protocol.so')

  # Load the exported API and setup the function pointer types.
  GetSerializedPolyhedron = ProtocolLibrary.GetSerializedPolyhedron
  GetSerializedPolyhedron.argtypes = (c_void_p, c_size_t)
  GetSerializedPolyhedron.restype = c_ssize_t

  SerializePolyhedron = ProtocolLibrary.SerializePolyhedron
  SerializePolyhedron.argtypes = (POINTER(PolyhedronBase), c_void_p, c_size_t)
  SerializePolyhedron.restype = c_ssize_t

  DeserializePolyhedron = ProtocolLibrary.DeserializePolyhedron
  DeserializePolyhedron.argtypes = (POINTER(PolyhedronBase), c_void_p, c_size_t)
  DeserializePolyhedron.restype = c_ssize_t

def main():
  LoadProtocolLibrary()

  # Create a buffer to hold the serialized payload.
  payload_buffer = create_string_buffer(1024)

  # Create a Polyhedron and serialize it to the buffer.
  polyhedron = Polyhedron(
    (
      Triangle(Vec3(0, 0, 0), Vec3(1, 1, 1), Vec3(2, 2, 2)),
      Triangle(Vec3(3, 3, 3), Vec3(4, 4, 4), Vec3(5, 5, 5)),
      Triangle(Vec3(6, 6, 6), Vec3(7, 7, 7), Vec3(8, 8, 8))
    )
  )

  count = SerializePolyhedron(polyhedron, payload_buffer, len(payload_buffer))
  if count >= 0:
    print count, 'bytes:', payload_buffer[0:count].encode('hex')
  else:
    print 'Error:', -count

  # Create an empty Polyhedron that can hold up to 10 triangles read the payload.
  polyhedron = Polyhedron(reserve=10)

  count = DeserializePolyhedron(polyhedron, payload_buffer, len(payload_buffer))
  if count >= 0:
    print polyhedron
  else:
    print 'Error:', -count

  # Get a serialized Polyhedron from the library and then deserialize it.
  count = GetSerializedPolyhedron(payload_buffer, len(payload_buffer))
  if count < 0:
    print 'Error:', -count
    return;

  count = DeserializePolyhedron(polyhedron, payload_buffer, len(payload_buffer))
  if count >= 0:
    print polyhedron
  else:
    print 'Error:', -count

if __name__ == '__main__':
  main()
