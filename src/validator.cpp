#include <twig/validator.hpp>
#include <twig/form_helper.hpp>

using namespace std l;
namespace twig {

std::string FormFieldValidator::interpolateMessage(const std::string &msg_template, const std::string &value, const FormField &field, const Dictionary &params) {

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
                res.append(field.getAlias()) ;
            } else if ( param == "value" ) {
                res.append(value) ;
            }
            else {
                string p = params.get(param) ;
                if ( !p.empty() )
                    res.append(p) ;
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



const std::string NonEmptyValidator::validation_msg_ = "{field} cannot be empty";

void NonEmptyValidator::validate(const std::string &val, const FormField &field) const {
    if ( val.empty() ) {
        throw FormFieldValidationError( FormFieldValidator::interpolateMessage(msg_.empty() ? validation_msg_ : msg_, val, field )) ;
    }
}

const std::string MinLengthValidator::validation_msg_ = "{field} should contain at least {min} characters";

void MinLengthValidator::validate(const std::string &val, const FormField &field) const
{
    size_t len = val.length() ;

    if ( len < min_len_ ) {
        string cmsg = std::to_string(len)  ;
        throw FormFieldValidationError( FormFieldValidator::interpolateMessage(msg_.empty() ? validation_msg_ : msg_, val, field, {{"min",  cmsg}} )) ;
    }

}

const std::string MaxLengthValidator::validation_msg_ = "{field} should contain less than {max} characters";

void MaxLengthValidator::validate(const std::string &val, const FormField &field) const
{
    size_t len = val.length() ;

    if ( len > max_len_ ) {
        string cmsg = std::to_string(len)  ;
        throw FormFieldValidationError( FormFieldValidator::interpolateMessage(msg_.empty() ? validation_msg_ : msg_, val, field, {{"max",  cmsg}} )) ;
    }
}

void RegexValidator::validate(const std::string &val, const FormField &field) const
{
    if ( !is_negative_ && !boost::regex_match(val, rx_) ) {
        throw FormFieldValidationError( FormFieldValidator::interpolateMessage(msg_, val, field) ) ;
    }
}

void SelectionValidator::validate(const std::string &val, const FormField &field) const
{
    if ( std::find(keys_.begin(), keys_.end(), val) == keys_.end() )
        throw FormFieldValidationError( FormFieldValidator::interpolateMessage(msg_, val, field) ) ;
}

const std::string UploadedFileValidator::max_size_validation_msg_ = "The file is too large ({size}). Allowed maximum size is {limit}.";
const std::string UploadedFileValidator::wrong_mime_validation_msg_ = "The file mime type ({mime}) is invalid.";

static string bytesToString(size_t ibytes)
{
    string suffix[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB" };

    int order = 0;
    double bytes = ibytes ;
    while ( bytes >= 1024 && order < sizeof(suffix)/sizeof(string) - 1) {
        order++;
        bytes = bytes/1024;
    }
    return str(boost::format("%.1lf %s") % bytes % suffix[order]);
}
void UploadedFileValidator::validate(const std::string &val, const FormField &field) const
{
    auto it = files_.find(val) ;
    if ( it == files_.end() )
        throw FormFieldValidationError("No file received") ;

    const Request::UploadedFile &file = it->second ;
    if ( file.size_ > max_file_size_ )
        throw FormFieldValidationError( FormFieldValidator::interpolateMessage(max_size_msg_.empty() ? max_size_validation_msg_ : max_size_msg_, val, field,
            {{"size", bytesToString(file.size_)}, {"limit", bytesToString(max_file_size_) }}) ) ;

    if ( mime_types_.empty() ) return ;

    for ( const auto &mime: mime_types_ ) {
        if ( mime == file.mime_ ) return ;
    }

    throw FormFieldValidationError( FormFieldValidator::interpolateMessage(allowed_mime_msg_.empty() ? wrong_mime_validation_msg_ : allowed_mime_msg_, val, field,
        {{"mime", file.mime_}}) ) ;
}


}
