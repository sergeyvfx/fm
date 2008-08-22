#!/bin/sh

#
# Copyright (C) 2008 Sergey I. Sharybin
#

echo "Initialize VFS module"

./vfs-test > /dev/null 2>&1 ||
  exit 1
