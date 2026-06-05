#pragma once

#include <string>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <filesystem>
#include <unicode/msgfmt.h>
#include <unicode/locid.h>
#include <variant/variant.hpp>

namespace twig {

class TranslationException: public std::runtime_error {
    public:
    TranslationException(const std::string &msg): std::runtime_error(msg) {}
};


struct TranslatableMessage {
    TranslatableMessage() = default ;
    explicit TranslatableMessage(const std::string &key, const Variant::Object &args = {}):
        key_(key), args_(args) {}
        
    bool empty() const { return key_.empty() ; }
    std::string key_ ; 
    Variant::Object args_;
};

inline TranslatableMessage tr(const char *msg, const Variant::Object &args = {}) {
    return TranslatableMessage{msg};
}

class Translator {
private:
    struct Impl ;
    std::unique_ptr<Impl> impl_ ;

public:
    // Pass the locale string during construction
    Translator(const std::string& target_locale) ;

    std::string getLocale() const ;

    bool loadTranslations(const std::string& filePath) ;

    std::string translate(const std::string& key, 
                          const Variant::Object &args = {}) ;
};


class TranslationManager {
private:
    std::unordered_map<std::string, std::shared_ptr<Translator>> translators_;
    std::string default_locale_;
    std::shared_mutex mutex_; 

public:
    TranslationManager(const std::string& default_locale = "en_US") 
        : default_locale_(default_locale) {}

    TranslationManager(const TranslationManager&) = delete;
    TranslationManager& operator=(const TranslationManager&) = delete;

    // Discover and load all translation JSON files inside a folder (e.g., locales/en_US.json)
    void loadAllFromDirectory(const std::string& dirPath) ;

    std::vector<std::string> getSupportedLocales() const ;

    // Thread-safe lookup for a specific translator
    std::shared_ptr<Translator> getTranslator(const std::string& requested_locale) ;

    std::string translate(const std::string &msg, const std::string &locale, const Variant::Object &params = {}) {
        auto translator = getTranslator(locale);
        return translator->translate(msg, params);
    }

    std::string translate(const twig::TranslatableMessage &msg, const std::string &locale) {
        return translate(msg.key_, locale, msg.args_) ;
    }
};



}