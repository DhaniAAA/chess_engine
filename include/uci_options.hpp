#ifndef UCI_OPTIONS_HPP
#define UCI_OPTIONS_HPP

// ============================================================================
// UCI Options Manager
// ============================================================================
// Provides a cleaner, type-safe way to manage UCI options using a map pattern
// instead of long if-else chains.
// ============================================================================

#include <string>
#include <functional>
#include <map>
#include <variant>
#include <iostream>

namespace UCI {

// ============================================================================
// Option Types
// ============================================================================

enum class OptionType {
    SPIN,    // Integer with min/max
    CHECK,   // Boolean
    STRING,  // String value
    BUTTON,  // Action trigger (no value)
    COMBO    // Dropdown (not implemented yet)
};

// ============================================================================
// Option Definition
// ============================================================================

struct OptionDef {
    OptionType type;
    std::string defaultValue;
    int minVal = 0;
    int maxVal = 0;
    std::function<void(const std::string&)> onChange;

    // Constructors for different option types
    static OptionDef Spin(int defaultVal, int min, int max,
                          std::function<void(const std::string&)> callback = nullptr) {
        OptionDef opt;
        opt.type = OptionType::SPIN;
        opt.defaultValue = std::to_string(defaultVal);
        opt.minVal = min;
        opt.maxVal = max;
        opt.onChange = callback;
        return opt;
    }

    static OptionDef Check(bool defaultVal,
                           std::function<void(const std::string&)> callback = nullptr) {
        OptionDef opt;
        opt.type = OptionType::CHECK;
        opt.defaultValue = defaultVal ? "true" : "false";
        opt.onChange = callback;
        return opt;
    }

    static OptionDef String(const std::string& defaultVal,
                            std::function<void(const std::string&)> callback = nullptr) {
        OptionDef opt;
        opt.type = OptionType::STRING;
        opt.defaultValue = defaultVal;
        opt.onChange = callback;
        return opt;
    }

    static OptionDef Button(std::function<void(const std::string&)> callback) {
        OptionDef opt;
        opt.type = OptionType::BUTTON;
        opt.onChange = callback;
        return opt;
    }
};

// ============================================================================
// Options Manager
// ============================================================================

class OptionsManager {
public:
    // Register an option
    void add(const std::string& name, OptionDef option) {
        options_[name] = option;
        values_[name] = option.defaultValue;
    }

    // Set option value (called by setoption command)
    bool set(const std::string& name, const std::string& value) {
        auto it = options_.find(name);
        if (it == options_.end()) {
            return false;  // Unknown option
        }

        OptionDef& opt = it->second;

        // Validate and set
        switch (opt.type) {
            case OptionType::SPIN: {
                int val = std::stoi(value);
                val = std::clamp(val, opt.minVal, opt.maxVal);
                values_[name] = std::to_string(val);
                break;
            }
            case OptionType::CHECK:
                values_[name] = (value == "true") ? "true" : "false";
                break;
            case OptionType::STRING:
                values_[name] = value;
                break;
            case OptionType::BUTTON:
                // Buttons don't store values, just trigger callback
                break;
            default:
                values_[name] = value;
        }

        // Call onChange callback if defined
        if (opt.onChange) {
            opt.onChange(values_[name]);
        }

        return true;
    }

    // Get option value as int
    int getInt(const std::string& name) const {
        auto it = values_.find(name);
        if (it != values_.end()) {
            return std::stoi(it->second);
        }
        return 0;
    }

    // Get option value as bool
    bool getBool(const std::string& name) const {
        auto it = values_.find(name);
        if (it != values_.end()) {
            return it->second == "true";
        }
        return false;
    }

    // Get option value as string
    std::string getString(const std::string& name) const {
        auto it = values_.find(name);
        if (it != values_.end()) {
            return it->second;
        }
        return "";
    }

    // Print all options in UCI format
    void printAll() const {
        for (const auto& [name, opt] : options_) {
            std::cout << "option name " << name << " type ";

            switch (opt.type) {
                case OptionType::SPIN:
                    std::cout << "spin default " << opt.defaultValue
                              << " min " << opt.minVal
                              << " max " << opt.maxVal;
                    break;
                case OptionType::CHECK:
                    std::cout << "check default " << opt.defaultValue;
                    break;
                case OptionType::STRING:
                    std::cout << "string default " << opt.defaultValue;
                    break;
                case OptionType::BUTTON:
                    std::cout << "button";
                    break;
                default:
                    std::cout << "string default " << opt.defaultValue;
            }

            std::cout << std::endl;
        }
    }

private:
    std::map<std::string, OptionDef> options_;
    std::map<std::string, std::string> values_;
};

} // namespace UCI

#endif // UCI_OPTIONS_HPP
