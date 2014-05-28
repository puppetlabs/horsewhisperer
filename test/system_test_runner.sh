#!/usr/bin/env sh

#     system_testr_unner.sh
#     =====================
#
#     Script to automate the execution of a few simple system tests
#     for horsewhisperer.
#     It is based on example1.cpp (client code), which is assumed to
#     be compiled as 'example' and located in the ../example directory.

# exit codes
EXIT_FAILURE=1

# executable of example1.cpp
EXAMPLE=../examples/example

check_strings() {
  if [ "$1" != "$2" ]; then
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
if [ $MSG_NUM -ne 4 ]; then
  exit $EXIT_FAILURE
fi

# action flag: we expect a different message from the action callback
TIRED_MSG=`"$EXAMPLE" gallop --tired`
check_strings "$TIRED_MSG" 'The pony is too tired to gallop.'
