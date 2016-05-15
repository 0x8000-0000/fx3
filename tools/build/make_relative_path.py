#!/usr/bin/python

import os
import sys

targetDirectory = sys.argv[1]

for fileList in sys.argv[2:]:
   with open(fileList, 'rt') as fp:
      for line in fp:
         relPath = os.path.relpath(line.rstrip(), targetDirectory)
         print(relPath)
