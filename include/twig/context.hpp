#ifndef TWIG_CONTEXT_HPP
#define TWIG_CONTEXT_HPP

#include <memory>
#include <set>

#include <variant/variant.hpp>

namespace twig {
namespace detail {
class NamedBlockNode ;
class ContainerNode ;
typedef std::shared_ptr<NamedBlockNode> NamedBlockNodePtr ;

class DocumentNode ;
typedef std::shared_ptr<DocumentNode> DocumentNodePtr ;
}

class TemplateRenderer ;
class TranslationManager ;

class Context {
public:
    Context(TemplateRenderer &rdr, const Variant::Object &data, TranslationManager *mgr, const std::string &locale):
      rdr_(rdr), data_(data), mgr_(mgr), locale_(locale) {}
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
    TemplateRenderer &rdr_ ;
    TranslationManager *mgr_ = nullptr;
    std::string escape_mode_ = "no", locale_ = "en_US";
    detail::DocumentNode *root_tmpl_ = nullptr ;
    detail::NamedBlockNode *active_block_  = nullptr;
};
} // twig
#endif
