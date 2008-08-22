#!/bin/sh

#
# Copyright (C) 2008 Sergey I. Sharybin
#

echo "Test vfs_read()"

./vfs-test --load-localfs --test-read > /dev/null 2>&1 ||
  exit 1
