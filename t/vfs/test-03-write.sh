#!/bin/sh

#
# Copyright (C) 2008 Sergey I. Sharybin
#

echo "Test vfs_write()"

./vfs-test --load-localfs --test-write > /dev/null 2>&1 ||
  exit 1
