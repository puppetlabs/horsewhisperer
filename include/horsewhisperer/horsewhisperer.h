#ifndef HORSEWHISPERER_INCLUDE_HORSE_WHISPERER_H_
#define HORSEWHISPERER_INCLUDE_HORSE_WHISPERER_H_

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <algorithm>
#include <functional>
#include <memory>
#include <stdexcept>

// To disable assert()
#define NDEBUG
#include <cassert>


namespace HorseWhisperer {

//
// Exceptions
//

class horsewhisperer_error : public std::runtime_error {
  public:
    explicit horsewhisperer_error(std::string const& msg) :
            std::runtime_error(msg) {}
};

class undefined_flag_error : public horsewhisperer_error {
  public:
    explicit undefined_flag_error(std::string const& msg) :
            horsewhisperer_error(msg) {}
};

class flag_validation_error : public horsewhisperer_error {
  public:
    explicit flag_validation_error(std::string const& msg) :
            horsewhisperer_error(msg) {}
};

//
// Tokens
//

static const std::string VERSION_STRING = "0.6.0";

// Context indexes
static const int GLOBAL_CONTEXT_IDX = 0;
static const int NO_CONTEXT_IDX = -1;

// Parse results
static const int PARSE_OK = 0;
static const int PARSE_HELP = -1;
static const int PARSE_VERSION = -2;
static const int PARSE_ERROR = 1;
static const int PARSE_INVALID_FLAG = 2;

// Right margin for help descriptions
static const unsigned int DESCRIPTION_MARGIN_LEFT_DEFAULT = 30;
static const unsigned int DESCRIPTION_MARGIN_RIGHT_DEFAULT = 80;

//
// Types
//

enum FlagType { Bool, Int, Double, String };

template <typename Type>
using FlagCallback = std::function<bool(Type)>;

using Arguments = std::vector<std::string>;

using ArgumentsCallback = std::function<bool(const Arguments& arguments)>;

using ActionCallback = std::function<int(const Arguments& arguments)>;

struct FlagBase {
    virtual ~FlagBase() {};
    std::string aliases;
    std::string description;
};

template <typename Type>
struct Flag : FlagBase {
    Type value;
    FlagCallback<Type> flag_callback;
};

struct Action {
    ~Action() {
        for (auto& flag : flags) {
            delete flag.second;
        }
    }
    // Action name
    std::string name;
    // Keys local to the action
    std::map<std::string, FlagBase*> flags;
    // Action description
    std::string description;
    // Arity of the action
    int arity;
    // Function called when we invoke the action
    ActionCallback action_callback;
    // Function called when we validate action arguments
    ArgumentsCallback arguments_callback;
    // Context sensitive action help
    std::string help_string_;
    // Wheter the action succeded
    bool success;
    // Whenter the action can be chained with other actions
    bool chainable;
};

struct Context {
    // Flags defined for the given context
    std::map<std::string, FlagBase*> flags;
    // What this context is doing
    Action* action;
    // Action arguments
    Arguments arguments;
};

typedef std::unique_ptr<Context> ContextPtr;

//
// API Declarations
//

template <typename Type>
static void DefineGlobalFlag(std::string aliases,
                             std::string description,
                             Type default_value,
                             FlagCallback<Type> flag_callback) __attribute__ ((unused));
template <typename Type>
static void DefineActionFlag(std::string action_name,
                             std::string aliases,
                             std::string description,
                             Type default_value,
                             FlagCallback<Type> flag_callback) __attribute__ ((unused));
static bool IsActionFlag(std::string action, std::string flagname) __attribute__ ((unused));
template <typename Type>
static Type GetFlag(std::string flag_name) __attribute__ ((unused));
// Throws undefined_flag_error in case the specified flag is unknown
static FlagType GetFlagType(std::string flag_name) __attribute__ ((unused));
template <typename Type>
static void SetFlag(std::string flag_name, Type value) __attribute__ ((unused));
static void DefineAction(std::string action_name,
                         int arity,
                         bool chainable,
                         std::string description,
                         std::string help_string,
                         ActionCallback action_callback,
                         ArgumentsCallback arguments_callback) __attribute__ ((unused));
static void SetAppName(std::string name) __attribute__ ((unused));
static void SetHelpBanner(std::string banner) __attribute__ ((unused));
static void SetVersion(std::string version) __attribute__ ((unused));
static void SetDelimiters(std::vector<std::string> delimiters) __attribute__ ((unused));
static int Parse(int argc, char** argv) __attribute__ ((unused));
static bool ValidateActionArguments() __attribute__ ((unused));
static void ShowHelp() __attribute__ ((unused));
static void ShowVersion() __attribute__ ((unused));
static std::vector<std::string> GetParsedActions() __attribute__ ((unused));
static int Start() __attribute__ ((unused));
static void Reset() __attribute__ ((unused));
static void SetHelpMargins(unsigned int left_margin,
                           unsigned int right_margin) __attribute__ ((unused));

//
// Auxiliary Functions
//

// Because regex is busted on a lot of versions of libstdc++ I'm rolling
// my own integer validation.
static bool validateInteger(const std::string& val) {
    for (size_t i = 0; i < val.size(); i++) {
        if ((val[i] < '0') || (val[i] > '9')) {
            return false;
        }
    }
    return true;
}

static bool validateDouble(const std::string& val) {
    std::istringstream i_s { val };
    double x {};
    char c;
    if (!(i_s >> x) || i_s.get(c)) {
        return false;
    }
    return true;
}

static std::vector<std::string> wordWrap(const std::string& txt,
                                         const unsigned int width) {
    std::istringstream input { txt };
    std::vector<std::string> lines {};
    std::string current_word {};
    std::string current_line {};

    while (getline(input, current_word, ' ')) {
        if (current_line.size() + current_word.size() >= width) {
            lines.push_back(current_line);
            current_line = current_word;
        } else {
            current_word = (current_line.empty() ? "" : " ") + current_word;
            current_line += current_word;
        }
    }

    if (!current_line.empty()) {
        lines.push_back(current_line);
    }

    return lines;
}

//
// HorseWhisperer
//

class HorseWhisperer {
  public:
    // Return reference to instance of HorseWhisperer singleton
    static HorseWhisperer& Instance() {
        static HorseWhisperer instance;
        return instance;
    }

