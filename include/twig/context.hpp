#ifndef TWIG_CONTEXT_HPP
#define TWIG_CONTEXT_HPP

#include <memory>

#include "variant.hpp"

namespace twig {
namespace detail {
class NamedBlockNode ;
typedef std::shared_ptr<NamedBlockNode> NamedBlockNodePtr ;

class DocumentNode ;
typedef std::shared_ptr<DocumentNode> DocumentNodePtr ;
}

class TemplateRenderer ;

class Context {
public:
    Context(TemplateRenderer &rdr, const Variant::Object &data):  rdr_(rdr), data_(data) {}
    Context() = delete ;

    Variant::Object &data() {
        return data_ ;
    }

    const Variant::Object &data() const {
        return data_ ;
    }

    const Variant &get(const std::string &key) {
        Variant a ;
        size_t pos = key.find('.') ;
        if ( pos == std::string::npos )
            return data_[key] ;
        else
            return data_[key.substr(0, pos)].at(key.substr(pos+1));
    }

    void addBlock(detail::NamedBlockNodePtr node) ;

    Variant::Object data_ ;


    std::map<std::string, detail::NamedBlockNodePtr> blocks_ ;
    TemplateRenderer &rdr_ ;
    std::string escape_mode_ = "html";
};

} // twig
#endif
