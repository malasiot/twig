#pragma once

#include <regex>
#include <stdexcept>
#include <functional>
#include <map>

#include <twig/forms/form_builder.hpp>

namespace twig {

class FormFieldValidators {
public:

    static FormFieldValidator isBoolean(const twig::Translatable &msg = {}) ;
    static FormFieldValidator isInteger(const twig::Translatable &msg = {}) ; 
    static FormFieldValidator required(const twig::Translatable &msg = {}) ; 
    static FormFieldValidator atLeastNChars(size_t min_len, const twig::Translatable &msg = {}) ;
    static FormFieldValidator atMostNChars(size_t max_len, const twig::Translatable &msg = {}) ;
    static FormFieldValidator matches(const std::regex &rx, const twig::Translatable &msg = {}) ;
    static FormFieldValidator isOneOf(const std::vector<std::string> &choices, const twig::Translatable &msg = {}) ;
    static FormFieldValidator isNoneOf(const std::vector<std::string> &choices, const twig::Translatable &msg = {}) ;
    static FormFieldValidator inRange(int min_val, int max_val, const twig::Translatable &msg = {}) ;
};

}

