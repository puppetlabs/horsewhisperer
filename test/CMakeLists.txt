cmake_minimum_required(VERSION 2.8.12)
project(horsewhisperer-tests C CXX)

set(BASEPATH "${CMAKE_CURRENT_LIST_DIR}/..")
set(VENDOR_DIRECTORY "${BASEPATH}/vendor")
set(CATCH_DIRECTORY "${VENDOR_DIRECTORY}/catch")
set(INCLUDE_DIRECTORY "${BASEPATH}/include")

include_directories("${BASEPATH}")
include_directories("${INCLUDE_DIRECTORY}")

set(SOURCES
    unit/horsewhisperer_test.cpp
    main.cpp
)

set(test_BIN horsewhisperer-unittests)
set(CMAKE_CXX_FLAGS "-std=c++11")

include_directories(
    ${CATCH_DIRECTORY}
)

ADD_EXECUTABLE(${test_BIN} ${SOURCES})
TARGET_LINK_LIBRARIES(
    ${test_BIN}
)

enable_testing()
add_test(NAME "HorseWhisperer\\ tests" COMMAND ${test_BIN})


