#!/bin/sh

#
# Copyright (C) 2008 Sergey I. Sharybin
#

echo "Test vfs_lseek()"

./vfs-test --load-localfs --test-lseek > /dev/null 2>&1 ||
  exit 1
