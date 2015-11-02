#ifndef INCLUDE_HORSEWHISPERER_HORSEWHISPERER_H_
#define INCLUDE_HORSEWHISPERER_HORSEWHISPERER_H_

#include <string>
#include <map>
#include <vector>
#include <functional>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <cctype>
#include <algorithm>
#include <memory>
#include <stdexcept>
#include <cassert>

// Used by consumers to export Horsewhisperer configuration from a shared library.
#ifndef HORSEWHISPERER_EXPORT
#define HORSEWHISPERER_EXPORT
#endif

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

class action_validation_error : public horsewhisperer_error {
  public:
    explicit action_validation_error(std::string const& msg) :
            horsewhisperer_error(msg) {}
};

//
// Tokens
//

static const std::string VERSION_STRING = "0.12.0";

// Context indexes
static const int GLOBAL_CONTEXT_IDX = 0;
static const int NO_CONTEXT_IDX = -1;

// Parse results
enum class ParseResult { OK, HELP, VERSION, FAILURE, INVALID_FLAG };

// Margins for help descriptions
static const unsigned int DESCRIPTION_MARGIN_LEFT_DEFAULT = 30;
static const unsigned int DESCRIPTION_MARGIN_RIGHT_DEFAULT = 80;

//
// Types
//

enum FlagType { Bool, Int, Double, String };

// Callback specified for a given value; called whenever the setFlag()
// function is executed in order to validate the flag argument - it
// will be called when parsing.
// The callback should throw an error in case an invalid value is
// assigned to the flag; in case the error is a flag_validation_error,
// it will be propagated.
template <typename Type>
using FlagCallback = std::function<void(Type&)>;

using Arguments = std::vector<std::string>;

// Callback specified for a given action; called by the parse()
// function after completing the parsing, in order to validate its
// arguments.
// The callback should throw an error in case an invalid value is
// assigned to the flag; in case the error is an
// action_validation_error, it will be propagated.
using ArgumentsCallback = std::function<void(const Arguments& arguments)>;

using ActionCallback = std::function<int(const Arguments& arguments)>;

struct FlagBase {
    virtual ~FlagBase() {}
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
    // Arity of the action, or min arity in case variable_arity is flagged
    unsigned int arity;
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
    // Wheter we invoke the action with variable num of args
    bool variable_arity;
};

static FlagType getTypeOfFlag(const FlagBase* flagp);

struct Context {
    // Flags defined for the given context
    std::map<std::string, FlagBase*> flags;
    // What this context is doing
    Action* action;
    // Action arguments
    Arguments arguments;

    std::string toString() {
        std::stringstream ss {};
        ss << "Action " << action->name;
        if (arguments.size() > 0) {
            ss << "  - arguments:";
            for (auto& arg : arguments) {
                ss << " " << arg;
            }
        }
        if (flags.size() > 0) {
            for (auto& k_v : flags) {
                ss << "\n  flag " << k_v.first << ": ";
                switch (getTypeOfFlag(k_v.second)) {
                    case FlagType::Bool:
                        ss << static_cast<Flag<bool>*>(k_v.second)->value;
                        break;
                    case FlagType::String:
                        ss << static_cast<Flag<std::string>*>(k_v.second)->value;
                        break;
                    case FlagType::Int:
                        ss << static_cast<Flag<int>*>(k_v.second)->value;
                        break;
                    case FlagType::Double:
                        ss << static_cast<Flag<double>*>(k_v.second)->value;
                }
            }
        }
        return ss.str();
    }
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
                         ArgumentsCallback arguments_callback,
                         bool variable_arity) __attribute__ ((unused));
static void SetAppName(std::string name) __attribute__ ((unused));
static void SetHelpBanner(std::string banner) __attribute__ ((unused));
static void SetVersion(std::string version, std::string short_flag) __attribute__ ((unused));
static void SetDelimiters(std::vector<std::string> delimiters) __attribute__ ((unused));
static ParseResult Parse(int argc, char** argv) __attribute__ ((unused));
static void ShowHelp(bool show_actions_help = true) __attribute__ ((unused));
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
    for (size_t i = (val[0] == '-' ? 1 : 0); i < val.size(); i++) {
        if ((val[i] < '0') || (val[i] > '9')) {
            return false;
        }
    }
    return true;
}

