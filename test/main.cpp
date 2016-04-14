// Refer to https://github.com/philsquared/Catch/blob/master/docs/own-main.md
// for providing our own main function to Catch
#define CATCH_CONFIG_RUNNER

#include "test/test.h"
#include "horsewhisperer/horsewhisperer.h"
#include <vector>

std::string ROOT_PATH;

// TODO(ale): manage Catch test case tags; list and describe tags

int main(int argc, const char** argv) {
    // Create the Catch session, pass CL args, and start it
    Catch::Session test_session {};
    test_session.applyCommandLine(argc, argv);

    // NOTE(ale): to list the reporters use:
    // test_session.configData().listReporters = true;

    // Reporters: "xml", "junit", "console", and "compact" (single line)
    test_session.configData().reporterNames =
            std::vector<std::string> { "console" };

    // ShowDurations::Always, ::Never, ::DefaultForReporter
    test_session.configData().showDurations = Catch::ShowDurations::Always;

    // NOTE(ale): enforcing ConfigData::useColour == UseColour::No
    // on Windows is not necessary; the default ::Auto works fine

    return test_session.run();
}