    // No args constructor creates the pointer to the root context and implicitly
    // declares the --help flag.
    HorseWhisperer() {
        ContextPtr global_context {new Context()};
        global_context->action = nullptr;
        context_mgr.push_back(std::move(global_context));
        current_context_idx = GLOBAL_CONTEXT_IDX;
        defineGlobalFlag<bool>("h help", "Show this message", false, nullptr);
        defineGlobalFlag<int>("vlevel", "", 0, nullptr);
        defineGlobalFlag<bool>("verbose", "Set verbose output", false, [this](bool) {setFlag<int>("vlevel", 1); return true;});
    }

    void setAppName(std::string name) {
        application_name_ = name;
    }

    void setHelpBanner(std::string banner) {
        help_banner_ = banner;
    }

    void setVersionString(std::string version_string) {
        version_string_ = version_string;
        defineGlobalFlag<bool>("version", "Display version information", false, nullptr);
    }

    void setDelimiters(std::vector<std::string> delimiters) {
        delimiters_ = delimiters;
    }

    bool isDelimiter(const char* argument) {
        if (std::find(delimiters_.begin(), delimiters_.end(), argument) != delimiters_.end()) {
            return true;
        }
        return false;
    }

    bool isActionFlag(std::string action, std::string flagname) {
        for (const auto& flag : actions[action]->flags) {
            if (flagname.compare(flag.first) == 0) {
                return true;
            }
        }

        return false;
    }

    void reset() {
        context_mgr.clear();
        ContextPtr global_context { new Context() };
        global_context->action = nullptr;
        context_mgr.push_back(std::move(global_context));
        current_context_idx = GLOBAL_CONTEXT_IDX;
    }