static bool validateDouble(const std::string& val) {
    std::istringstream i_s { (val[0] == '-' ? val.substr(1) : val) };
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

static FlagType getTypeOfFlag(const FlagBase* flagp) {
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

//
// HorseWhisperer
//

class HORSEWHISPERER_EXPORT HorseWhisperer {
  public:
    // Return reference to instance of HorseWhisperer singleton
    static HorseWhisperer& Instance() {
        static HorseWhisperer instance;
        return instance;
    }

    // No args constructor creates the pointer to the root context and
    // implicitly declares the --help flag.
    // Initializations are performed by init().
    HorseWhisperer() {
        init();
    }

    void reset() {
        clean();
        init();
    }

    void setAppName(std::string name) {
        application_name_ = name;
    }

    void setHelpBanner(std::string banner) {
        help_banner_ = banner;
    }

    void setVersionString(std::string version_string, std::string short_flag_string) {
        version_string_ = version_string;
        version_short_flag_string_ = short_flag_string;
        std::string flag_string = "version";
        if (short_flag_string != "") {
            flag_string = short_flag_string + " " + flag_string;
        }
        defineGlobalFlag<bool>(flag_string, "Display version information and quit",
                               false, nullptr);
    }

    void setDelimiters(const std::vector<std::string>& delimiters) {
        delimiters_ = delimiters;
    }

    bool isDelimiter(const char* argument) const {
        if (std::find(delimiters_.begin(), delimiters_.end(), argument)
                != delimiters_.end()) {
            return true;
        }
        return false;
    }

    bool isActionFlag(const std::string& action_name, const std::string& flagname) {
        for (const auto& flag : actions_[action_name]->flags) {
            if (flagname.compare(flag.first) == 0) {
                return true;
            }
        }
        return false;
    }

    void setContextFlags(ContextPtr& action_context, const std::string& action_name) {
        // Copy the specific action flags, so that, in case this
        // action has been chained multiple times, each context
        // will have a different flag instace, thus allowing to
        // parse and store different flag values - example:
        // `app_name action_1 --flag_a foo + action_1 --flag_a bar`
        for (auto& k_v : actions_[action_name]->flags) {
            // However, if we have already been set as an alias in
            // setActionFlags we don't have to do it again.
            if (action_context->flags[k_v.first]) {
                continue;
            }
            switch (getTypeOfFlag(k_v.second)) {
                case FlagType::Bool: {
                    action_context->flags[k_v.first] = new Flag<bool>(
                        *(static_cast<Flag<bool>*>(k_v.second)));
                    break;
                }
                case FlagType::String: {
                    action_context->flags[k_v.first] = new Flag<std::string>(
                        *(static_cast<Flag<std::string>*>(k_v.second)));
                    break;
                }
                case FlagType::Int: {
                    action_context->flags[k_v.first] = new Flag<int>(
                        *(static_cast<Flag<int>*>(k_v.second)));
                    break;
                }
                case FlagType::Double: {
                    action_context->flags[k_v.first] = new Flag<double>(
                        *(static_cast<Flag<double>*>(k_v.second)));
                    break;
                }
            }

            std::istringstream iss { k_v.second->aliases };
            std::string tmp;
            while (iss >> tmp) {
                action_context->flags[tmp] = action_context->flags[k_v.first];
            }
        }
    }

    ParseResult parse(int argc, char* argv[]) {
        for (int arg_idx = 1; arg_idx < argc; arg_idx++) {
            // Identify if it's a flag
            if (argv[arg_idx][0] == '-') {
                auto parse_flag_outcome = parseFlag(argv, arg_idx);

                if (parse_flag_outcome != ParseResult::OK) {
                    return parse_flag_outcome;
                }
            } else if (isDelimiter(argv[arg_idx])) {  // skip over delimiter
                continue;
            } else {
                std::string action = argv[arg_idx];

                if (isActionDefined(action)) {
                    ContextPtr action_context { new Context() };
                    action_context->flags = std::map<std::string, FlagBase*> {};
                    setContextFlags(action_context, action);
                    action_context->action = actions_[argv[arg_idx]];
                    action_context->arguments = Arguments {};
                    context_mgr_.push_back(std::move(action_context));
                    current_context_idx_++;

                    assert(static_cast<unsigned int>(current_context_idx_)
                           == context_mgr_.size() - 1);

                    // parse arguments and action flags
                    auto arity =
                        static_cast<int>(context_mgr_[current_context_idx_]->action->arity);

                    if (!context_mgr_[current_context_idx_]->action->variable_arity) {
                        // Read as many parameters as the current arity value
                        while (arity > 0) {
                            ++arg_idx;
                            if (arg_idx >= argc) {  // have we run out of tokens?
                                break;
                            } else if (argv[arg_idx][0] == '-') {  // is it a flag token?
                                auto parse_flag_outcome = parseFlag(argv, arg_idx);
                                if (parse_flag_outcome != ParseResult::OK) {
                                    return parse_flag_outcome;
                                }
                            } else if (isActionDefined(argv[arg_idx])) {  // is it an action?
                                std::cout << "Expected parameter for action: " << action
                                          << ". Found action: " << argv[arg_idx] << std::endl;
                                return ParseResult::FAILURE;
                            } else if (isDelimiter(argv[arg_idx])) {  // is it a delimiter?
                                std::cout << "Expected parameter for action: " << action
                                          << ". Found delimiter: " << argv[arg_idx] << std::endl;
                                return ParseResult::FAILURE;
                            } else {
                                context_mgr_[current_context_idx_]->arguments
                                    .push_back(argv[arg_idx]);
                                arity--;
                            }
                        }

                        if (arity > 0) {
                            std::cout << "Expected "
                                      << context_mgr_[current_context_idx_]->action->arity
                                      << " parameters for action " << action << ". Only read "
                                      << context_mgr_[current_context_idx_]->action->arity - arity
                                      << "." << std::endl;
                            return ParseResult::FAILURE;
                        }
                    } else {
                        // When arity is an "at least" representation we eat arguments
                        // until we either run, we hit a delimiter, or we find a known
                        // action
                        do {
                            ++arg_idx;

                            if (arg_idx >= argc) {
                                // No more tokens
                                break;
                            } else if (argv[arg_idx][0] == '-') {
                                auto parse_flag_outcome = parseFlag(argv, arg_idx);
                                if (parse_flag_outcome != ParseResult::OK) {
                                    return parse_flag_outcome;
                                }
                            } else {
                                context_mgr_[current_context_idx_]->arguments
                                    .push_back(argv[arg_idx]);
                                --arity;
                            }
                        } while ((arg_idx+1 < argc)
                                  && !isDelimiter(argv[arg_idx+1])
                                  && !isActionDefined(argv[arg_idx+1]));

                        if (arity > 0) {
                            std::cout << "Expected at least "
                                      << context_mgr_[current_context_idx_]->action->arity
                                      << " parameters for action " << action << ". Only read "
                                      << context_mgr_[current_context_idx_]->action->arity - arity
                                      << "." << std::endl;
                            return ParseResult::FAILURE;
                        }
                    }
                } else {
                    std::cout << "Unknown action: " << argv[arg_idx] << std::endl;
                    return ParseResult::FAILURE;
                }
            }
        }

        validateActionArguments();

        parsed_ = true;
        return ParseResult::OK;
    }

    void validateActionArguments() {
        if (context_mgr_.size() > 1) {
            for (auto & context : context_mgr_) {
                if (context->action && context->action->arguments_callback) {
                    try {
                        context->action->arguments_callback(context->arguments);
                    } catch (action_validation_error) {
                        throw;
                    } catch (std::exception& e) {
                        throw action_validation_error { "failed to validate "
                                                        + context->action->name
                                                        + " argument - " + e.what() };
                    }
                }
            }
        }
    }

    // Dynamically output help information based on registered global and action
    // specific flags
    void help(bool show_actions_help) {
        if (context_mgr_[current_context_idx_]->action) {
            actionHelp();
        } else {
            globalHelp(show_actions_help);
        }
    }

    // Display the version information on stdout
    void version() {
        std::cout << version_string_;
    }

    int whisper() {
        if (!parsed_) {
            return EXIT_FAILURE;
        }

        current_context_idx_ = GLOBAL_CONTEXT_IDX - 1;
        int previous_exit_code = EXIT_SUCCESS;

        if (context_mgr_.size() > 1) {
            for (size_t i = 0; i < context_mgr_.size(); i++) {
                current_context_idx_++;
                if (context_mgr_[i]->action) {
                    auto& current_action = context_mgr_[i]->action;
                    if (previous_exit_code != EXIT_SUCCESS) {
                        std::cout << "Not starting action '"
                                  << current_action->name
                                  << "'. Previous action failed to complete "
                                  << "successfully." << std::endl;
                    } else if (!current_action->action_callback) {
                        std::cout << "No calback has been defined for action '"
                                  << current_action->name << "'." << std::endl;
                        previous_exit_code = EXIT_FAILURE;
                    } else {
                        // Record the current_context_idx_. Calling parse inside
                        // an action_callback allows the context list to grow
                        // during execution but has the side effect of mutating
                        // the current_context_index.
                        int tmp = current_context_idx_;

                        previous_exit_code = current_action->action_callback(
                                                context_mgr_[i]->arguments);
                        current_context_idx_ = tmp;
                    }

                    if (!current_action->chainable) {
                        if (i < context_mgr_.size() - 1
                            && context_mgr_[i+1]->action) {
                            std::cout << "Skipping the following actions; '"
                                      << current_action->name
                                      << "' is not chainable." << std::endl;
                        }
                        break;
                    }
                }
           }
        } else {
            std::cout << "No action specified. See \"" << application_name_
                      << " --help\" for available actions." << std::endl;
        }

        return previous_exit_code;
    }

    template <typename Type>
    void defineGlobalFlag(std::string aliases, std::string description,
                          Type default_value, FlagCallback<Type> flag_callback) {
        Flag<Type>* flagp = new Flag<Type>();
        flagp->aliases = aliases;
        flagp->value = default_value;
        flagp->description = description;
        flagp->flag_callback = flag_callback;
        // Aliases are space separated
        std::istringstream iss { aliases };
        while (iss) {
            std::string tmp;
            iss >> tmp;
            context_mgr_[GLOBAL_CONTEXT_IDX]->flags[tmp] = flagp;
        }

        // vlevel is special and we don't want it showing up in the help list
        if (aliases != "vlevel") {
            registered_flags_["global"].push_back(flagp);
        }
    }

    template <typename Type>
    void defineActionFlag(std::string action_name, std::string aliases, std::string description,
                          Type default_value, FlagCallback<Type> flag_callback) {
        Flag<Type>* flagp = new Flag<Type>();
        flagp->aliases = aliases;
        flagp->value = default_value;
        flagp->description = description;
        flagp->flag_callback = flag_callback;
        // Aliases are space separated
        std::istringstream iss { aliases };
        std::string tmp;
        while (iss >> tmp) {
            actions_[action_name]->flags[tmp] = flagp;
        }
        registered_flags_[action_name].push_back(flagp);
    }

    void defineAction(std::string name, int arity, bool chainable,
                      std::string description, std::string help_string,
                      ActionCallback action_callback,
                      ArgumentsCallback arguments_callback,
                      bool variable_arity) {
        Action* actionp = new Action();
        actionp->name = name;
        actionp->arity = arity;
        actionp->description = description;
        actionp->help_string_ = help_string;
        actionp->action_callback = action_callback;
        actionp->arguments_callback = arguments_callback;
        actionp->chainable = chainable;
        actionp->variable_arity = variable_arity;
        actions_[name] = actionp;
    }

    template <typename Type>
    Type getFlagValue(std::string name) throw (undefined_flag_error) {
        int context_idx = getContextIdxIfDefined(name);
        if (context_idx != NO_CONTEXT_IDX) {
            return static_cast<Flag<Type>*>(context_mgr_[context_idx]->flags[name])->value;
        }

        throw undefined_flag_error { "undefined flag: " + name };
    }

    FlagType checkAndGetTypeOfFlag(const std::string& flag_name) {
        int context_idx = getContextIdxIfDefined(flag_name);

        if (context_idx == NO_CONTEXT_IDX) {
            throw undefined_flag_error { "undefined flag: " + flag_name };
        }

        return getTypeOfFlag(context_mgr_[context_idx]->flags[flag_name]);
    }

    // ALSO check both contexts
    template <typename Type>
    void setFlag(std::string name, Type value) throw (undefined_flag_error,
                                                      flag_validation_error) {
        int context_idx = getContextIdxIfDefined(name);
        if (context_idx != NO_CONTEXT_IDX) {
            Flag<Type>* flagp = static_cast<Flag<Type>*>(
                                    context_mgr_[context_idx]->flags[name]);

            if (flagp->flag_callback) {
                try {
                    flagp->flag_callback(value);
                } catch (flag_validation_error) {
                    throw;
                } catch (std::exception& e) {
                    throw flag_validation_error { "failed to validate '" + name
                                                  + "' flag: " + e.what() };
                }
            }

            flagp->value = value;
            return;
        }

        throw undefined_flag_error { "undefined flag: " + name };
    }

    std::vector<std::string> getParsedActions() {
        std::vector<std::string> action_container {};

        if (parsed_ && context_mgr_.size() > 1) {
            for (size_t i = 0; i < context_mgr_.size(); i++) {
                if (context_mgr_[i]->action) {
                    action_container.push_back(context_mgr_[i]->action->name);
                }
            }
        }

        return action_container;
    }

    void setHelpMargins(unsigned int left_margin, unsigned int right_margin) {
        description_margin_left_ = left_margin;
        description_margin_right_ = right_margin;
    }

    // Debug method
    void printState() {
        std::stringstream ss {};
        ss << "Current context index = "
           << std::to_string(current_context_idx_);
        if (context_mgr_.size() > 1) {
            for (size_t idx = 1; idx < context_mgr_.size(); idx++) {
                ss << "\n" << context_mgr_[idx]->toString();
            }
        }
        std::cout << ss.str() << "\n";
    }

  private:
    // Index of the context currently being processed
    int current_context_idx_;

    // Container of contexts
    std::vector<ContextPtr> context_mgr_;

    // Registered flags
    std::map<std::string, Action*> actions_;

    // Maps contexts (global and single actions) to registered flags
    std::map<std::string, std::vector<FlagBase*>> registered_flags_;

    // Whether CL args have been parsed
    bool parsed_;

    // Action delimeters
    std::vector<std::string> delimiters_;

    // Application name
    std::string application_name_;

    // Text header of the help message
    std::string help_banner_;

    // Version information
    std::string version_string_;
    std::string version_short_flag_string_;

    // Margins, indicated as number of columns
    unsigned int description_margin_left_;
    unsigned int description_margin_right_;

    void clean() {
        context_mgr_.clear();
        actions_.clear();
        registered_flags_.clear();
        delimiters_.clear();
    }

    void init() {
        current_context_idx_ = GLOBAL_CONTEXT_IDX;

        ContextPtr global_context { new Context() };
        global_context->action = nullptr;
        context_mgr_.push_back(std::move(global_context));

        parsed_ = false;
        application_name_ = "";
        help_banner_ = "";
        version_string_ = "";
        version_short_flag_string_ = "";
        description_margin_left_ = DESCRIPTION_MARGIN_LEFT_DEFAULT;
        description_margin_right_ = DESCRIPTION_MARGIN_RIGHT_DEFAULT;

        defineGlobalFlag<bool>("h help", "Show this message", false, nullptr);
        defineGlobalFlag<int>("vlevel", "", 0, nullptr);
        defineGlobalFlag<bool>("verbose", "Set verbose output", false,
                               [this] (bool val) { setFlag<int>("vlevel", 1); });
    }

    ParseResult parseFlag(char* argv[], int& i) {
        // It's a flag. Get the array offset
        int offset = 1;
        if (argv[i][1] == '-') {
            ++offset;
        }
        std::string flagname { &argv[i][offset] };

        // check if flag looks like key=value
        size_t k_v { flagname.find("=") };

        if (k_v != std::string::npos) {
            flagname = flagname.substr(0, k_v);
            // += offset to take - or -- into account
            // increment to get the first char after '='
            ++k_v += offset;
        }

        // Deal with special vlevel flags
        if (flagname[0] == 'v') {
            size_t vlevel = 0;

            while (flagname[++vlevel] == 'v') {
                // keep counting the v's
            }

            if (vlevel == flagname.size()) {
                setFlag<bool>("verbose", true);
                setFlag<int>("vlevel", vlevel);
                return ParseResult::OK;
            }
        }

        // Deal with the special help flag
        if (flagname == "help" || flagname == "h") {
            return ParseResult::HELP;
        }

        // Deal with the special --version flag
        if (flagname == "version" || flagname == version_short_flag_string_) {
            return ParseResult::VERSION;
        }

        if (!isFlagDefined(flagname)) {
            std::cout << "Unknown flag: " << flagname << std::endl;
            return ParseResult::FAILURE;
        }

        FlagType flag_type = checkAndGetTypeOfFlag(flagname);

        std::string value {};

        if (k_v != std::string::npos) {
            value = &argv[i][k_v];
        } else if (flag_type != FlagType::Bool && argv[++i]) {
            // bool shouldn't try and take an argument from argv
            value = argv[i];
        }

        return setAndValidateFlag(flag_type, flagname, value);
    }

    ParseResult setAndValidateFlag(FlagType flag_type, std::string flagname,
                                   std::string value) {
        if (flag_type == FlagType::Bool) {
            bool b_val { true };

            if (!value.empty()) {
                // passed as --true_thing=false|true
                if (value == "false") {
                    b_val = false;
                } else if (value != "true") {
                    std::cout << "Flag '" << flagname
                              << "' expects a value of 'true' or 'false'"
                              << std::endl;
                    return ParseResult::FAILURE;
                }
            }
            setFlag<bool>(flagname, b_val);
            return ParseResult::OK;
        } else {
            if (value.empty()) {
                std::cout << "Missing value for flag: " << flagname << std::endl;
                return ParseResult::FAILURE;
            }

            if (flag_type == FlagType::String) {
                setFlag<std::string>(flagname, std::string(value));
                return ParseResult::OK;
            } else if (flag_type == FlagType::Int) {
                if (validateInteger(value)) {
                    setFlag<int>(flagname, std::stol(value, nullptr, 10));
                    return ParseResult::OK;
                } else {
                    std::cout << "Flag '" << flagname
                                << "' expects a value of type integer" << std::endl;
                    return ParseResult::INVALID_FLAG;
                }
            } else if (flag_type == FlagType::Double) {
                if (validateDouble(value)) {
                    setFlag<double>(flagname, std::stod(value));
                    return ParseResult::OK;
                } else {
                    std::cout << "Flag '" << flagname
                              << "' expects a value of type double" << std::endl;
                    return ParseResult::INVALID_FLAG;
                }
            }
        }

        std::cout << flagname << " is not of a valid flag type." << std::endl;
        return ParseResult::FAILURE;
    }

    // Display help information for the global context
    void globalHelp(bool show_actions_help) {
        std::cout << help_banner_ << std::endl;
        std::cout << std::endl;

        if (show_actions_help) {
            std::cout << "Global options:";
        } else {
            std::cout << "Options:";
        }

        for (const auto& flag : registered_flags_["global"]) {
            writeFlagHelp(flag);
        }

        if (show_actions_help) {
            std::cout << "\n\nActions:\n";
            for (const auto& action : actions_) {
                writeActionDescription(action.second);
            }

            std::cout << "\nFor action specific help run \"" << application_name_
                      << " <action> --help\"";
        }

        std::cout << std::endl << std::endl;
    }

    // Display help information for the current action context
    void actionHelp() {
        if (context_mgr_[current_context_idx_]->action->help_string_.empty()) {
            std::cout << "No specific help found for action :"
                      << context_mgr_[current_context_idx_]->action->name
                      << "\n\n";
            return;
        }

        std::cout << context_mgr_[current_context_idx_]->action->help_string_;

        if (registered_flags_.find(context_mgr_[current_context_idx_]->action->name)
                != registered_flags_.end()) {
            std::cout << "\n  " << context_mgr_[current_context_idx_]->action->name
                      << " specific flags:\n";
            for (const auto& f : registered_flags_[
                                    context_mgr_[current_context_idx_]->action->name]) {
                writeFlagHelp(f);
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

        switch (getTypeOfFlag(flag)) {
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
                output << std::setw(description_margin_left_) << std::left;
                last_alias_size = alias.size();

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
        if (last_alias_size + 6 > description_margin_left_) {
            newLine(description_margin_left_);
        }

        bool first_line { true };
        for (auto& line : wordWrap(flag->description, getDescriptionWidth())) {
            if (!first_line) {
                newLine(description_margin_left_);
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

        std::cout << std::setw(description_margin_left_) << std::left
                  << action_stream.str();

        // New line condition: (2 spaces + action name + 2 spaces to
        // separate from description) > margin
        if (action->name.size() + 4 > description_margin_left_) {
            std::cout << "\n";
            std::cout << std::setw(description_margin_left_) << std::left
                      << "    ";
        }

        std::cout << std::setw(description_margin_left_) << std::left;

        bool first_line { true };
        for (auto& line : wordWrap(action->description, getDescriptionWidth())) {
            if (!first_line) {
                std::cout << std::setw(description_margin_left_) << std::left
                          << "    "
                          << std::setw(description_margin_left_) << std::left;
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
        if (context_mgr_[current_context_idx_]->flags.find(name)
                != context_mgr_[current_context_idx_]->flags.end()) {
            return current_context_idx_;
        } else if (context_mgr_[GLOBAL_CONTEXT_IDX]->flags.find(name)
                != context_mgr_[GLOBAL_CONTEXT_IDX]->flags.end()) {
            return GLOBAL_CONTEXT_IDX;
        } else {
            return NO_CONTEXT_IDX;
        }
    }

    bool isActionDefined(const std::string& name) {
        return !(actions_.find(name) == actions_.end());
    }

    unsigned int getDescriptionWidth() {
        return description_margin_right_ - description_margin_left_;
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
    return HorseWhisperer::Instance().checkAndGetTypeOfFlag(flag_name);
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
                           ArgumentsCallback arguments_callback = nullptr,
                           bool variable_arity = false) {
    HorseWhisperer::Instance().defineAction(action_name,
                                            arity,
                                            chainable,
                                            description,
                                            help_string,
                                            action_callback,
                                            arguments_callback,
                                            variable_arity);
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

static void SetVersion(std::string version_string, std::string short_flag_string = "") {
    HorseWhisperer::Instance().setVersionString(version_string, short_flag_string);
}

static void SetDelimiters(std::vector<std::string> delimiters) {
    HorseWhisperer::Instance().setDelimiters(delimiters);
}

// Return the parsing outcome as a ParseResult enum value.
// Throw an action_validation_error in case any action validation
// callback invalidates or fails to validate an argument.
// Throw a flag_validation_error in case any flag validation
// callback invalidates or fails to validate a flag value.
static ParseResult Parse(int argc, char** argv) {
    return HorseWhisperer::Instance().parse(argc, argv);
}

static void ShowHelp(bool show_actions_help) {
    HorseWhisperer::Instance().help(show_actions_help);
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

#endif  // INCLUDE_HORSEWHISPERER_HORSEWHISPERER_H_
