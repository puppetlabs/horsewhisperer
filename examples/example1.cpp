/*
    example1.cpp
    ============

    Basic usage of the Horse Whisperer

    Compile with:
        c++ -std=c++11 example1.cpp -o example

    Run with
        ./example

    Try to run:
        ./example
        ./example --help
        ./example gallop
        ./example gallop --ponies 5
        ./example gallop --ponies 6
        ./example gallop --ponies 5 --tired
        ./example trot 'mode elegant dancer' 'mode drunk panda'
*/
#include "../include/horsewhisperer/horsewhisperer.h"
#include <string>
#include <vector>

using namespace HorseWhisperer;

// flag validation callback
void validation(int x) {
    // Max value we accept is 5
    if (x > 5) {
        throw flag_validation_error { "You have assigned too many ponies!" };
    }
}

// help messages
std::string gallop_help = "The horses, they be a galloping\n";
std::string trot_help = "The horses, they be trotting in some 'mode ...'\n";

// gallop: action callback
int gallop(const Arguments& arguments) {
    for (int i = 0; i < GetFlag<int>("ponies"); i++) {
        if (!GetFlag<int>("tired")) {
            std::cout << "Galloping into the night!" << std::endl;
        } else {
            std::cout << "The pony is too tired to gallop." << std::endl;
        }
    }
    return 0;
}

// trot: arguments validation callback
void trotArgumentsCallback(const std::vector<std::string>& arguments) {
    for (const auto& arg : arguments) {
        if (arg.find("mode", 0) == std::string::npos) {
            throw action_validation_error { "unknown trot mode '" + arg + "'" };
        }
    }
}

// trot: action callback
int trot(const std::vector<std::string>& arguments) {
    for (const auto& mode : arguments) {
        std::cout << "Trotting like a " << mode << std::endl;
    }
    return 0;
}

int main(int argc, char* argv[]) {
    // Configuring the global context
    SetHelpBanner("Usage: MyProg [global options] <action> [options]");
    SetAppName("MyProg");
    SetVersion("MyProg version - 0.1.0\n");
    SetDelimiters(std::vector<std::string>{"+", ";", "_then"});

    // Define global flags
    DefineGlobalFlag<int>("ponies", "all the ponies", 1, validation);

    // Define action: gallop
    DefineAction("gallop", 0, true, "make the ponies gallop", gallop_help, gallop);
    DefineActionFlag<bool>("gallop", "tired", "are the horses tired?", false, nullptr);

    // Define action: trot (at least two arguments are required)
    DefineAction("trot", -2, true, "make the ponies trot in some way", trot_help,
                 trot, trotArgumentsCallback);

    // Parse command line: global flags, action arguments, and action flags
    try {
        switch (Parse(argc, argv)) {
            case ParseResult::OK:
                break;
            case ParseResult::HELP:
                ShowHelp();
                return 0;
            case ParseResult::VERSION:
                ShowVersion();
                return 0;
            case ParseResult::ERROR:
                std::cout << "Failed to parse the command line input.\n";
                return 1;
            case ParseResult::INVALID_FLAG:
                std::cout << "Invalid flag.\n";
                return 2;
        }
    } catch (horsewhisperer_error& e) {
        std::cout << e.what() << "\n";
    }

    // Start execution
    return Start();
}
