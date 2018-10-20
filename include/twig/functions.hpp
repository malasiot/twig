#ifndef TWIG_FUNCTIONS_HPP__
#define TWIG_FUNCTIONS_HPP__

#include <string>
#include <functional>

#include <twig/variant.hpp>
#include <twig/context.hpp>

namespace twig {


using TemplateFunction = std::function<Variant(const Variant &)>;

// Unpack positional and named arguments passed to the function to the list of expected arguments
// The named_args is a list of arguments names. If ending with '?' argument is optional. Non supplied arguments are given undefined value.
// Throws TemplateRuntimeException if not all required arguments are provided

void unpack_args(const Variant &args, const std::vector<std::string> &named_args, Variant::Array &res) ;


class FunctionFactory {
public:

    FunctionFactory() ;

    static FunctionFactory &instance() {
        static FunctionFactory s_instance ;
        return s_instance ;
    }

    bool hasFunction(const std::string &name) ;

    Variant invoke(const std::string &name, const Variant &args) ;

    void registerFunction(const std::string &name, const TemplateFunction &f);

private:

    std::map<std::string, TemplateFunction> functions_ ;
};

}

#endif
