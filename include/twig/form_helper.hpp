#pragma once

#include <twig/variant.hpp>

#include <memory>
#include <functional>
#include <regex>
#include <map>
#include <set>

using dictionary_t = std::map<std::string, std::string> ;

namespace twig {

 // Form helper class. The aim of the class is:
// 1) declare form in a view agnostic way
// 2) validate input data for those fields e.g passed in a POST request via validators
// 3) controller for server side form handling
class FormField ;
struct ValidationResult {
    bool is_valid_;
    std::string error_msg_ ;

    bool operator()() const { return is_valid_ ; }

    // Static helper factories for clean return expressions
    static ValidationResult success() {
        return {true, {}};
    }

    static ValidationResult failure(const std::string &msg) {
        return {false, std::move(msg)};
    }
};

using FormFieldValidator = std::function<ValidationResult (const Variant &, const FormField &)> ;
using FormFieldNormalizer = std::function<Variant (const Variant &)> ;

class FormField {
public:

    // The normalizer preprocesses an input value before passing it to validators
   

    typedef std::shared_ptr<FormField> Ptr ;

public:

    FormField(const std::string &name): name_(name), id_(name) {}

    // label
    FormField &label(const std::string &val) { label_ = val ; return *this ; }
    // set field value
    FormField &value(const Variant &val) { value_ = val ; return *this ; }

    FormField &id(const std::string &val) { id_ = val ; return *this ; }

    FormField &addClass(const std::vector<std::string> &classes) {
         for (const auto& cls : classes) {
            if (!cls.empty()) {
                css_class_.insert(cls);
            }
        }
        return *this;
    }
    FormField &addClass(const std::string &cls) {
         if (!cls.empty()) {
                css_class_.insert(cls);
         }
         return *this ;
    }

    // add validator
    FormField &addValidator(FormFieldValidator val) { 
        validators_.push_back(val) ; 
        return *this ; 
    }
  
    FormField &addNormalizer(FormFieldNormalizer val) { normalizers_.push_back(val) ; return *this ; }

    FormField &attrs(const Variant::Object &v) { attrs_ = v ; return *this; }
    FormField &attribute(const std::string &key, const Variant &val) { attrs_.emplace(key, val) ; return *this ; }
    FormField &group(const std::string &v) { group_ = v ; return *this ; }

    bool valid() const { return is_valid_ ; }

    Variant getValue() const { return value_ ; }
    std::string getName() const { return name_ ; }

    bool hasErrors() { return !is_valid_ ; }
   
    virtual bool process(const std::string &val) ;

    std::string interpolateMessage(const std::string &msg_template, const Variant &value, const dictionary_t &params = {}) const;

protected:

    friend class Form ;

    virtual void render(Variant::Object &data) const ;

    virtual Variant normalize(const std::string &val) ;
    virtual bool validate(const Variant &value) ;

    std::string key_, name_, label_, widget_, id_, group_ ;
    Variant value_ ;
    Variant::Object attrs_ ;
    std::set<std::string> css_class_ ;

    std::string error_msg_ ;
    std::vector<FormFieldValidator> validators_ ;
    std::vector<FormFieldNormalizer> normalizers_ ;

    bool is_valid_ = false ;
};

class Form ;
using FormValidator = std::function<ValidationResult (const dictionary_t &, const Form &)> ;

class Form {
public:
    Form(const std::string &action = "#", const std::string &method = "POST"): 
        action_(action), method_(method) {}

    template <typename T, typename... Args>
    T& field(Args&&... args) {
        static_assert(std::is_base_of_v<FormField, T>, "T must inherit from FormField");
        auto new_field = std::make_unique<T>(std::forward<Args>(args)...);
        T& field_ref = *new_field; 
        fields_.emplace(new_field->name_, std::move(new_field));
        return field_ref;
    }

