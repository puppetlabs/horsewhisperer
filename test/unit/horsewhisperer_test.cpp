#include <horsewhisperer/horsewhisperer.h>
#include "../test.h"

namespace HW = HorseWhisperer;

void prepareGlobal() {
    // configure horsewhisperer
    HW::SetAppName("test-app");
    HW::DefineGlobalFlag<bool>("global-get", "a test flag", false, nullptr);
    HW::DefineGlobalFlag<int>("global-bad-flag", "a bad test flag",
                              false, nullptr);
}

void prepareAction(std::function<int(std::vector<std::string>)> f) {
    HW::DefineAction("test-action", 0, false, "no description", "no help", f);
    HW::DefineActionFlag<bool>("test-action", "action-get", "a test flag",
                               false, nullptr);
}

TEST_CASE("reset", "[reset]") {
    SECTION("Reset resets global flags") {
        prepareGlobal();
        HW::Reset();
        REQUIRE_THROWS_AS(HW::GetFlag<bool>("global-get"),
                          HW::undefined_flag_error);
    }
}

TEST_CASE("global GetFlag", "[global getflag]") {
    HW::Reset();
    prepareGlobal();

    SECTION("it returns the default value of a unset flag") {
        REQUIRE(HW::GetFlag<bool>("global-get") == false);
    }

    SECTION("it throws an exception when trying to access and undefined flag") {
        REQUIRE_THROWS_AS(HW::GetFlag<bool>("not-global-get"),
                          HW::undefined_flag_error);
    }
}

TEST_CASE("setDelimiters - isDelimiter", "[global setting]") {
    HW::Reset();
    prepareGlobal();

    SECTION("no delimiters set") {
        REQUIRE(!HW::HorseWhisperer::Instance().isDelimiter(","));
    }

    SECTION("can set and check delimiters") {
        HW::HorseWhisperer::Instance().setDelimiters({ ",", "*" });

        REQUIRE(HW::HorseWhisperer::Instance().isDelimiter(","));
        REQUIRE(HW::HorseWhisperer::Instance().isDelimiter("*"));
        REQUIRE(!HW::HorseWhisperer::Instance().isDelimiter("+"));
    }
}

TEST_CASE("global SetFlag", "[global setflag]") {
    HW::Reset();
    prepareGlobal();

    SECTION("it sets the value of a flag") {
        HW::SetFlag<bool>("global-get", true);
        REQUIRE(HW::GetFlag<bool>("global-get") == true);
    }

    SECTION("it sets the value of an int option") {
        HW::DefineGlobalFlag<int>("global-int", "test", 1, nullptr);
        HW::SetFlag<int>("global-int", 42);
        REQUIRE(HW::GetFlag<int>("global-int") == 42);
    }

    SECTION("it sets the value of a double option") {
        HW::DefineGlobalFlag<double>("global-double", "test", 1.1, nullptr);
        HW::SetFlag<double>("global-double", 3.14);
        REQUIRE(HW::GetFlag<double>("global-double") == 3.14);
    }

    SECTION("it sets the value of a string option") {
        HW::DefineGlobalFlag<std::string>("global-string", "test", "bar", nullptr);
        HW::SetFlag<std::string>("global-string", "foo");
        REQUIRE(HW::GetFlag<std::string>("global-string") == "foo");
    }

    SECTION("it throws when trying to set an undefined flag") {
        REQUIRE_THROWS_AS(HW::SetFlag<bool>("not-global-get", false),
                          HW::undefined_flag_error);
    }

    SECTION("it does not throws when flag validation succeeds") {
        HW::DefineGlobalFlag<bool>("global-success",
                                   "a test flag",
                                   false,
                                   [](bool) {});
        REQUIRE_NOTHROW(HW::SetFlag<bool>("global-success", false));
    }

    SECTION("it throws when flag validation fails") {
        HW::FlagCallback<bool> v_c;

        SECTION("flag_validation_error is thrown") {
            v_c = [](bool) { throw HW::flag_validation_error { "error!" }; };
        }

        SECTION("runtime_error is thrown") {
            v_c = [](bool) { throw std::runtime_error { "error!" }; };
        }

        HW::DefineGlobalFlag<bool>("global-failure",
                                   "a test flag",
                                   false,
                                   v_c);
        REQUIRE_THROWS_AS(HW::SetFlag<bool>("global-failure", false),
                          HW::flag_validation_error);
    }
}

