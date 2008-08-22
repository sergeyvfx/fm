#!/bin/sh

#
# Copyright (C) 2008 Sergey I. Sharybin
#

echo "Load localfs plugin"

./vfs-test --load-localfs > /dev/null 2>&1 ||
  exit 1