    template <typename T>
    T* getField(const std::string &name) {
        auto it = fields_.find(name) ;
        if ( it == fields_.end() ) return nullptr ;
        return dynamic_cast<T*>(*(it->second)) ;
    }

    Form &group(const std::string &name, const std::string &label) { groups_[name] = label ; return *this ; }

    Form &addValidator(FormValidator val) { validators_.push_back(val) ; return *this ; }

    Form &enableCSRF(const std::string &token) { csrf_enabled_ = true ; csrf_token_ = token ; return *this ; }

    virtual bool process(const dictionary_t &data) ;
    virtual Variant::Object render() const ;

private:

    bool validateForm(const dictionary_t &data) ;

    std::unordered_map<std::string, std::unique_ptr<FormField>> fields_ ;
    std::string method_, enc_type_, action_, global_error_msg_ ;
    std::vector<FormValidator> validators_ ;
    std::string csrf_token_;
    dictionary_t groups_ ;
    bool is_valid_ = false, csrf_enabled_ = false ;
};


class InputField: public FormField {
public:
    InputField(const std::string &key, const std::string &type): FormField(key), type_(type) {
        widget_ = "input" ;
    }
protected:
    virtual void render(Variant::Object &data) const override {
        FormField::render(data) ;
        data["type"] = type_ ;
    }
    std::string type_ ;
};

class TextField: public InputField {
    public:

    TextField(const std::string &name): InputField(name, "text") {}
};

class EmailField: public InputField {
    public:

    EmailField(const std::string &name): InputField(name, "email") {}
};

class PasswordField: public InputField {
    public:

    PasswordField(const std::string &name): InputField(name, "password") {}
};

class NumberField: public InputField {
    public:

    NumberField(const std::string &name): InputField(name, "number") {}
};

class TextAreaField: public InputField {
    public:

    TextAreaField(const std::string &name): InputField(name, "textarea") {}
};

class CheckBoxField: public InputField {
    public:

    CheckBoxField(const std::string &name): InputField(name, "checkbox") {}

    protected:

    void render(Variant::Object &data) const override {
        InputField::render(data) ;
        data["checked"] = value_.toBoolean(); 
    }
};


class SelectField: public FormField {
public:
    SelectField(const std::string &name, const dictionary_t &options = {}): 
        FormField(name), options_(options) { widget_ = "select" ; }

    SelectField &addOption(const std::string &key, const std::string &label) {
        options_.emplace(key, label) ;
        return *this ;
    }
protected:
    void render(Variant::Object &data) const override {
        FormField::render(data) ;
        
        Variant::Array options ;

        for (const auto& [val, lbl] : options_) {
            options.push_back(Variant::Object{
                {"value", val},
                {"label", lbl},
                {"selected", (val == value_.toString())}
            } 
            );
        }

        data["options"] = options ;
    }

private:
    dictionary_t options_ ;
};

class RadioField: public FormField {
public:
    RadioField(const std::string &name, const dictionary_t &options = {}): 
        FormField(name), options_(options) { widget_ = "radio" ; }

    RadioField &addOption(const std::string &key, const std::string &label) {
        options_.emplace(key, label) ;
        return *this ;
    }
protected:
    void render(Variant::Object &data) const override {
        FormField::render(data) ;
        
        Variant::Array options ;

        for (const auto& [val, lbl] : options_) {
            options.push_back(Variant::Object{
                {"value", val},
                {"label", lbl},
                {"checked", (val == value_.toString())}
            } 
            );
        }

        data["options"] = options ;
    }

private:
    dictionary_t options_ ;
};

class FileUploadField: public FormField {
public:
    FileUploadField(const std::string &name);

    FileUploadField &maxFileSize(uint64_t sz) { max_file_size_ = sz ; return *this ; }
    FileUploadField &accept(const std::string &a) { accept_ = a ; return *this ; }

private:
    uint64_t max_file_size_ = 0 ;
    std::string accept_ ;
};


}