#!/usr/bin/env python

import glob
import re
import os

reInclude = re.compile("""^.*(#include\s+<(nop/[^>]+)>)""", re.M)

def main():
  all_headers = dict()
  included_headers = set()

  def entry(arg, dirname, names):
    for name in names:
      pathname = os.path.join(dirname, name)
      basepath, extension = os.path.splitext(pathname)
      if extension == ".h":
        all_headers[pathname] = file(pathname, 'r').read()

  os.path.walk('include/nop', entry, None)

  print sorted(all_headers.keys())

  def process(header):
    if header in included_headers:
      return ''

    print 'Processing:', header
    included_headers.add(header)

    f = file(header, 'r')
    contents = f.read()

    matches = reInclude.finditer(contents)
    includes = [(match.groups()[0], os.path.join('include', match.groups()[1])) for match in matches]
    print includes
    for pattern, include in includes:
      print 'Including:', include
      contents = re.sub(re.escape(pattern), process(include), contents, 0)

    return contents

  aggregate_header = ''

  for h in glob.glob('include/nop/*.h') + ['include/nop/utility/buffer_reader.h', 'include/nop/utility/buffer_writer.h', 'include/nop/utility/stream_reader.h', 'include/nop/utility/stream_writer.h']:
    aggregate_header += process(h)

  outfile = file('out/unified.h', 'w')
  outfile.write(aggregate_header)
  outfile.close()

if __name__ == '__main__':
  main()

