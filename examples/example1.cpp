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

bool validation(int x) {
    // Max value we accept is 5
    if (x > 5) {
        std::cout << "You have assigned too many ponies!" << std::endl;
        return false;
    }
    return true;
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

// trot: internal function to process arguments
bool processTrotArguments_(const std::vector<std::string>& cl_arguments,
                           std::vector<std::string>& processed_arguments){
    for (std::string arg : cl_arguments) {
        if (arg.find("mode", 0) == std::string::npos) {
            std::cout << "Error: invalid trot argument " << arg << ".\n";
            return false;
        } else {
            processed_arguments.push_back(arg.substr(5));
        }
    }
    return true;
}

// trot: arguments callback
bool trotArgumentsCallback(const std::vector<std::string>& arguments) {
    std::vector<std::string> processed_args {};
    return processTrotArguments_(arguments, processed_args);
}

// trot: action callback
int trot(const std::vector<std::string>& arguments) {
    std::vector<std::string> processed_args {};
    if (processTrotArguments_(arguments, processed_args)) {
        for (std::string mode : processed_args) {
            std::cout << "Trotting like a " << mode << std::endl;
        }
        return 0;
    }
    return 1;
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
    if (!Parse(argc, argv)) {
        std::cout << "Failed to parse the command line input.\n";
        return 1;
    }

    // Process and validate action arguments
    if (!ValidateActionArguments()) {
        std::cout << "Failed to validate the action arguments.\n";
        return 1;
    }

    // Start execution
    return Start();
}
