#ifndef TWIG_EXCEPTIONS_HPP__
#define TWIG_EXCEPTIONS_HPP__

#include <stdexcept>
#include <string>

namespace twig {


class TemplateException: public std::runtime_error {
public:

    TemplateException(const std::string &msg): std::runtime_error(msg) {}
};


class TemplateCompileException: public TemplateException {
public:
    TemplateCompileException(const std::string &msg): TemplateException(msg) {}
};

class TemplateLoadException: public TemplateException {
public:
    TemplateLoadException(const std::string &msg): TemplateException(msg) {}
};

class TemplateRuntimeException: public TemplateException {
public:
    TemplateRuntimeException(const std::string &msg): TemplateException(msg) {}
};

}
#endif
