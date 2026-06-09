#include <twig/forms/form_builder.hpp>
#include <twig/translator.hpp>

using namespace std ;

namespace twig {

Variant FormField::normalize(const std::string &src) {
    Variant norm_value = src ;
    
    for( auto n: normalizers_ ) {
        norm_value = n(norm_value) ;
    }

    return norm_value ;
}

void FormField::render(Variant::Object &data, twig::TranslationManager *trans, const std::string &locale) const {
    data["name"] = name_ ;
    if ( !label_.key_.empty() ) {
        if ( trans )
            data["label"] = trans->translate(label_, locale) ;
        else
            data["label"] = label_.key_ ;
    }
    data["id"] = id_ ;
    Variant::Object attrs ;
    for( const auto &[key, val]: attrs_ )
        attrs.emplace(key, val) ;
    data["attr"] = attrs ;
    data["value"] = value_ ;

    Variant::Array classes ;
    if ( !is_valid_ ) {
         if ( trans )
            data["error"] = trans->translate(error_msg_, locale) ;
        else
            data["error"] = error_msg_.key_ ;
    }

     for( const auto &c: css_class_ )
        classes.push_back(c) ;
    data["cls"] = classes ;
    data["widget"] = widget_ ;
    if ( !group_.empty() ) data["group"] = group_ ;
}

bool FormField::validate(const Variant &value) {
    for( auto v: validators_ ) {
        auto res = v(value, *this) ;
        if (!res()) {
            error_msg_ = res.error_msg_ ;
            return false ;
        }
    }
    return true ;
}


bool FormField::process(const std::string &v) {
    error_msg_.key_.clear() ;
    Variant nrm = normalize(v) ;
    is_valid_ =  validate(nrm) ;
    if ( is_valid_ )
        value_ = nrm ;
    
    return is_valid_ ;
}

void FormField::reset() {
    value_ = default_ ;
}

std::string FormField::interpolateMessage(const std::string &msg_template, const Variant &value, const dictionary_t &params) const {

    string res, param ;
    bool in_param = false ;
    string::const_iterator cursor = msg_template.begin(), end = msg_template.end() ;

    while ( cursor != end ) {
        char c = *cursor++ ;
        if ( c == '{' ) {
            in_param = true ;
            param.clear() ;
        }
        else if ( c == '}' ) {
            in_param = false ;

            if ( param.empty() ) ;
            else if ( param == "field" ) {
                res.append(getName()) ;
            } else if ( param == "value" ) {
                res.append(value.toString()) ;
            }
            else {
                auto it = params.find(param) ;
                
                if ( it != params.end() )
                    res.append(it->second) ;
            }
        }
        else if ( c == '\\' && !in_param) {
            char c = *cursor ;
            switch (c) {
                case '{':
                case '}':
                case '\\':
                    res.push_back(c) ;
                    break ;
                default:
                    res.push_back('\\') ;
                    res.push_back(c) ;
            }
        }
        else if ( in_param ) {
            param.push_back(c) ;
        }
        else res.push_back(c) ;
    }

    return res ;
}

bool Form::process(const dictionary_t &data) {
    if ( csrf_enabled_) {
        auto it = data.find("csrf_token");
        if ( it == data.end() || it->second != csrf_token_ ) {
            return false; 
        }
    }

    bool valid = true;
        
    // check each field and validate it, if any field is invalid then the form is invalid
    for ( auto &[field_name, field] : fields_ ) {
        std::string submission_value = "";  
        
        // Check if field was actually submitted, else treat as blank
        if ( data.find(field_name) != data.end()) {
            submission_value = data.at(field_name);
        }
            
        if ( !field->process(submission_value)) {
                valid = false;
        }
    }

    is_valid_ = valid && validateForm(data) ;
    return is_valid_ ;
}

bool Form::validateForm(const dictionary_t &data) {
    for( auto &v: validators_ ) {
        auto res = v(data, *this) ;
        if (!res()) {
            global_error_msg_ = res.error_msg_ ;
            return false ;
        }
    }
    return true ;
}

Variant::Object Form::render(twig::TranslationManager *tr, const std::string &locale) const {
    Variant::Object ctx ;
    
    ctx["action"] = action_ ;
    ctx["method"] = method_ ;

    if ( csrf_enabled_ ) {
        ctx["_csrf_token"] = csrf_token_ ;
    }

    if ( !is_valid_ && !global_error_msg_.key_.empty() ) {
        if ( tr != nullptr)
            ctx["error"] = tr->translate(global_error_msg_, locale)  ;
        else
            ctx["error"] = global_error_msg_.key_ ;
    }
    
    Variant::Object fields, groups ;
    
    for (const auto& [name,  field]: fields_) {
        Variant::Object field_data ;
        field->render(field_data, tr, locale) ;
        fields.emplace(name, field_data);
    }
    ctx["fields"] = fields ;

    for (const auto& [name,  label]: groups_) {
        groups.emplace(name, label);
    }
    ctx["groups"] = groups ;

    return ctx;
}

void Form::reset() {
    global_error_msg_.key_.clear() ;
    for (const auto& [name,  field]: fields_) {
        field->reset() ;
    }
}
}