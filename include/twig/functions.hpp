#ifndef TWIG_FUNCTIONS_HPP__
#define TWIG_FUNCTIONS_HPP__

#include <string>
#include <functional>

#include <variant/variant.hpp>
#include <twig/context.hpp>

namespace twig {

using TemplateFunction = std::function<Variant(const Variant &, Context &ctx)>;
using TestFunction = std::function<bool(const Variant &, const Variant &, Context &ctx)>;
using FilterFunction = std::function<Variant(const Variant &, const Variant &, Context &ctx)>;

void unpack_args(const Variant &args, const std::vector<std::string> &spec, Variant::Array &res) ;

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
    Variant invokeFilter(const std::string &name, const Variant &target, const Variant &args, Context &ctx) ;
    Variant invokeTest(const std::string &name, const Variant &target, const Variant &args, Context &ctx) ;

    void registerFunction(const std::string &name, const TemplateFunction &f);
    void registerFilter(const std::string &name, const FilterFunction &f);
    void registerTest(const std::string &name, const TestFunction &f);

private:

    std::map<std::string, TemplateFunction> functions_ ;
    std::map<std::string, FilterFunction> filters_ ;
    std::map<std::string, TestFunction> tests_ ;
};

}

#endif