TEST_CASE("GetFlagType", "[type]") {
    HW::Reset();
    prepareGlobal();

    SECTION("correctly gives Bool type for a flag") {
        HW::SetFlag<bool>("global-get", true);
        REQUIRE(HW::GetFlagType("global-get") == HW::FlagType::Bool);
    }

    SECTION("correctly gives Int type") {
        HW::DefineGlobalFlag<int>("global-int", "test", 1, nullptr);
        HW::SetFlag<int>("global-int", 42);
        REQUIRE(HW::GetFlagType("global-int") == HW::FlagType::Int);
    }

    SECTION("correctly gives Double type") {
        HW::DefineGlobalFlag<double>("global-double", "test", 1.1, nullptr);
        HW::SetFlag<double>("global-double", 3.14);
        REQUIRE(HW::GetFlagType("global-double") == HW::FlagType::Double);
    }

    SECTION("correctly gives string type") {
        HW::DefineGlobalFlag<std::string>("global-string", "test", "bar", nullptr);
        HW::SetFlag<std::string>("global-string", "foo");
        REQUIRE(HW::GetFlagType("global-string") == HW::FlagType::String);
    }

    SECTION("it throws when trying to set an undefined flag") {
        REQUIRE_THROWS_AS(HW::SetFlag<bool>("not-global-get", false),
                          HW::undefined_flag_error);
    }

    SECTION("it does not throws when flag validation succeeds") {
        HW::DefineGlobalFlag<bool>("global-success",
                                   "a test flag",
                                   false,
                                   [](bool) {});
        REQUIRE_NOTHROW(HW::SetFlag<bool>("global-success", false));
    }

    SECTION("it throws when flag validation fails") {
        HW::FlagCallback<bool> v_c;

        SECTION("flag_validation_error is thrown") {
            v_c = [](bool) { throw HW::flag_validation_error { "error!" }; };
        }

        SECTION("runtime_error is thrown") {
            v_c = [](bool) { throw std::runtime_error { "error!" }; };
        }

        HW::DefineGlobalFlag<bool>("global-failure",
                                   "a test flag",
                                   false,
                                   v_c);

        REQUIRE_THROWS_AS(HW::SetFlag<bool>("global-failure", false),
                         HW::flag_validation_error);
    }

    SECTION("it can get the value for all flag aliasses") {
        HW::DefineGlobalFlag<bool>("a alias", "aliased flag", false, nullptr);
        HW::SetFlag<bool>("a", true);
        REQUIRE(HW::GetFlag<bool>("a") == true);
        REQUIRE(HW::GetFlag<bool>("alias") == true);
    }
}

int getTest(std::vector<std::string>) {
    SECTION("it returns the default value of a unset flag") {
        // check local flag context
        REQUIRE(HW::GetFlag<bool>("action-get") == false);
        // check global flag context
        REQUIRE(HW::GetFlag<bool>("global-get") == false);
        REQUIRE_THROWS_AS(HW::GetFlag<bool>("not-action-get"),
                          HW::undefined_flag_error);
    }
    return 0;
}

TEST_CASE("action GetFlag", "[action getflag]") {
    HW::Reset();
    prepareGlobal();
    prepareAction(getTest);
    const char* args[] = { "test-app", "test-action", nullptr };

    // duck around Wc++11-compat-deprecated-writable-strings
    HW::Parse(2, const_cast<char**>(args));
    HW::Start();
}

int setTest(std::vector<std::string>) {
    SECTION("it set's an action specific flag") {
        HW::SetFlag<bool>("action-get", true);
        REQUIRE(HW::GetFlag<bool>("action-get") == true);
        REQUIRE_THROWS_AS(HW::SetFlag<bool>("not-action-set", false),
                          HW::undefined_flag_error);
    }
    return 0;
}

TEST_CASE("action SetFlag", "[action setflag]") {
    HW::Reset();
    prepareGlobal();
    prepareAction(setTest);
    const char* args[] = { "test-app", "test-action", nullptr };

    // duck around Wc++11-compat-deprecated-writable-strings
    HW::Parse(2, const_cast<char**>(args));
    HW::Start();
}

int testActionCallback(std::vector<std::string> args) {
    return 0;
}