    int parse(int argc, char* argv[]) {
        for (int arg_idx = 1; arg_idx < argc; arg_idx++) {
            // Identify if it's a flag
            if (argv[arg_idx][0] == '-') {
                int parse_flag_outcome { parseFlag(argv, arg_idx ) };
                if (parse_flag_outcome != PARSE_OK) {
                    return parse_flag_outcome;
                }
            } else if (isDelimiter(argv[arg_idx])) { // skip over delimiter
                continue;
            } else {
                std::string action = argv[arg_idx];
                if (isActionDefined(action)) {
                    ContextPtr action_context {new Context()};
                    action_context->flags = actions[argv[arg_idx]]->flags;
                    action_context->action = actions[argv[arg_idx]];
                    action_context->arguments = Arguments {};
                    context_mgr.push_back(std::move(action_context));
                    current_context_idx++;

                    assert(current_context_idx == context_mgr.size() - 1);

                    // parse arguments and action flags
                    int arity = context_mgr[current_context_idx]->action->arity;
                    if (arity > 0) { // iff read parameters = arity
                        while (arity > 0) {
                            ++arg_idx;
                            if (arg_idx >= argc) { // have we run out of tokens?
                                break;
                            } else if (argv[arg_idx][0] == '-') { // is it a flag token?
                                int parse_flag_outcome { parseFlag(argv, arg_idx ) };
                                if (parse_flag_outcome != PARSE_OK) {
                                    return parse_flag_outcome;
                                }
                            } else if (isActionDefined(argv[arg_idx])) { // is it an action?
                                std::cout << "Expected parameter for action: " << action
                                          << ". Found action: " << argv[arg_idx] << std::endl;
                                return PARSE_ERROR;
                            } else if (std::find(delimiters_.begin(), delimiters_.end(),
                                                 argv[arg_idx]) != delimiters_.end()) { // is it a delimiter?
                                std::cout << "Expected parameter for action: " << action
                                          << ". Found delimiter: " << argv[arg_idx] << std::endl;
                                return PARSE_ERROR;
                            } else {
                                context_mgr[current_context_idx]->arguments.push_back(argv[arg_idx]);
                                arity--;
                            }
                        }

                        if (arity > 0) {
                            std::cout << "Expected " << context_mgr[current_context_idx]->action->arity
                                      << " parameters for action " << action << ". Only read "
                                      << context_mgr[current_context_idx]->action->arity - arity
                                      << "." << std::endl;
                            return PARSE_ERROR;
                        }
                    } else if (arity < 0) { // if read parameters at least = arity
                        // When arity is an "at least" representation we eat arguments
                        // until we either run out or until we hit a delimiter.

                        if (arg_idx >= argc - 1) {
                            std::cout << "No arguments specified for " << action << ".\n";
                            return PARSE_ERROR;
                        }

                        int abs_arity { -arity };

                        do {
                            ++arg_idx;
                            if (argv[arg_idx][0] == '-') {
                                int parse_flag_outcome { parseFlag(argv, arg_idx ) };
                                if (parse_flag_outcome != PARSE_OK) {
                                    return parse_flag_outcome;
                                }
                            }else {
                                context_mgr[current_context_idx]->arguments.push_back(argv[arg_idx]);
                                --abs_arity;
                            }
                        } while ((arg_idx+1 < argc) && std::find(delimiters_.begin(), delimiters_.end(),
                                                              argv[arg_idx+1]) == delimiters_.end());

                        if (abs_arity > 0) {
                            auto expected_arity = -context_mgr[current_context_idx]->action->arity;
                            std::cout << "Expected at least " << expected_arity
                                      << " parameters for action " << action << ". Only read "
                                      << expected_arity - abs_arity
                                      << "." << std::endl;
                            return PARSE_ERROR;
                        }
                    }
                } else {
                    std::cout << "Unknown action: " << argv[arg_idx] << std::endl;
                    return PARSE_ERROR;
                }
            }
        }

        parsed_ = true;
        return PARSE_OK;
    }

    bool validateActionArguments() {
        if (!parsed_) {
            return false;
        }

        if (context_mgr.size() > 1) {
            for (auto & context : context_mgr) {
                if (context->action && context->action->arguments_callback) {
                    if (!context->action->arguments_callback(context->arguments)) {
                        return false;
                    }
                }
            }
        }

        return true;
    }

    // Dynamically output help information based on registered global and action
    // specific flags
    void help() {
        if (context_mgr[current_context_idx]->action) {
            actionHelp();
        } else {
            globalHelp();
        }
    }

    // Display the version information on stdout
    void version() {
        std::cout << version_string_;
    }

