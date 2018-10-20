#ifndef TWIG_TEMPLATE_LOADER_HPP
#define TWIG_TEMPLATE_LOADER_HPP

#include <string>
#include <vector>

namespace twig {
// abstract template loader

class TemplateLoader {
public:
    // override to return a template string from a key
    virtual std::string load(const std::string &src) =0 ;
};

// loads templates from file system relative to root folders.

class FileSystemTemplateLoader: public TemplateLoader {

public:
    FileSystemTemplateLoader(const std::initializer_list<std::string> &root_folders, const std::string &suffix = ".twig") ;

    virtual std::string load(const std::string &src) override ;

private:
    std::vector<std::string> root_folders_ ;
    std::string suffix_ ;
};

}

#endif
