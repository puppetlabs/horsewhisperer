// Refer to https://github.com/philsquared/Catch/blob/master/docs/own-main.md
// for providing our own main function to Catch
#define CATCH_CONFIG_RUNNER

#include "test/test.h"
#include "horsewhisperer/horsewhisperer.h"

std::string ROOT_PATH;

// TODO(ale): manage Catch test case tags; list and describe tags

int main(int argc, char* const argv[]) {
    // configure the Catch session and start it

    // TODO(ale): improve output by properly using reporters
    // (dump the xml to a file and use an external parser)

    Catch::Session test_session;
    test_session.applyCommandLine(argc, argv);

    // To list the reporters use:
    // test_session.configData().listReporters = true;

    // Reporters: "xml", "junit", "console", "compact"
    test_session.configData().reporterName = "console";

    // ShowDurations::Always, ::Never, ::DefaultForReporter
    test_session.configData().showDurations = Catch::ShowDurations::Always;

    return test_session.run();
}
