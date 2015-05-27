# Changelog

Change history for HorseWhisperer.

# 0.9.0

Released 2015-05-27

* Fixed the way action flags are stored; previously, in case a given action
was chained and an action flag was specified with different values, the last
flag value would have been used in all action calls. This is now fixed; action
flag values are confined in their action contexts.
* FlagCallback and ArgumentsCallback are a void function type.
* Now flag and action arguments validation callbacks are suppose to throw an
exception in case of invalid arguments.
* New ParseResult enum class.
* The Parse() function now return ParseResult values.

# 0.8.0

Released 2015-05-12

* Add ability to pass flags in --key=value style

# 0.7.0

Released 2015-05-11

* Option arguments are now displayed in the help text
* Action and option descriptions are now displayed with margins in the help text
* Added SetHelpMargins function to set the left and right margins of descriptions
* Added the FlagType enumeration that lists the supported flag types
* Added GetFlagType function that return, for a given flag, an element of the
FlagType enumeration indicating the flag type
* Supporting flags of double type

# 0.6.0

* Added GetParsedActions function that will return a std::vector<std::string> of
actions parsed from a CLI string.

# 0.5.1

Released 2015-03-27

* Fixed a bug where specifying a flag could let HW incorrectly process the arguments of an action

# 0.5.0

Released 2015-02-11

* Vendor catch testing framework
* add unit tests
* SetFlag and GetFlag now throw horsewhisperer_error if looking up defined flag

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
