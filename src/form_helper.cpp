#include <twig/form_helper.hpp>

using namespace std ;

namespace twig {

void FormField::fillData(Variant::Object &res) const {

    if ( !name_.empty() ) res.insert({"name", name_}) ;

    if ( !value_.empty() ) res.insert({"value", value_}) ;
    else if ( !default_value_.empty() ) res.insert({"value", default_value_}) ;

    if ( !errors_.empty() ) res.insert({"errors", Variant::fromVector(errors_)}) ;
}

bool FormField::validate(const string &value)
{
    // normalize passed value

    
    string n_value = value ;
    
    for( auto n: normalizers_ ) {
        n_value = n(n_value) ;
    }

    // call validators

    for( auto &v: validators_ ) {
        try {
            v->validate(n_value, *this) ;
        }
        catch ( FormFieldValidationError &e ) {
            addErrorMsg(e.what());
            return false ;
        }
    }

    // store value

    value_ = n_value ;
    is_valid_ = true ;
    return true ;
}


}