    bool whisper() {
        if (!parsed_) {
            return false;
        }

        current_context_idx = GLOBAL_CONTEXT_IDX - 1;
        bool previous_result = true;

        if (context_mgr.size() > 1) {
            for (size_t i = 0; i < context_mgr.size(); i++) {
                current_context_idx++;
                if (context_mgr[i]->action) {
                    if (!previous_result) {
                        std::cout << "Not starting action '" << context_mgr[i]->action->name
                                  << "'. Previous action failed to complete successfully." << std::endl;
                    } else {
                        // Record the current_context_idx. calling parse inside an action_callback
                        // allows the context list to grow during execution but has the side effect of
                        // mutating the current_context_index.
                        int tmp = current_context_idx;
                        // Flip it because success is 0
                        previous_result = !context_mgr[i]->action->action_callback(context_mgr[i]->arguments);
                        current_context_idx = tmp;
                        if (!context_mgr[i]->action->chainable) {
                            return !previous_result;
                        }
                    }
                }
           }
        } else {
            std::cout << "No action specified. See \"" << application_name_ << " --help\" for available actions." << std::endl;
        }

        return !previous_result;
    }

    template <typename Type>
    void defineGlobalFlag(std::string aliases, std::string description,
                          Type default_value, FlagCallback<Type> flag_callback){
        Flag<Type>* flagp = new Flag<Type>();
        flagp->aliases = aliases;
        flagp->value = default_value;
        flagp->description = description;
        flagp->flag_callback = flag_callback;
        // Aliases are space separated
        std::istringstream iss(aliases);
        while (iss) {
            std::string tmp;
            iss >> tmp;
            context_mgr[GLOBAL_CONTEXT_IDX]->flags[tmp] = flagp;
        }

        // vlevel is special and we don't want it showing up in the help list
        if (aliases != "vlevel") {
            registered_flags_["global"].push_back(flagp);
        }
    }

    template <typename Type>
    void defineActionFlag(std::string action_name, std::string aliases, std::string description,
                          Type default_value, FlagCallback<Type> flag_callback){
        Flag<Type>* flagp = new Flag<Type>();
        flagp->aliases = aliases;
        flagp->value = default_value;
        flagp->description = description;
        flagp->flag_callback = flag_callback;
        // Aliases are space separated
        std::istringstream iss(aliases);
        std::string tmp;
        while (iss >> tmp) {
            actions[action_name]->flags[tmp] = flagp;
        }
        registered_flags_[action_name].push_back(flagp);
    }

    void defineAction(std::string name, int arity, bool chainable, std::string description, std::string help_string,
                      ActionCallback action_callback, ArgumentsCallback arguments_callback) {
        Action* actionp = new Action();
        actionp->name = name;
        actionp->arity = arity;
        actionp->description = description;
        actionp->help_string_ = help_string;
        actionp->action_callback = action_callback;
        actionp->arguments_callback = arguments_callback;
        actionp->chainable = chainable;
        actions[name] = actionp;
    }

    template <typename Type>
    Type getFlagValue(std::string name) throw (undefined_flag_error) {
        int context_idx = getContextIdxIfDefined(name);
        if (context_idx != NO_CONTEXT_IDX) {
            return static_cast<Flag<Type>*>(context_mgr[context_idx]->flags[name])->value;
        }

        throw undefined_flag_error { "undefined flag: " + name };
    };

    FlagType getFlagType(const std::string& flag_name) {
        int context_idx = getContextIdxIfDefined(flag_name);

        if (context_idx == NO_CONTEXT_IDX) {
            throw undefined_flag_error { "undefined flag: " + flag_name };
        }

        return getFlagType(context_mgr[context_idx]->flags[flag_name]);
    }

    // ALSO check both contexts
    template <typename Type>
    void setFlag(std::string name, Type value) throw (undefined_flag_error,
                                                      flag_validation_error) {
        int context_idx = getContextIdxIfDefined(name);
        if (context_idx != NO_CONTEXT_IDX) {
            Flag<Type>* flagp = static_cast<Flag<Type>*>(context_mgr[context_idx]->flags[name]);
            Type tmp_value = flagp->value;
            flagp->value = value;

            // If there is a validation callback, do it
            if (flagp->flag_callback && !flagp->flag_callback(value)) {
                flagp->value = tmp_value;
                throw flag_validation_error { "callback for flag '" + name +
                                              "' returned false" };
            }
            return;
        }

        throw undefined_flag_error { "undefined flag: " + name };
    };

