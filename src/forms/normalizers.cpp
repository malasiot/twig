#include <twig/forms/normalizers.hpp>

using namespace std ;

namespace twig {

Variant FormFieldNormalizers::toBoolean(const Variant &val) {
    if ( val.isBoolean() )
        return val ;
    if ( val.isString() ) {
        auto s = val.toString() ;
        if ( s == "true" || s == "1" || s == "yes" || s == "on" )
            return true ;
        else if ( s == "false" || s == "0" || s == "no" || s == "off" )
            return false ;
    }
    return val ;
}

Variant FormFieldNormalizers::toInteger(const Variant &val) {
    if ( val.isNumber() )
        return val ;
    if ( val.isString() ) {
        try {
            return (int64_t)std::stoll(val.toString()) ;
        } catch ( const std::exception & ) {
            return val ;
        }
    }
    return val ;
}

Variant FormFieldNormalizers::trim(const Variant &val) {
    if ( val.isString() ) {
        string s = val.toString() ;
        s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
            return s ;
    }
    return val ;
}

}