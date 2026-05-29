#pragma once

#include <regex>
#include <stdexcept>
#include <functional>

namespace twig {

    using Dictionary = std::map<std::string, std::string> ;
// exception thrown by validators
class FormFieldValidationError: public std::runtime_error {
public:

    FormFieldValidationError(const std::string &msg): std::runtime_error(msg) {}
};

class FormField ;

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

// checks if the field value is empty

class NonEmptyValidator: public FormFieldValidator {
public:

    NonEmptyValidator(const std::string &msg = std::string()):  msg_(msg) {}

    virtual void validate(const std::string &val, const FormField &field) const override ;
protected:
    std::string msg_ ;
private:
    const static std::string validation_msg_ ;
};

// check minimum characters

class MinLengthValidator: public FormFieldValidator {
public:
    MinLengthValidator(uint min_length, const std::string &msg = std::string()): min_len_(min_length), msg_(msg) {}

    virtual void validate(const std::string &val, const FormField &field) const override ;
protected:
    std::string msg_ ;
    uint min_len_ ;
private:
    const static std::string validation_msg_ ;
};

// check maximum characters

class MaxLengthValidator: public FormFieldValidator {
public:
    MaxLengthValidator(uint max_length, const std::string &msg = std::string()): max_len_(max_length), msg_(msg) {}

    virtual void validate(const std::string &val, const FormField &field) const override ;
protected:
    std::string msg_ ;
    uint max_len_ ;
private:
    const static std::string validation_msg_ ;
};

// Validate against regular expression. If negative is set to true it will only validate if the value does not match the expression

class RegexValidator: public FormFieldValidator {
public:

    RegexValidator(const std::regex &rx, const std::string &msg, bool negative = false): rx_(rx), msg_(msg), is_negative_(false) {}

    virtual void validate(const std::string &val, const FormField &field) const override ;
protected:
    std::regex rx_ ;
    std::string msg_ ;
    bool is_negative_ ;
private:
    const static std::string validation_msg_ ;
};

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
#if 0
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

