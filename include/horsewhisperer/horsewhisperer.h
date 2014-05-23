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

namespace HorseWhisperer {

static const std::string VERSION_STRING = "0.1.0";

using ActionCallback = std::function<int(std::vector<std::string> args)>;

template <typename FlagType>
using ValidationCallback = std::function<bool(FlagType)>;


template <typename FlagType>
static void DefineGlobalFlag(std::string aliases,
                             std::string description,
                             FlagType default_value,
                             ValidationCallback<FlagType>validation_callback) __attribute__ ((unused));
template <typename FlagType>
static void DefineActionFlag(std::string action_name,
                             std::string aliases,
                             std::string description,
                             FlagType default_value,
                             ValidationCallback<FlagType> validation_callback) __attribute__ ((unused));
template <typename FlagType>
static FlagType GetFlag(std::string flag_name) __attribute__ ((unused));
template <typename FlagType>
static bool SetFlag(std::string flag_name, FlagType value) __attribute__ ((unused));
static void DefineAction(std::string action_name, int arity, bool chainable, std::string description, std::string help_string,
                           ActionCallback action_callback) __attribute__ ((unused));
static void SetAppName(std::string name) __attribute__ ((unused));
static void SetHelpBanner(std::string banner) __attribute__ ((unused));
static void SetVersion(std::string version) __attribute__ ((unused));
static void SetDelimiters(std::vector<std::string> delimiters) __attribute__ ((unused));
static bool Parse(int argc, char** argv) __attribute__ ((unused));
static int Start() __attribute__ ((unused));

struct FlagBase {
    virtual ~FlagBase() {};
    std::string aliases;
    std::string description;
};

template <typename FlagType>
struct Flag : FlagBase {
    FlagType value;
    ValidationCallback<FlagType> validation_callback;
};

struct Action {
    ~Action() {
        for (auto& flag : flags) {
            delete flag.second;
        }
    }
    std::string name;
    std::map<std::string, FlagBase*> flags; // Keys local to the action
    std::string description;
    int arity; // Arity of the action
    // Function called when we invoke the action
    ActionCallback action_callback;
    // Context sensitive action help
    std::string help_string_;
    bool success;
    bool chainable;
};

struct Context {
    ~Context() {
        for (auto& flag : flags) {
            if (flag.second) {
                delete flag.second;
            }
        }
    }
    std::map<std::string, FlagBase*> flags; // Flags defined for the given context
    Context *next; // Pointer to the next context structure
    Action* action; // What this context is doing
    std::vector<std::string> arguments;
};

// Because regex is busted on a lot of versions of libstdc++ I'm rolling
// my own integer validation.
static bool validateInteger(std::string val) {
    for (size_t i = 0; i < val.size(); i++) {
        if ((val[i] < '0') || (val[i] > '9')) {
            return false;
        }
    }
    return true;
}

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
        global_context_ = new Context();
        global_context_->next = nullptr;
        global_context_->action = nullptr;
        current_context_ = global_context_;
        defineGlobalFlag<bool>("h help", "Shows this message.", false, nullptr);
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

