#include <twig/validators.hpp>
#include <twig/form_helper.hpp>
#include <iostream>
using namespace std;
namespace twig {


template<typename Predicate>
FormFieldValidator makeValidator(
    Predicate pred, // error condition predicate: should return true if the value is invalid
    const std::string &customMsg,
    const std::string &defaultMsg,
    std::map<std::string, std::string> params = {})
{
    return [pred, customMsg, defaultMsg, params](const Variant &val, const FormField &field) {
        if (!pred(val))
            return ValidationResult::success();
        return ValidationResult::failure(
            customMsg.empty() ? field.interpolateMessage(defaultMsg, val, params) : customMsg
        );
    };
}

FormFieldValidator FormFieldValidators::isBoolean(const std::string &msg) {
    return makeValidator(
        [](const Variant &val) { return !val.isBoolean(); },
        msg,
        "Field {field} must be a boolean value"
    );
}

FormFieldValidator FormFieldValidators::isInteger(const std::string &msg) {
    return makeValidator(
        [](const Variant &val) { return !val.isInteger(); },
        msg,
        "Field {field} must be an integer value"
    );
}

FormFieldValidator FormFieldValidators::required(const std::string &msg) {
    return makeValidator(
        [](const Variant &val) { return val.isString() && val.toString().empty(); },
        msg,
        "Field {field} cannot be empty"
    );
}

FormFieldValidator FormFieldValidators::atLeastNChars(size_t min_len, const std::string &msg) {
    return makeValidator(
        [min_len](const Variant &val) { return !val.isString() || val.toString().size() < min_len; },
        msg,
        "Field {field} should have at least {n} characters",
        {{"n", std::to_string(min_len)}}
    );
}

FormFieldValidator FormFieldValidators::atMostNChars(size_t max_len, const std::string &msg) {
    return makeValidator(
        [max_len](const Variant &val) { return !val.isString() || val.toString().size() > max_len; },
        msg,
        "Field {field} should have no more than {n} characters",
        {{"n", std::to_string(max_len)}}
    );
}

FormFieldValidator FormFieldValidators::matches(const std::regex &rx, const std::string &msg) {
    return makeValidator(
        [rx](const Variant &val) { return !val.isString() || !std::regex_match(val.toString(), rx); },
        msg,
        "Field {field} does not match expression"
    );
}

FormFieldValidator FormFieldValidators::isOneOf(const std::vector<std::string> &keys, const std::string &msg) {
    return makeValidator(
        [keys](const Variant &val) { return std::find(keys.begin(), keys.end(), val.toString()) == keys.end(); },
        msg,
        "Field {field} value is not in the set of allowable values"
    );
}

FormFieldValidator FormFieldValidators::isNoneOf(const std::vector<std::string> &keys, const std::string &msg) {
    return makeValidator(
        [keys](const Variant &val) { return std::find(keys.begin(), keys.end(), val.toString()) != keys.end(); },
        msg,
        "Field {field} value is in the set of disallowed values"
    );
}

FormFieldValidator FormFieldValidators::inRange(int min_val, int max_val, const std::string &msg) {
    return makeValidator(
        [min_val, max_val](const Variant &val) { 
            try {
                int num = val.toInteger() ;
                return num < min_val || num > max_val ;
            } catch ( std::exception &e ) {
                return true ;
            }
        },
        msg,
        "Field {field} value should be between {min} and {max}",
        {{"min", std::to_string(min_val)}, {"max", std::to_string(max_val)}}
    );
}

#if 0
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
#endif

}
