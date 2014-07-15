#!/usr/bin/env sh

#     system_test_runner.sh
#     =====================
#
#     Script to automate the execution of a few simple system tests
#     for horsewhisperer.
#     It is based on example1.cpp (client code), which is assumed to
#     be compiled as 'example' and located in the ../example directory.

# exit codes
EXIT_FAILURE=1
EXIT_MISSING_EXECUTABLE=3

# executable of example1.cpp
EXAMPLE=../examples/example

if [ ! -e ${EXAMPLE} ]; then
  exit $EXIT_MISSING_EXECUTABLE
fi

check_strings() {
  if [ "$1" != "$2" ]; then
    exit $EXIT_FAILURE
  fi
}

check_numeric_pair() {
  if [ $1 -ne $2 ]; then
    exit $EXIT_FAILURE
  fi
}

# help: we expect the usage message
USAGE_LABEL=`"$EXAMPLE" --help | head -n 1 | awk '{print $1}'`
check_strings "$USAGE_LABEL" 'Usage:'

# no args: we expect the "No action specified" message
NO_ACTION_MSG=`"$EXAMPLE" | awk '{print $1, $2, $3}'`
check_strings "$NO_ACTION_MSG" "No action specified."

# action: we expect the message displayed by the action callback
ACTION_MSG=`"$EXAMPLE" gallop| awk '{print $1}'`
check_strings "$ACTION_MSG" 'Galloping'

# global flag: we expect the above message 4 times
MSG_NUM=`"$EXAMPLE" gallop --ponies 4 | awk '{ if ( $1 ~ /Galloping/ ) {print $1} }' | wc -l`
check_numeric_pair $MSG_NUM 4

# action flag: we expect a different message from the action callback
TIRED_MSG=`"$EXAMPLE" gallop --tired`
check_strings "$TIRED_MSG" 'The pony is too tired to gallop.'

# no action arguments specified: we expect an error message
ARG_ERROR=`"$EXAMPLE" trot`
check_strings "$ARG_ERROR" '''No arguments specified for trot.
Failed to parse the command line input.'''

# not all action arguments specified: we expect an error message
ARG_ERROR=`"$EXAMPLE" trot 'mode foo'`
check_strings "$ARG_ERROR" '''Expected at least 2 parameters for action trot. Only read 1.
Failed to parse the command line input.'''

# action arguments: we expect it works with at least 2 correct arguments
TROT_MSG_NUM=`"$EXAMPLE" trot 'mode world champion' 'mode panda' 'mode rhino' | grep Trotting | wc -l`
check_numeric_pair $TROT_MSG_NUM 3

# invalid action arguments: we expect a validation error
INVALID_ARG_ERROR=`"$EXAMPLE" trot 'mode world champion' panda`
check_strings "$INVALID_ARG_ERROR" '''Error: invalid trot argument panda.
Failed to validate the action arguments.'''

# invalid action arguments: we expect a validation error when chaining
INVALID_ARG_ERROR=`"$EXAMPLE" gallop + trot 'mode world champion' 'panda way' --ponies 3`
check_strings "$INVALID_ARG_ERROR" '''Error: invalid trot argument panda way.
Failed to validate the action arguments.'''

# chaining works
CHAINING_MSG=`"$EXAMPLE" gallop + trot 'mode one' 'mode two' 'mode three' --ponies 4`
check_numeric_pair `echo "$CHAINING_MSG" | grep Galloping | wc -l` 4
check_numeric_pair `echo "$CHAINING_MSG" | grep Trotting | wc -l` 3

# return code: valid request
"$EXAMPLE" gallop > /dev/null 2>&1
check_numeric_pair $? 0

# return code: fails to parse
"$EXAMPLE" gallop "invalid_argument" > /dev/null 2>&1
check_numeric_pair $? 1

# return code: invalid flag type
"$EXAMPLE" gallop --ponies "forty two" > /dev/null 2>&1
check_numeric_pair $? 2

# return code: help request
"$EXAMPLE" --help > /dev/null 2>&1
check_numeric_pair $? 0

# return code: action help request
"$EXAMPLE" trot --help > /dev/null 2>&1
check_numeric_pair $? 0

# return code: version request
"$EXAMPLE" --version > /dev/null 2>&1
check_numeric_pair $? 0
