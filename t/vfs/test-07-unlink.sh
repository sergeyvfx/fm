#!/bin/sh

#
# Copyright (C) 2008 Sergey I. Sharybin
#

echo "Test vfs_unlink()"

./vfs-test --load-localfs --test-unlink > /dev/null 2>&1 ||
  exit 1
