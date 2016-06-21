Horse Whisperer Tests
===

System Tests
---

The `sytems_test_runner.sh` script consists of a number of assertion tests over
a simple cli application that use on Horse Whisperer.
The application source code is contained in `../examples/example1.cpp`.
It must be compiled; the executable must be named `example` and stored in the
`../examples` directory before running the script.

Unit Tests
---

The unit tests for Horse Whisperer are based on the
[Catch framework](https://github.com/philsquared/Catch).

To run the tests, simply execute:

```
    cd <HORSEWHISPERER root dir>/test
    mkdir build
    cd build
    cmake ..
    make
    ./horsewhisperer-unittests
```