    bool parse(int argc, char* argv[]) {
        for (int i = 1; i < argc; i++) {
            // Identify if it's a flag
            if (argv[i][0] == '-') {
                if (!parseFlag(argv, i)) {
                    return false;
                }
                if (abort_parse_) {
                    return true;
                }
            } else if (std::find(delimiters_.begin(), delimiters_.end(), argv[i]) != delimiters_.end()) { // skip over delimiter
                continue;
            } else {
                std::string action = argv[i];
                if (isActionDefined(action)) {
                    Context* action_context = new Context();
                    current_context_->next = action_context;
                    action_context->next = nullptr;
                    action_context->flags = actions[argv[i]]->flags;
                    action_context->action = actions[argv[i]];
                    current_context_ = action_context;

                    int arity = action_context->action->arity;
                    // parse parameters
                    if (arity > 0) { // iff read parameters = arity
                        for (; arity > 0; arity--) {
                            ++i;
                            if (argv[i] == nullptr) { // have we run out of tokens?
                                break;
                            } else if (argv[i][0] == '-') { // is it a flag token?
                                if (!parseFlag(argv, i)) {
                                    return false;
                                }
                                if (abort_parse_) {
                                    // When a special event like a --version or --help flag
                                    // are found, we terminate the parse process.
                                    return true;
                                }
                            } else if (isActionDefined(argv[i])) { // is it an action?
                                std::cout << "Expected parameter for action: " << action
                                          << ". Found action: " << argv[i] << std::endl;
                                return false;
                            } else if (std::find(delimiters_.begin(), delimiters_.end(), argv[i]) != delimiters_.end()) { // is it a delimiter?
                                std::cout << "Expected parameter for action: " << action
                                          << ". Found delimiter: " << argv[i] << std::endl;
                                return false;
                            } else {
                                 action_context->arguments.push_back(argv[i]);
                            }
                        }
                        if (arity > 0) {
                            std::cout << "Expected " << action_context->action->arity << " parameters"
                                      << " for action " << action << ". Only read " << action_context->action->arity - arity
                                      << "." << std::endl;
                                      return false;
                        }
                    } else if (arity < 0) { // if read parameters at least = arity
                        // When arity is an "at least" representation we eat arguments
                        // until we either run out or until we hit a delimiter.
                        do {
                            ++i;
                            if (argv[i][0] == '-') {
                                if (!parseFlag(argv, i)) {
                                    return false;
                                }
                                if (abort_parse_) {
                                    return true;
                                }
                            }else {
                                action_context->arguments.push_back(argv[i]);
                                --arity;
                            }
                        } while (argv[i+1] && std::find(delimiters_.begin(), delimiters_.end(), argv[i+1]) == delimiters_.end());
                        if (arity > 0) {
                            std::cout << "Expected " << action_context->action->arity << " parameters"
                                      << " for action " << action << ". Only read " << action_context->action->arity - arity
                                      << "." << std::endl;
                                      return false;
                        }
                    }
                } else {
                    std::cout << "Unknown action: " << argv[i] << std::endl;
                    return false;
                }
            }
        }

        parsed_ = true;
        return true;
    }

    // Dynamically output help information based on registered global and action
    // specific flags
    void help() {
        abort_parse_ = true;
        if (current_context_->action) {
            actionHelp();
        } else {
            globalHelp();
        }
    }