    std::vector<std::string> getParsedActions() {
        std::vector<std::string> action_container {};

        if (parsed_ && context_mgr.size() > 1) {
            for (size_t i = 0; i < context_mgr.size(); i++) {
                if (context_mgr[i]->action) {
                    action_container.push_back(context_mgr[i]->action->name);
                }
            }
        }

        return action_container;
    }

    void setHelpMargins(unsigned int left_margin, unsigned int right_margin) {description_margin_left = left_margin;
        description_margin_right = right_margin;
    }

  private:
    int current_context_idx;
    std::vector<ContextPtr> context_mgr;
    std::map<std::string, Action*> actions;
    std::map<std::string, std::vector<FlagBase*>> registered_flags_;
    bool parsed_ = false;
    std::vector<std::string> delimiters_;
    std::string application_name_ = "";
    std::string help_banner_ = "";
    std::string version_string_ = "";
    unsigned int description_margin_left = DESCRIPTION_MARGIN_LEFT_DEFAULT;
    unsigned int description_margin_right = DESCRIPTION_MARGIN_RIGHT_DEFAULT;

    int parseFlag(char* argv[], int& i) {
        // It's a flag. Get the array offset
        int offset = 1;
        if (argv[i][1] == '-') {
            ++offset;
        }
        std::string flagname(&argv[i][offset]);

        //  Deal with special vlevel flags
        if (flagname[0] == 'v') {
            size_t vlevel = 0;
            while (flagname[++vlevel] == 'v'); // keep counting the v's
            if (vlevel == flagname.size()) {
                setFlag<bool>("verbose", true);
                setFlag<int>("vlevel", vlevel);
                return PARSE_OK;
            }
        }

        // Deal with the special help flag
        if (flagname == "help" || flagname == "h") {
            return PARSE_HELP;
        }

        // Deal with the special --version flag
        if (flagname == "version") {
            return PARSE_VERSION;
        }

        if (!isFlagDefined(flagname)) {
            std::cout << "Unknown flag: " << flagname << std::endl;
            return PARSE_ERROR;
        }

        switch (getFlagType(flagname)) {
            case FlagType::Bool:
                setFlag<bool>(flagname, true);
                return PARSE_OK;
            case FlagType::String:
                if (argv[++i]) {
                    setFlag<std::string>(flagname, std::string(argv[i]));
                    return PARSE_OK;
                } else {
                    std::cout << "Missing value for flag: " << argv[i-1] << std::endl;
                    return PARSE_ERROR;
                }
            case FlagType::Int:
                if (argv[++i]) {
                    // Validate string looks like an interger
                    if (validateInteger(argv[i])) {
                        setFlag<int>(flagname, std::stol(argv[i], nullptr, 10));
                        return PARSE_OK;
                    } else {
                        std::cout << "Flag '" << flagname
                                  << "' expects a value of type integer" << std::endl;
                        return PARSE_INVALID_FLAG;
                    }
                } else {
                    std::cout << "Missing value for flag: " << argv[i-1] << std::endl;
                    return PARSE_ERROR;
                }
            case FlagType::Double:
                if (argv[++i]) {
                    if (validateDouble(argv[i])) {
                        setFlag<double>(flagname, std::stod(argv[i]));
                        return PARSE_OK;
                    } else {
                        std::cout << "Flag '" << flagname
                                  << "' expects a value of type double" << std::endl;
                        return PARSE_INVALID_FLAG;
                    }
                } else {
                    std::cout << "Missing value for flag: " << argv[i-1] << std::endl;
                    return PARSE_ERROR;
                }
        }

        std::cout << flagname << " is not of a valid flag type." << std::endl;
        return PARSE_ERROR;
    }

