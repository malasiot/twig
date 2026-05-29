#pragma once

#include <twig/variant.hpp>
#include <twig/validator.hpp>

#include <memory>
#include <functional>

namespace twig {

 // Form helper class. The aim of the class is:
// 1) declare form in a view agnostic way
// 2) validate input data for those fields e.g passed in a POST request via validators
// 3) controller for server side form handling

class FormField {
public:

    // The normalizer preprocesses an input value before passing it to validators
    typedef std::function<std::string (const std::string &)> Normalizer ;

    typedef std::shared_ptr<FormField> Ptr ;

public:

    FormField(const std::string &name): name_(name) {}

    // label
    FormField &label(const std::string &val) { label_ = val ; return *this ; }
    // set field value
    FormField &value(const std::string &val) { value_ = val ; return *this ; }

    FormField &id(const std::string &val) { id_ = val ; return *this ; }

    // the validator checks the validity of the input string.
    // if invalid then it should throw a FormFieldValidationError exception

    // add validator
    FormField &addValidator(FormFieldValidator *val) { validators_.emplace_back(val) ; return *this ; }
    // add custom validator as lambda
   // FormField &addValidator(ValidatorCallback val) { addValidator<CallbackValidator>(val) ;  return *this ; }
    // validator helper
    template <class T, typename ... Args>
    FormField &addValidator(Args&&...args) {
        validators_.emplace_back(std::unique_ptr<T>(new T(std::forward<Args>(args)...))); return *this ;
    }

    // set custom normalizer
    FormField &addNormalizer(Normalizer val) { normalizers_.push_back(val) ; return *this ; }

    FormField &defaultValue(const std::string &v) { default_value_ = v ; return *this ; }

    FormField &alias(const std::string &v) { alias_ = v ; return *this ; }

    FormField &attrs(const std::map<std::string, std::string> &v) { attrs_ = v ; return *this; }

    bool valid() const { return is_valid_ ; }

    void addErrorMsg(const std::string &msg) { errors_.emplace_back(msg) ; }

    std::string getValue() const { return value_ ; }

    std::string getAlias() const { return alias_ ; }

    std::string getName() const { return name_ ; }
   

protected:
    virtual void fillData(Variant::Object &) const ;

    virtual bool validate(const std::string &value) ;
    virtual bool process(const Variant::Object &data) ;
    virtual Variant render() ;

    std::string name_, value_, default_value_, alias_, label_, widget_, id_ ;
    std::map<std::string, std::string> attrs_ ;

    std::vector<std::string> errors_ ;
    std::vector<std::unique_ptr<FormFieldValidator>> validators_ ;
    std::vector<Normalizer> normalizers_ ;
    bool is_valid_ = false ;
};

class OptionsModel {
public:
    using Ptr = std::shared_ptr<OptionsModel> ;

    virtual std::map<std::string, std::string> fetch() = 0 ;
};

// convenience class for wrapping a dictionary

class DictionaryOptionsModel: public OptionsModel {
public:
    DictionaryOptionsModel(const std::map<std::string, std::string> &dict): dict_(dict) {}
    virtual std::map<std::string, std::string> fetch() override {
        return dict_ ;
    }

private:
    std::map<std::string, std::string> dict_ ;
};


class InputField: public FormField {
public:
    InputField(const std::string &name, const std::string &type): FormField(name), type_(type) {}

    void fillData(Variant::Object &) const override;
private:
    std::string type_ ;
};

class FileUploadField: public FormField {
public:
    FileUploadField(const std::string &name);

    FileUploadField &maxFileSize(uint64_t sz) { max_file_size_ = sz ; return *this ; }
    FileUploadField &accept(const std::string &a) { accept_ = a ; return *this ; }

    void fillData(Variant::Object &) const override;
private:
    uint64_t max_file_size_ = 0 ;
    std::string accept_ ;
};

class SelectField: public FormField {
public:
    SelectField(const std::string &name, std::shared_ptr<OptionsModel> options, bool multi = false);
    SelectField(const std::string &name, const std::map<std::string, std::string> &options, bool multi = false);

    void fillData(Variant::Object &) const override;

private:
    bool multiple_ ;
    std::shared_ptr<OptionsModel> options_ ;
};

class CheckBoxField: public FormField {
public:
    CheckBoxField(const std::string &name, bool is_checked = false);

    void fillData(Variant::Object &res) const override;
private:
    bool is_checked_ = false ;
};

#if 0
class FormHandler {
public:

    FormHandler() ;

    // helper for fluent field creation
    template<typename ... Args >
    FormField &field(Args... args) {
        auto f = std::make_shared<FormField>(args...) ;
        addField(f) ;
        return *f ;
    }

    // call to validate the user data against the form
    // the field values are stored in case of succesfull field validation
    // override to add additional validation e.g. requiring more than one fields (do not forget to call base class)
    virtual bool validate(const Request &vals) ;

    // init form with user supplied values (no validation)
    void init(const Dictionary &vals) ;

    // get form view to pass to template render
    Variant::Object view() const ;

    // get errors object
    Variant::Object errors() const ;

    // get value stored in a field
    string getValue(const string &field_name) ;

    // override to perform special processing after a succesfull form validation (e.g. persistance)
    virtual void onSuccess(const Request &request) {}
    // override to perform special processing when a GET request is handled (usually init form from persistance)
    virtual void onGet(const Request &request) {}

    // Use this to avoid boilerplate in form request handling
    // When a POST request is recieved and succesfully validated then onSuccess function is called
    // When a GET request is received for initial rendering of the form then the onGet handler is called to initialize
    // the form
    void handle(const Request &req, server::Response &response, twig::TemplateRenderer &engine) ;

protected:

    std::vector<FormField::Ptr> fields_ ;
    std::map<string, FormField::Ptr> field_map_ ;

    std::vector<string> errors_ ;
    bool is_valid_ = false ;

    void addField(const FormField::Ptr &field);
} ;

#endif

}