TEST_CASE("parse", "[parse]") {
    HW::Reset();
    prepareGlobal();
    prepareAction(nullptr);

    SECTION("returns ParseResult::OK on success") {
        const char* args[] = { "test-app", "test-action", nullptr };
        REQUIRE(HW::Parse(2, const_cast<char**>(args)) == HW::ParseResult::OK);
    }

    SECTION("returns ParseResult::HELP on the help flag") {
        const char* args[] = { "test-app", "test-action", "--help", nullptr };
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::HELP);
    }

    SECTION("returns ParseResult::VERSION on the --version flag") {
        const char* args[] = { "test-app", "test-action", "--version", nullptr };
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::VERSION);
    }

    SECTION("returns PARSE_FAILURE on bad arguments") {
        const char* args[] = { "test-app", "test-action", "test-smachtions", nullptr };
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::FAILURE);
    }

    SECTION("returns ParseResult::INVALID_FLAG on a bad flag") {
        const char* args[] = { "test-app", "test-action", "--global-bad-flag",
                               "foo", nullptr };
        REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::ParseResult::INVALID_FLAG);
    }

    SECTION("returns ParseResult::OK when mixing key=value and other flags") {
        HW::DefineGlobalFlag<int>("foo", "a int test flag", 0, nullptr);
        HW::DefineGlobalFlag<bool>("bar", "a bool test flag", false, nullptr);

        const char* args[] = { "test-app", "test-action", "--bar", "--foo=5", nullptr };
        REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::ParseResult::OK);
    }

    SECTION("returns ParseResult::OK when flag is given as key=value") {
        HW::DefineGlobalFlag<int>("foo", "a test flag", 0, nullptr);
        const char* args[] = { "test-app", "test-action", "--foo=5", nullptr };
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::OK);
    }

    SECTION("action with null arity") {
        HW::DefineAction("no_arg_action", 0, false, "test action",
                         "no arg required!", testActionCallback);

        SECTION("returns ParseResult::FAILURE if an argument is passed") {
            const char* args[] = { "test-app", "no_arg_action", "bad_arg", nullptr };
            REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::FAILURE);
        }

        SECTION("returns ParseResult::FAILURE if an argument/flag are passed") {
            const char* args[] = { "test-app", "no_arg_action", "bad_arg",
                                   "--verbose", nullptr };
            REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::ParseResult::FAILURE);
        }

        SECTION("returns ParseResult::FAILURE if an flag/argument are passed") {
            const char* args[] = { "test-app", "no_arg_action", "--verbose",
                                   "bad_arg", nullptr };
            REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::ParseResult::FAILURE);
        }

        SECTION("returns ParseResult::OK if no action argument is provided") {
            const char* args[] = { "test-app", "no_arg_action", nullptr };
            REQUIRE(HW::Parse(2, const_cast<char**>(args)) == HW::ParseResult::OK);
        }

        SECTION("returns ParseResult::OK if just a flag is provided") {
            const char* args[] = { "test-app", "no_arg_action", "--verbose", nullptr };
            REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::OK);
        }
    }

    SECTION("action with fixed arity") {
        HW::DefineAction("two_args_action", 2, false, "test action",
                         "2 args required!", testActionCallback);

        SECTION("returns ParseResult::FAILURE if no argument is passed") {
            const char* args[] = { "test-app", "two_args_action", nullptr };
            REQUIRE(HW::Parse(2, const_cast<char**>(args)) == HW::ParseResult::FAILURE);
        }

        SECTION("return ParseResult::FAILURE if one action argument is missing") {
            const char* args[] = { "test-app", "two_args_action", "spam", nullptr };
            REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::FAILURE);
        }

        SECTION("returns ParseResult::FAILURE if one action arguments is missing and a "
                "flag is passed") {
            const char* args[] = { "test-app", "two_args_action", "spam",
                                   "--verbose", nullptr };
            REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::ParseResult::FAILURE);
        }

        SECTION("returns ParseResult::OK if all action arguments are provided") {
            const char* args[] = { "test-app", "two_args_action", "spam",
                                   "eggs", nullptr };
            REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::ParseResult::OK);
        }
    }

    SECTION("action with variable num of arguments") {
        SECTION("zero or more arguments") {
            HW::DefineAction("var_args_action", 0, false, "test action",
                             "more than 2 args required!", testActionCallback,
                             nullptr, true);

            SECTION("returns ParseResult::OK if no action argument is provided") {
                const char* args[] = { "test-app", "var_args_action", nullptr };
                REQUIRE(HW::Parse(2, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            SECTION("returns ParseResult::OK if 4 action arguments are provided") {
                const char* args[] = { "test-app", "var_args_action",
                                       "foo", "bar", "spam", "beans", nullptr };
                REQUIRE(HW::Parse(6, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            SECTION("returns ParseResult::OK if 4 action arguments and a global "
                    "are provided") {
                const char* args[] = { "test-app", "var_args_action",
                                       "foo", "bar", "--verbose", "spam", "beans",
                                       nullptr };
                REQUIRE(HW::Parse(7, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            SECTION("returns ParseResult::OK if 4 action arguments and a flag "
                     "are provided") {
                const char* args[] = { "test-app", "var_args_action",
                                       "foo", "bar", "spam", "beans", "--verbose",
                                       nullptr };
                REQUIRE(HW::Parse(7, const_cast<char**>(args)) == HW::ParseResult::OK);
            }
        }

        SECTION("two or more arguments") {
            HW::DefineAction("two_or_more_args_action", 2, false, "test action",
                             "more than 2 args required!", testActionCallback,
                             nullptr, true);

            SECTION("returns ParseResult::FAILURE if no action arguments is passed") {
                const char* args[] = { "test-app", "two_or_more_args_action", nullptr };
                REQUIRE(HW::Parse(2, const_cast<char**>(args)) == HW::ParseResult::FAILURE);
            }

            SECTION("return ParseResult::FAILURE if one action argument is missing") {
                const char* args[] = { "test-app", "two_or_more_args_action",
                                       "spam", nullptr };
                REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::FAILURE);
            }

            SECTION("returns ParseResult::OK if 2 action arguments are provided") {
                const char* args[] = { "test-app", "two_or_more_args_action",
                                       "foo", "bar", nullptr };
                REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            SECTION("returns ParseResult::OK if 4 action arguments are provided") {
                const char* args[] = { "test-app", "two_or_more_args_action",
                                       "foo", "bar", "spam", "beans", nullptr };
                REQUIRE(HW::Parse(6, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            SECTION("returns ParseResult::OK if 4 action arguments and a flag "
                     "are provided") {
                const char* args[] = { "test-app", "two_or_more_args_action",
                                       "foo", "bar", "spam", "beans", "--verbose",
                                       nullptr };
                REQUIRE(HW::Parse(7, const_cast<char**>(args)) == HW::ParseResult::OK);
            }
        }
    }

    SECTION("multiple actions") {
        HW::DefineAction("no_arg_action", 0, false, "test action",
                         "no arg required!", testActionCallback);

        HW::DefineAction("two_args_action", 2, false, "test action",
                         "2 args required!", testActionCallback);

        HW::DefineAction("var_args_action", 0, false, "test action",
                         "more than 2 args required!", testActionCallback,
                         nullptr, true);

        HW::DefineAction("two_or_more_args_action", 2, false, "test action",
                         "more than 2 args required!", testActionCallback,
                         nullptr, true);

        SECTION("correctly return ParseResult::OK") {
            const char* args[] = { "test-app",
                                   "no_arg_action",
                                   "no_arg_action",
                                   "two_args_action", "foo", "bar",
                                   "var_args_action",
                                   "var_args_action", "a", "b", "c", nullptr };
            REQUIRE(HW::Parse(11, const_cast<char**>(args)) == HW::ParseResult::OK);
        }

        SECTION("correctly return ParseResult::OK with a global flag in the middle") {
            const char* args[] = { "test-app",
                                   "no_arg_action",
                                   "no_arg_action",
                                   "two_args_action", "foo", "--verbose", "bar",
                                   "var_args_action",
                                   "var_args_action", "a", "b", "c", nullptr };
            REQUIRE(HW::Parse(12, const_cast<char**>(args)) == HW::ParseResult::OK);
        }

        SECTION("correctly return ParseResult::FAILURE") {
            const char* args[] = { "test-app",
                                   "no_arg_action",
                                   "no_arg_action", "bad_arg",  // error
                                   "two_args_action", "foo", "bar",
                                   "var_args_action", "spam", "eggs",
                                   "var_args_action",
                                   "two_or_more_args_action", "a", "b", "c", nullptr };
            REQUIRE(HW::Parse(15, const_cast<char**>(args)) == HW::ParseResult::FAILURE);
        }

        SECTION("correctly return ParseResult::OK with variable args actions") {
            const char* args[] = { "test-app",
                                   "no_arg_action",
                                   "no_arg_action",
                                   "two_args_action", "foo", "bar",
                                   "var_args_action", "spam", "eggs",
                                   "var_args_action",
                                   "two_or_more_args_action", "a", "b", "c", "d", "e",
                                   "no_arg_action",
                                   "var_args_action", "maradona", nullptr };
            REQUIRE(HW::Parse(19, const_cast<char**>(args)) == HW::ParseResult::OK);
        }
    }

    SECTION("it parses and sets aliased flags") {
        HW::DefineGlobalFlag<bool>("a alias", "aliased flag", false, nullptr);
        const char* args[] = { "test-app", "test-action", "-a", nullptr };
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::OK);
        REQUIRE(HW::GetFlag<bool>("a") == true);
        REQUIRE(HW::GetFlag<bool>("alias") == true);
    }

    SECTION("it parses and sets aliased action flags") {
        HW::DefineActionFlag<bool>("test-action", "a alias", "aliased flag", false, nullptr);
        const char* args[] = { "test-app", "test-action", "-a", nullptr };
        REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::OK);
        REQUIRE(HW::GetFlag<bool>("a") == true);
        REQUIRE(HW::GetFlag<bool>("alias") == true);
    }

    SECTION("it parses and sets integer numbers") {
        HW::DefineGlobalFlag<int>("int-flag", "no useful description", 42, nullptr);

        SECTION("positive") {
            SECTION("value after space") {
                const char* args[] = { "test-app", "test-action", "--int-flag", "3",
                                       nullptr };
                REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            SECTION("key=value format") {
                const char* args[] = { "test-app", "test-action", "--int-flag=3",
                                       nullptr };
                REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            REQUIRE(HW::GetFlag<int>("int-flag") == 3);
        }

        SECTION("negative") {
            SECTION("value after space") {
                const char* args[] = { "test-app", "test-action", "--int-flag", "-4",
                                       nullptr };
                REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            SECTION("key=value format") {
                const char* args[] = { "test-app", "test-action", "--int-flag=-4",
                                       nullptr };
                REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            REQUIRE(HW::GetFlag<int>("int-flag") == -4);
        }
    }

    SECTION("it parses negative doubles") {
        HW::DefineGlobalFlag<double>("double-flag", "no useful description", 4.2,
                                     nullptr);

        SECTION("positive") {
            SECTION("value after space") {
                const char* args[] = { "test-app", "test-action", "--double-flag",
                                       "2.718", nullptr };
                REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            SECTION("key=value format") {
                const char* args[] = { "test-app", "test-action",
                                       "--double-flag=2.718", nullptr };
                REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            REQUIRE(HW::GetFlag<double>("double-flag") == 2.718);
        }

        SECTION("negative") {
            SECTION("value after space") {
                const char* args[] = { "test-app", "test-action", "--double-flag",
                                       "-3.14", nullptr };
                REQUIRE(HW::Parse(4, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            SECTION("key=value format") {
                const char* args[] = { "test-app", "test-action", "--double-flag=-3.14",
                                       nullptr };
                REQUIRE(HW::Parse(3, const_cast<char**>(args)) == HW::ParseResult::OK);
            }

            REQUIRE(HW::GetFlag<double>("double-flag") == -3.14);
        }
    }
}

auto action_callback = [](std::vector<std::string>) -> int { return 0; };

TEST_CASE("HorseWhisperer::getActions" "[getActions]") {
    HW::Reset();
    prepareGlobal();
    HorseWhisperer::DefineAction("new_action", 2, true, "no description",
                                 "no help", action_callback);

    SECTION("returns a vector containing a single action name") {
        const char* args[] = { "test-app", "new_action", "spam", "eggs", nullptr };
        std::vector<std::string> test_result { "new_action" };
        HW::Parse(4, const_cast<char**>(args));
        REQUIRE(HW::GetParsedActions() == test_result);
    }

    HorseWhisperer::DefineAction("new_action_2", 0, true, "no description",
                                  "no help", action_callback);

    SECTION("returns multiple action names") {
        const char* args[] = { "test-app", "new_action", "spam", "eggs",
                               "new_action_2", nullptr };
        std::vector<std::string> test_result { "new_action", "new_action_2" };
        HW::Parse(5, const_cast<char**>(args));
        REQUIRE(HW::GetParsedActions() == test_result);
    }

    std::vector<std::string> delim { "+" };
    HW::SetDelimiters(delim);

    SECTION("works properly with user-defined delimiters") {
        const char* args[] = { "test-app", "new_action", "foo", "bar",
                               "+", "new_action_2", nullptr };
        std::vector<std::string> test_result { "new_action", "new_action_2" };
        HW::Parse(6, const_cast<char**>(args));
        REQUIRE(HW::GetParsedActions() == test_result);
    }

    SECTION("returns duplicate actions") {
        const char* args[] = { "test-app", "new_action", "foo", "bar",
                               "+", "new_action_2",
                               "+", "new_action", "spam", "eggs", nullptr };
        std::vector<std::string> test_result { "new_action", "new_action_2",
                                               "new_action" };
        HW::Parse(10, const_cast<char**>(args));
        REQUIRE(HW::GetParsedActions() == test_result);
    }
}

TEST_CASE("HorseWhisperer::Start", "[start]") {
    HW::Reset();
    prepareGlobal();

    SECTION("returns false in case of missing action callback") {
        HW::DefineAction("start_test_1", 0, false, "test-action", "no help",
                         nullptr);
        REQUIRE(!HW::Start());
    }

    SECTION("it executes an action") {
        int modify_me = 0;
        HW::DefineAction("start_test_1", 0, false, "test-action", "no help",
                         [&modify_me](std::vector<std::string>) -> int {
                            return ++modify_me; });

        const char* args[] = { "test-app", "start_test_1", nullptr };
        HW::Parse(2, const_cast<char**>(args));
        HW::Start();
        REQUIRE(modify_me == 1);
    }

    SECTION("it can chain actions") {
        int modify_me1 = 0;
        int modify_me2 = 1;
        std::vector<std::string> delim { "+" };
        HW::SetDelimiters(delim);

        HW::DefineAction("chain_test_1", 0, true, "test-action", "no help",
                         [&modify_me1](std::vector<std::string>) -> int {
                            ++modify_me1; return 0; });

        HW::DefineAction("chain_test_2", 0, true, "test-action", "no help",
                         [&modify_me2](std::vector<std::string>) -> int {
                            ++modify_me2; return 0; });

        const char* args[] = { "test-app", "chain_test_1", "+", "chain_test_2",
                               nullptr };
        HW::Parse(4, const_cast<char**>(args));
        HW::Start();
        REQUIRE(modify_me1 == 1);
        REQUIRE(modify_me2 == 2);
    }

    SECTION("chained actions have confined flags and arguments") {
        int call_counter = 0;

        auto a_c = [&call_counter](std::vector<std::string> args) -> int {
            REQUIRE(args.size() == 1);
            auto t_v = HW::GetFlag<std::string>("test_flag");
            if (call_counter == 0) {
                REQUIRE(args[0] == "arg_one");
                REQUIRE(t_v == "spam");
            } else if (call_counter == 1) {
                REQUIRE(args[0] == "arg_two");
                REQUIRE(t_v == "eggs");
            } else if (call_counter == 2) {
                REQUIRE(args[0] == "arg_three");
                REQUIRE(t_v == "beans");
            } else {
                REQUIRE(false);
            }
            call_counter++;
            return 0;
        };

        HW::DefineAction("chain_test_3", 1, true, "test-action", "no help", a_c);
        HW::DefineActionFlag<std::string>("chain_test_3", "test_flag",
                                          "no description", "foo", nullptr);

        const char* args[] = { "test-app",
                               "chain_test_3", "arg_one", "--test_flag", "spam",
                               "chain_test_3", "arg_two", "--test_flag", "eggs",
                               "chain_test_3", "arg_three", "--test_flag", "beans",
                               nullptr };

        HW::Parse(13, const_cast<char**>(args));
        HW::Start();
        REQUIRE(call_counter == 3);
    }
}