    // Display help information for the global context
    void globalHelp() {
        std::cout << help_banner_ << std::endl;
        std::cout << std::endl;

        std::cout << "Global options:";

        for (const auto& flag : registered_flags_["global"]) {
            writeFlagHelp(flag);
        }

        std::cout << "\n\nActions:\n";
        for (const auto& action : actions) {
            writeActionDescription(action.second);
        }

        std::cout << "\n";

        for (const auto& context : registered_flags_) {
            if (context.first != "global") {
                std::cout << context.first << " action options:";
                for (const auto& flag : context.second) {
                    writeFlagHelp(flag);
                }
                std::cout << "\n\n";
            }
        }
        std::cout << "For action specific help run \"" << application_name_
                  << " <action> --help\"" << std::endl;
    }

    // Display help information for the current action context
    void actionHelp() {
        if (context_mgr[current_context_idx]->action->help_string_.empty()) {
            std::cout << "No specific help found for action :"
                      << context_mgr[current_context_idx]->action->name
                      << "\n\n";
            return;
        }

        std::cout << context_mgr[current_context_idx]->action->help_string_;

        if (registered_flags_.find(context_mgr[current_context_idx]->action->name) != registered_flags_.end()) {
            std::cout << std::endl << "  " << context_mgr[current_context_idx]->action->name << " specific flags:" << std::endl;
            for (const auto& flag : registered_flags_[context_mgr[current_context_idx]->action->name]) {
                writeFlagHelp(flag);
            }
        }
        std::cout << std::endl << std::endl;
    }

    // Output the help information related to a single flag
    void writeFlagHelp(const FlagBase* flag) {
        std::stringstream aliases_stream { flag->aliases };
        std::stringstream output {};
        std::string alias {};
        std::string arg {};
        size_t last_alias_size { 0 };

        switch (getFlagType(flag)) {
            case FlagType::Bool:
                // No argument
                break;
            case FlagType::String:
                arg = " <str>";
                break;
            case FlagType::Int:
                arg = " <int>";
                break;
            case FlagType::Double:
                arg = " <float>";
        }

        while (aliases_stream >> alias) {
            if (alias != "") {
                output << "\n";
                output << std::setw(description_margin_left) << std::left;
                last_alias_size = alias.size() + arg.size();

                if (last_alias_size == 1) {
                    output << "   -" + alias + arg;
                } else if (last_alias_size > 1) {
                    output << "  --" + alias + arg;
                }
            }
        }

        auto newLine = [&output](unsigned int margin) {
            output << "\n" << std::setw(margin) << std::left;
            // Same length as above to fill the field in the same way
            output << "    ";
        };

        // New line condition: (2 or 3 spaces + dash prefix + alias
        // size + 2 spaces to separate from description) > margin
        if (last_alias_size + 6 > description_margin_left) {
            newLine(description_margin_left);
        }

        bool first_line { true };
        for (auto& line : wordWrap(flag->description, getDescriptionWidth())) {
            if (!first_line) {
                newLine(description_margin_left);
            }
            output << line;
            first_line = false;
        }

        std::cout << output.str();
    }

    // Output the action description related to a specific action
    void writeActionDescription(const Action* action) {
        std::stringstream action_stream;
        action_stream << "  " << action->name;

        std::cout << std::setw(description_margin_left) << std::left
                  << action_stream.str();

        // New line condition: (2 spaces + action name + 2 spaces to
        // separate from description) > margin
        if (action->name.size() + 4 > description_margin_left) {
            std::cout << "\n";
            std::cout << std::setw(description_margin_left) << std::left
                      << "    ";
        }

        std::cout << std::setw(description_margin_left) << std::left;

        bool first_line { true };
        for (auto& line : wordWrap(action->description, getDescriptionWidth())) {
            if (!first_line) {
                std::cout << std::setw(description_margin_left) << std::left
                          << "    "
                          << std::setw(description_margin_left) << std::left;
            }
            std::cout << line << "\n";
            first_line = false;
        }
    }

    bool isFlagDefined(std::string name) {
        if (getContextIdxIfDefined(name) != NO_CONTEXT_IDX) {
            return true;
        } else {
            return false;
        }
    }

    int getContextIdxIfDefined(std::string name) {
        if (context_mgr[current_context_idx]->flags.find(name) != context_mgr[current_context_idx]->flags.end()) {
            return current_context_idx;
        } else if (context_mgr[GLOBAL_CONTEXT_IDX]->flags.find(name) != context_mgr[GLOBAL_CONTEXT_IDX]->flags.end()) {
            return GLOBAL_CONTEXT_IDX;
        } else {
            return NO_CONTEXT_IDX;
        }
    }

