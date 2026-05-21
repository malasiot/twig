#ifndef TWIG_FUNCTIONS_HPP__
#define TWIG_FUNCTIONS_HPP__

#include <string>
#include <functional>

#include <twig/variant.hpp>
#include <twig/context.hpp>

namespace twig {


using TemplateFunction = std::function<Variant(const Variant &, Context &ctx)>;

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
    bool hasFilter(const std::string &name) ;
    bool hasTest(const std::string &name) ;

    Variant invokeFunction(const std::string &name, const Variant &args, Context &ctx) ;
    Variant invokeFilter(const std::string &name, const Variant &args, Context &ctx) ;
    Variant invokeTest(const std::string &name, const Variant &args, Context &ctx) ;

    void registerFunction(const std::string &name, const TemplateFunction &f);
    void registerFilter(const std::string &name, const TemplateFunction &f);
    void registerTest(const std::string &name, const TemplateFunction &f);

private:

    std::map<std::string, TemplateFunction> functions_, filters_, tests_ ;
};

}

#endif
