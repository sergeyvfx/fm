#!/bin/sh

#
# Copyright (C) 2008 Sergey I. Sharybin
#

# Exit codes
ok=0
error=1
known_error=2
fixed=3

# Logger function
if test -n "$color"; then
  log() {
    TERM=$ORIGINAL_TERM
    export TERM
    case "$1" in
      header) tput setaf 3;;
      error) tput bold; tput setaf 1;;
      broken) tput bold; tput setaf 2;;
      ok) tput setaf 2;;
      summary-error) tput setaf 1;;
    esac
    shift
    echo "$*"
    tput sgr0
  }
else
  log() {
    shift
    echo "$*"
  }
fi

# Print test description
test "x${test_description}" == "x" || log header "${test_description}"

# Run commands from list

total=0        # Count of total tests
ok_count=0     # Count of successfull tests
error_count=0  # Count of tests with error
known_error_count=0 # Count of tests with known error
fixed_count=0  # Count of tests whith fixed known errors
for t in test-*; do
  if test "x${t}" != "xtest-all.sh"; then
    let total++

    # Run test
    description=`${t}`

    # Review exit code
    exit_code=$?

    if test "x${exit_code}" == "x${ok}"; then
      # ok
      log text "    ok $total: ${description}"
      let ok_count++
    fi

    if test "x${exit_code}" == "x${known_error}"; then
      # test with known error
      log broken "    still broken $total: ${description}"
      let known_error_count++
    fi

    if test "x${exit_code}" == "x${error}"; then
      # error
      log error "    fail $total: ${description}"
      let error_count++
    fi

    if test "x${exit_code}" == "x${fixed}"; then
      # test fixed known error
      log text "    FIXED $total: ${description}"
      let fixed_count++
    fi

  fi
done

# Print summary information about tests set
if test ${fixed_count} -gt 0; then
  log ok "  Fixed ${fixed_count} known breakage(s)"
fi

if test ${total} -gt 0; then
  if test "x${error_count}" == "x0"; then
    if test "x${known_error_count}" == "x0"; then
      log ok "   Passed all ${ok_count} test(s)"
    else
      let remain=total-known_error_count
      log error "  Still have ${known_error_count} breakage(s)"
      log ok "  Passed all remaining ${remain} test(s)"
    fi
  else
    log summary-error " Failed ${error_count} of ${total} test(s)"
    exit 1
  fi
fi

# Run tests from subdirs
if test "x${subdirs}" != "x"; then
  for subdir in ${subdirs}; do
    cd ${subdir}
    make-test.sh
    exit_code=$?
    cd ..

    if test "x${exit_code}" != "x0"; then
      exit 1
    fi
  done
fi

exit 0
