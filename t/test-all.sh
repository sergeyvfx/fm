#!/bin/sh

#
# Copyright (C) 2008 Sergey I. Sharybin
#

ORIGINAL_TERM=$TERM # need for log()

TERM=dumb
export TERM ORIGINAL_TERM


if test -f .tests-passed; then
  # All tests has been already passed
  exit 0
fi

root_dir="`dirname $0`"
export subdirs="vfs"

# Determine if we could use color output
[ "x$ORIGINAL_TERM" != "xdumb" ] && (
    TERM=$ORIGINAL_TERM &&
    [ -t 1 ] &&
    tput bold 2>&1 &&
    tput setaf 1 2>&1 &&
    tput sgr0 1 2>&1
  ) &&
  color=t

# Introdunce message
hello_string="Started testing of all stuffs"

# Parse argument line
while test "$#" -ne 0; do
  case "$1" in
	  --no-color)
        # There will be no color output
		    export color=
        shift
      ;;
    *)
      break;
    ;;
  esac
done

export color

# Print introduce message
if test "x${hello_string}" != "x"; then
  if test -n "$color"; then
    tput setaf 7
    echo ${hello_string}
    tput sgr0
  else
    echo ${hello_string}
  fi
fi

# Run main stuff of testing process
${root_dir}/testlib.sh
exit_code=$?

# If testing finished with zero exit code,
# we can save that tests have been passed
if test "x${exit_code}" == "x0"; then
  touch .tests-passed
else
  # Otherwise we should exit with non-zero status
  exit 1
fi

exit 0