    bool isActionDefined(std::string name) {
        return !(actions.find(name) == actions.end());
    }

    FlagType getFlagType(const FlagBase* flagp) {
        FlagType flag_type { FlagType::Bool };

        // RTTI to determine the flag value type
        if (dynamic_cast<const Flag<bool>*>(flagp)) {
            flag_type = FlagType::Bool;
        } else if (dynamic_cast<const Flag<std::string>*>(flagp)) {
            flag_type = FlagType::String;
        } else if (dynamic_cast<const Flag<int>*>(flagp)) {
            flag_type = FlagType::Int;
        } else if (dynamic_cast<const Flag<double>*>(flagp)) {
            flag_type = FlagType::Double;
        } else {
            // We only support the types in the FlagType enum...
            assert(false);
        }

        return flag_type;
    }

    unsigned int getDescriptionWidth() {
        return description_margin_right - description_margin_left;
    }
};

//
// API
//

template <typename Type>
static void DefineGlobalFlag(std::string aliases,
                                std::string description,
                                Type default_value,
                                FlagCallback<Type> flag_callback) {
    HorseWhisperer::Instance().defineGlobalFlag<Type>(aliases,
                                                      description,
                                                      default_value,
                                                      flag_callback);
}

template <typename Type>
static void DefineActionFlag(std::string action_name,
                                std::string aliases,
                                std::string description,
                                Type default_value,
                                FlagCallback<Type> flag_callback) {
    HorseWhisperer::Instance().defineActionFlag<Type>(action_name,
                                                      aliases,
                                                      description,
                                                      default_value,
                                                      flag_callback);
}

template <typename Type>
static Type GetFlag(std::string flag_name) {
    return HorseWhisperer::Instance().getFlagValue<Type>(flag_name);
}

static FlagType GetFlagType(std::string flag_name) {
    return HorseWhisperer::Instance().getFlagType(flag_name);
}

template <typename Type>
static void SetFlag(std::string flag_name, Type value) {
    HorseWhisperer::Instance().setFlag<Type>(flag_name, value);
}

static void DefineAction(std::string action_name,
                           int arity,
                           bool chainable,
                           std::string description,
                           std::string help_string,
                           ActionCallback action_callback,
                           ArgumentsCallback arguments_callback = nullptr) {
    HorseWhisperer::Instance().defineAction(action_name,
                                            arity,
                                            chainable,
                                            description,
                                            help_string,
                                            action_callback,
                                            arguments_callback);
}

static bool IsActionFlag(std::string action, std::string flagname) {
    return HorseWhisperer::Instance().isActionFlag(action, flagname);
}

static void SetAppName(std::string name) {
    HorseWhisperer::Instance().setAppName(name);
}

static void SetHelpBanner(std::string banner) {
    HorseWhisperer::Instance().setHelpBanner(banner);
}

static void SetVersion(std::string version_string) {
    HorseWhisperer::Instance().setVersionString(version_string);
}

static void SetDelimiters(std::vector<std::string> delimiters) {
    HorseWhisperer::Instance().setDelimiters(delimiters);
}

// Return 1 if parse didn't succeed.
static int Parse(int argc, char** argv) {
    return HorseWhisperer::Instance().parse(argc, argv);
}

// Return false if parse didn't succeed.
static bool ValidateActionArguments() {
    return HorseWhisperer::Instance().validateActionArguments();
}

static void ShowHelp() {
    HorseWhisperer::Instance().help();
}

static void ShowVersion() {
    HorseWhisperer::Instance().version();
}

static std::vector<std::string> GetParsedActions() {
    return HorseWhisperer::Instance().getParsedActions();
}

static int Start() {
    return HorseWhisperer::Instance().whisper();
}

static void Reset() {
    HorseWhisperer::Instance().reset();
}

static void SetHelpMargins(unsigned int left_margin, unsigned int right_margin) {
    HorseWhisperer::Instance().setHelpMargins(left_margin, right_margin);
}

}  // namespace HorseWhisperer

#endif  // HORSEWHISPERER_INCLUDE_HORSE_WHISPERER_H_
