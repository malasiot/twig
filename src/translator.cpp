#include <twig/translator.hpp>
#include <string>
#include <unordered_map>
#include <fstream>
#include <mutex>
#include <shared_mutex>
#include <filesystem>
#include <unicode/msgfmt.h>
#include <unicode/locid.h>

namespace twig {

struct Translator::Impl {
    Impl(const std::string& target_locale) 
        : locale_str_(target_locale), locale_(target_locale.c_str()) {}

    bool loadTranslations(const std::string& filePath) {
        std::ifstream file(filePath);
        if (!file.is_open()) return false;
        Variant j = Variant::fromJSONFile(filePath) ;
        
        for (auto& [key, value] : j.toObject()) {
            if (value.isString()) {
                translations_[key] = value.toString();
            }
        }
        return true;
    }

    std::string translate(const std::string& key, 
                          const Variant::Object &args) {

        string pattern ;
        auto it = translations_.find(key);
        if (it == translations_.end()) pattern = key ;
        else pattern = it->second ;

        UErrorCode status = U_ZERO_ERROR;
        icu::UnicodeString uc_pattern = icu::UnicodeString::fromUTF8(pattern);
        icu::MessageFormat msgFmt(uc_pattern, locale_, status);
        if (U_FAILURE(status)) return "Format Error";

        int argCount = args.size() ;
        auto argumentNames = std::make_unique<icu::UnicodeString[]>(argCount);
        auto arguments = std::make_unique<icu::Formattable[]>(argCount);

        int i = 0;
        for (const auto& [name, val] : args) {
            argumentNames[i] = icu::UnicodeString::fromUTF8(name);
            if ( val.type() == Variant::Type::Float )
                arguments[i] = icu::Formattable(val.toFloat());
            else if ( val.isInteger() )
                arguments[i] = icu::Formattable(val.toInteger()) ;
            else 
                arguments[i] = icu::Formattable(icu::UnicodeString::fromUTF8(val.toString()));
            i++;
        }

        icu::UnicodeString result;
        icu::FieldPosition pos(icu::FieldPosition::DONT_CARE);

        msgFmt.format(argumentNames.get(), arguments.get(), argCount, result, status);

        if (U_FAILURE(status)) {
            throw TranslationException("Evaluation Error: " + std::string(u_errorName(status)));
        }
     

        std::string utf8Result;
        result.toUTF8String(utf8Result);
        return utf8Result;
    }

    std::string locale_str_;
    icu::Locale locale_;
    std::unordered_map<std::string, std::string> translations_;
};

Translator::Translator(const std::string &loc) {
    impl_.reset(new Impl(loc)) ;
}

std::string Translator::getLocale() const { return impl_->locale_str_; }

bool Translator::loadTranslations(const std::string& filePath) {
    return impl_->loadTranslations(filePath) ;
}

std::string Translator::translate(const std::string& key, 
                          const Variant::Object &args) {
                            return impl_->translate(key, args) ;
}


void TranslationManager::loadAllFromDirectory(const std::string& dirPath) {
    std::unique_lock<std::shared_mutex> lock(mutex);
    for ( const auto& entry : std::filesystem::directory_iterator(dirPath)) {
        if ( entry.path().extension() == ".json" ) {
            std::string locale_name = entry.path().stem().string(); // e.g., "fr_FR"
                
            auto translator = std::make_shared<Translator>(locale_name);

            if ( translator->loadTranslations(entry.path().string()) ) {
                translators_[locale_name] = translator;
            }
        }
    }
}

std::vector<std::string> TranslationManager::getSupportedLocales() const {
    vector<string> res ;
    for ( const auto &[key, _]: translators_)
        res.push_back(key) ;
    return res ;
}

    // Thread-safe lookup for a specific translator
std::shared_ptr<Translator> TranslationManager::getTranslator(const std::string& requested_locale) {
    std::shared_lock<std::shared_mutex> lock(mutex_);
        
    // 1. Match exact requested locale (e.g., "en_US")
    auto it = translators_.find(requested_locale);
    if ( it != translators_.end() ) return it->second;

    // 2. Loose fallback fallback: Match language prefix (e.g., "en" from "en_GB")
    if ( requested_locale.length() >= 2 ) {
        std::string lang_prefix = requested_locale.substr(0, 2);
        for (const auto& [name, translator] : translators_) {
            if ( name.substr(0, 2) == lang_prefix ) return translator;
        }
    }

    auto fallback_it = translators_.find(default_locale_);
    if ( fallback_it != translators_.end() ) return fallback_it->second;

        // empty translator
    return std::make_shared<Translator>(default_locale_);
}


}