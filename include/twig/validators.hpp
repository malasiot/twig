#pragma once

#include <regex>
#include <stdexcept>
#include <functional>
#include <map>

#include <twig/form_helper.hpp>

namespace twig {

class FormFieldValidators {
public:

    static FormFieldValidator isBoolean(const std::string &msg = {}) ;
    static FormFieldValidator isInteger(const std::string &msg = {}) ; 
    static FormFieldValidator required(const std::string &msg = {}) ; 
    static FormFieldValidator atLeastNChars(size_t min_len, const std::string &msg = {}) ;
    static FormFieldValidator atMostNChars(size_t max_len, const std::string &msg = {}) ;
    static FormFieldValidator matches(const std::regex &rx, const std::string &msg = {}) ;
    static FormFieldValidator isOneOf(const std::vector<std::string> &choices, const std::string &msg = {}) ;
    static FormFieldValidator isNoneOf(const std::vector<std::string> &choices, const std::string &msg = {}) ;
    static FormFieldValidator inRange(int min_val, int max_val, const std::string &msg = {}) ;
};

#if 0
class FormFieldValidator {
public:
    // Override this to perform custom validation of passed value. On error through a FormFieldValidationError
    virtual void validate(const std::string &val, const FormField &field) const = 0 ;

    // the function is used to interpolate a template string with variables contained in params and implicitly declared variables
    // such as {field} and {value} obtained from the field and passed value respectively
    static std::string interpolateMessage(const std::string &msg_template, const std::string &value, const FormField &field, const Dictionary &params= Dictionary()) ;

    virtual ~FormFieldValidator() {}
};

// Helper for defining a lambda validator
typedef std::function<void (const std::string &, const FormField &)> ValidatorCallback ;

class CallbackValidator: public FormFieldValidator {
public:

    CallbackValidator(ValidatorCallback v): cb_(v) {}

    virtual void validate(const std::string &val, const FormField &field) const {
        cb_(val, field) ;
    }
private:
    ValidatorCallback cb_ ;
};

#endif


#if 0

class SelectionValidator: public FormFieldValidator {
public:

    SelectionValidator(const std::vector<std::string> &keys, const std::string &msg = std::string()): keys_(keys), msg_(msg) {}

    virtual void validate(const std::string &val, const FormField &field) const override ;
protected:
    std::vector<std::string> keys_ ;
    std::string msg_ ;
private:
    const static std::string validation_msg_ ;
};

class UploadedFileValidator: public FormFieldValidator {
public:
    UploadedFileValidator(const std::map<std::string, wspp::server::Request::UploadedFile> &files,
                          size_t max_file_size, const std::vector<std::string> &allowed_types = {},
                          const std::string &max_file_msg = std::string(),
                          const std::string &allowed_mime_msg = std::string()):
        files_(files), max_file_size_(max_file_size), mime_types_(allowed_types), max_size_msg_(max_file_msg),
    allowed_mime_msg_(allowed_mime_msg){}

    void validate(const std::string &val, const FormField &field) const override ;
protected:

    const std::map<std::string, wspp::server::Request::UploadedFile> &files_ ;
    std::string max_size_msg_, allowed_mime_msg_ ;
    size_t max_file_size_ = 20 * 1024 * 1024;
    std::vector<std::string> mime_types_ ;
private:
    const static std::string max_size_validation_msg_ ;
    const static std::string wrong_mime_validation_msg_ ;
};
#endif
}

