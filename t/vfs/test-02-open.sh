#!/bin/sh

#
# Copyright (C) 2008 Sergey I. Sharybin
#

echo "Test vfs_open()"

./vfs-test --load-localfs --test-open > /dev/null 2>&1 ||
  exit 1
