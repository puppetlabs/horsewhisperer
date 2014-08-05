# Changelog

Change history for HorseWhisperer.

# 0.4.0

Released 2014-08-05

* Added IsActionFlag method which will check if a flag has been defined in an specific action.
* Fixed a bug where calling HorseWhisperer::Parse from an execution context would leave the
context pointer in an incorrect state.

# 0.3.0

Released 2014-07-15

* Improvements and update to example program and testing
* The Parse function now returns an integer indicating its outcome
* New function to display the help message
* New function to display the version string
* ValidateActionArguments now checks if Parse succeeded
* The Start function now checks if Parse succeeded

# 0.2.0

Released 2014-07-09

* Improvements and update to example program
* Improvements to use stl containers instead of free standing pointers
* Improved testing
* Actions with negative arity now parse correctly
* Action validation callbacks have been added


# 0.1.0

Released 2014-05-23

* First Release of HorseWhisperer
