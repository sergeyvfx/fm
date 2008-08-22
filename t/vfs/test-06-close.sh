#!/bin/sh

#
# Copyright (C) 2008 Sergey I. Sharybin
#

echo "Test vfs_close()"

./vfs-test --load-localfs --test-close > /dev/null 2>&1 ||
  exit 1