    bool whisper() {
        if (abort_parse_) {
            return true;
        }

        current_context_ = global_context_;
        bool previous_result = true;
        if (current_context_->next) {
            while ((current_context_ = current_context_->next)) {
                if (current_context_->action) {
                    if (!previous_result) {
                        std::cout << "Not starting action '" << current_context_->action->name
                                  << "'. Previous action failed to complete successfully." << std::endl;
                    } else {
                        // Flip it because success is 0
                        previous_result = !current_context_->action->action_callback(current_context_->arguments);
                        if (!current_context_->action->chainable) {
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

    template <typename FlagType>
    void defineGlobalFlag(std::string aliases, std::string description,
                           FlagType default_value, ValidationCallback<FlagType> validation_callback){
        Flag<FlagType>* flagp = new Flag<FlagType>();
        flagp->aliases = aliases;
        flagp->value = default_value;
        flagp->description = description;
        flagp->validation_callback = validation_callback;
        // Aliases are space separated
        std::istringstream iss(aliases);
        while (iss) {
            std::string tmp;
            iss >> tmp;
            global_context_->flags[tmp] = flagp;
        }

        // vlevel is special and we don't want it showing up in the help list
        if (aliases != "vlevel") {
            registered_flags_["global"].push_back(flagp);
        }
    }

    template <typename FlagType>
    void defineActionFlag(std::string action_name, std::string aliases, std::string description,
                           FlagType default_value, ValidationCallback<FlagType> validation_callback){
        Flag<FlagType>* flagp = new Flag<FlagType>();
        flagp->aliases = aliases;
        flagp->value = default_value;
        flagp->description = description;
        flagp->validation_callback = validation_callback;
        // Aliases are space separated
        std::istringstream iss(aliases);
        std::string tmp;
        while (iss >> tmp) {
            actions[action_name]->flags[tmp] = flagp;
        }
        registered_flags_[action_name].push_back(flagp);
    }

    void defineAction(std::string name, int arity, bool chainable, std::string description, std::string help_string,
                      ActionCallback action_callback) {

        Action* actionp = new Action();
        actionp->name = name;
        actionp->arity = arity;
        actionp->description = description;
        actionp->help_string_ = help_string;
        actionp->action_callback = action_callback;
        actionp->chainable = chainable;
        actions[name] = actionp;
    }

    template <typename FlagType>
    FlagType getFlagValue(std::string name) {
        if (Context* context = definedInContext(name)) {
            return static_cast<Flag<FlagType>*>(context->flags[name])->value;
        }

        return FlagType();
    };

    // ALSO check both contexts
    template <typename FlagType>
    bool setFlag(std::string name, FlagType value) {
        Context* context = definedInContext(name);
        if (context) {
            Flag<FlagType>* flagp = static_cast<Flag<FlagType>*>(context->flags[name]);
            FlagType tmp_value = flagp->value;
            flagp->value = value;

            // If there is a validation callback, do it
            if (flagp->validation_callback && !flagp->validation_callback(value)) {
                flagp->value = tmp_value;
                return false;
            }

            return true;
        }

        return false;
    };

  private:
    Context* global_context_;
    Context* current_context_;
    std::map<std::string, Action*> actions;
    std::map<std::string, std::vector<FlagBase*>> registered_flags_;
    bool parsed_ = false;
    std::vector<std::string> delimiters_;
    std::string application_name_ = "";
    std::string help_banner_ = "";
    std::string version_string_ = "";
    bool abort_parse_ = false; // abort parse if condition is hit (version or help flag)

    bool parseFlag(char* argv[], int& i) {
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
                return true;
            }
        }

        // Deal with the special help flag
        if (flagname == "help" || flagname == "h") {
            help();
            return true;
        }

        // Deal with the special --version flag
        if (flagname == "version") {
            std::cout << version_string_;
            abort_parse_ = true;
            return true;
        }

        if (!isFlagDefined(flagname)) {
            // TODO(ploubser): Should I warn context?
            std::cout << "Unknown flag: " << flagname << std::endl;
            return false;
        }

        FlagBase* flagp = definedInContext(flagname)->flags[flagname];

        // RTTI to determine how set the flag value
        if (dynamic_cast<Flag<bool>*>(flagp)) {
            return setFlag<bool>(flagname, true);
        } else if (dynamic_cast<Flag<std::string>*>(flagp)) {
            if (argv[++i]) {
                return setFlag<std::string>(flagname, std::string(argv[i]));
            } else {
                std::cout << "Missing value for flag: " << argv[i-1] << std::endl;
                return false;
            }
        } else if (dynamic_cast<Flag<int>*>(flagp)) {
            if (argv[++i]) {
                    // Validate string looks like an interger
                if (validateInteger(argv[i])) {
                    return setFlag<int>(flagname, std::stol(argv[i], nullptr, 10));
                } else {
                    std::cout << "Flag '" << flagname << "' expects a value of type integer" << std::endl;
                    return false;
                }
            } else {
                std::cout << "Missing value for flag: " << argv[i-1] << std::endl;
                return false;
            }
        }

        std::cout << flagname << " is not of a valid flag type." << std::endl;
        return false;
    }

    // Display help information for the global context
    void globalHelp() {
        std::cout << help_banner_ << std::endl;
        std::cout << std::endl;

        std::cout << "Global options:" << std::endl;

        for (const auto& flag : registered_flags_["global"]) {
            writeFlagHelp(flag);
        }
        std::cout << std::endl << std::endl;

        std::cout << "Actions:" << std::endl << std::endl;
        for (const auto& action : actions) {
            writeActionDescription(action.second);
        }

        std::cout << std::endl;

        for (const auto& context : registered_flags_) {
            if (context.first != "global") {
                std::cout << context.first << " action options:" << std::endl;
                for (const auto& flag : context.second) {
                    writeFlagHelp(flag);
                }
                std::cout << std::endl << std::endl;
            }
        }
        std::cout << "For action specific help run \"" << application_name_ << " <action> --help\"" << std::endl;
    }

    // Display help information for the current action context
    void actionHelp() {
        if (current_context_->action->help_string_.empty()) {
            std::cout << "No specific help found for action :" << current_context_->action->name
                      << std::endl << std::endl;
            return;
        }

        std::cout << current_context_->action->help_string_;

        if (registered_flags_.find(current_context_->action->name) != registered_flags_.end()) {
            std::cout << std::endl << "  " << current_context_->action->name << " specific flags:" << std::endl;
            for (const auto& flag : registered_flags_[current_context_->action->name]) {
                writeFlagHelp(flag);
            }
        }
        std::cout << std::endl << std::endl;
    }

    // Output the help information related to a single flag
    void writeFlagHelp(const FlagBase* flag) {
        std::stringstream input(flag->aliases);
        std::stringstream output;
        std::string tmp;

        while (input >> tmp) {
            if (tmp != "") {
                output << std::endl;
                output << std::setw(30) << std::left;

                if (tmp.size() == 1) {
                    output << "   -" + tmp;
                } else if (tmp.size() > 1) {
                    output << "  --" + tmp;
                }
            }
        }

        output << flag->description;
        std::cout << output.str();
    }

    // Output the action description related to a specific action
    void writeActionDescription(const Action* action) {
        std::stringstream example;
        example << "  " << action->name;
        std::cout << std::setw(30) << std::left << example.str()
                  << std::setw(30) << std::left << action->description << std::endl;
    }

    bool isFlagDefined(std::string name) {
        if (definedInContext(name)) {
            return true;
        } else {
            return false;
        }
    }

    Context* definedInContext(std::string name) {
        if (current_context_->flags.find(name) != current_context_->flags.end()) {
            return current_context_;
        } else if (global_context_->flags.find(name) != global_context_->flags.end()) {
            return global_context_;
        } else {
            return nullptr;
        }
    }

    bool isActionDefined(std::string name) {
        return !(actions.find(name) == actions.end());
    }
};


template <typename FlagType>
static void DefineGlobalFlag(std::string aliases,
                                std::string description,
                                FlagType default_value,
                                ValidationCallback<FlagType> validation_callback) {
    HorseWhisperer::Instance().defineGlobalFlag<FlagType>(aliases,
                                                         description,
                                                         default_value,
                                                         validation_callback);
}

template <typename FlagType>
static void DefineActionFlag(std::string action_name,
                                std::string aliases,
                                std::string description,
                                FlagType default_value,
                                ValidationCallback<FlagType> validation_callback) {
    HorseWhisperer::Instance().defineActionFlag<FlagType>(action_name,
                                                         aliases,
                                                         description,
                                                         default_value,
                                                         validation_callback);
}

template <typename FlagType>
static FlagType GetFlag(std::string flag_name) {
    return HorseWhisperer::Instance().getFlagValue<FlagType>(flag_name);
}

template <typename FlagType>
static bool SetFlag(std::string flag_name, FlagType value) {
    return HorseWhisperer::Instance().setFlag<FlagType>(flag_name, value);
}

static void DefineAction(std::string action_name, int arity, bool chainable, std::string description,
                           std::string help_string, ActionCallback action_callback) {
    HorseWhisperer::Instance().defineAction(action_name, arity, chainable, description, help_string, action_callback);

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

static bool Parse(int argc, char** argv) {
    return HorseWhisperer::Instance().parse(argc, argv);
}

static int Start() {
    return HorseWhisperer::Instance().whisper();
}

}  // namespace HorseWhisperer

#endif  // HORSEWHISPERER_INCLUDE_HORSE_WHISPERER_H_
