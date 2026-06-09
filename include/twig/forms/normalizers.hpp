#pragma once

#include <regex>
#include <stdexcept>
#include <functional>
#include <map>

#include <twig/forms/form_builder.hpp>

namespace twig {

class FormFieldNormalizers {
public:

    static Variant toBoolean(const Variant &val) ;
    static Variant toInteger(const Variant &val) ; 
    static Variant trim(const Variant &val) ; 
};

}

