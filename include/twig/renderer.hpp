#ifndef TWIG_RENDERER_HPP
#define TWIG_RENDERER_HPP

#include <memory>

#include <twig/loader.hpp>
#include <twig/exceptions.hpp>
#include <twig/functions.hpp>

#include <twig/variant.hpp>
#include <mutex>

namespace twig {
namespace detail {
    class DocumentNode ;
    class ExtensionBlockNode ;
    class ImportBlockNode ;
    class IncludeBlockNode ;
    class EmbedBlockNode ;
    class FormThemeBlockNode ;

    typedef std::shared_ptr<DocumentNode> DocumentNodePtr ;
}

class FunctionFactory ;
class Cache ;

class TemplateRenderer {
public:
    TemplateRenderer(std::shared_ptr<TemplateLoader> loader): loader_(loader) {}

    std::string render(const std::string &resource, const Variant::Object &ctx, bool ignore_missing = false) ;
    std::string renderString(const std::string &str, const Variant::Object &ctx) ;

    void setDebug(bool debug = true) {
        debug_ = debug ;
    }

    void setCache(const std::shared_ptr<Cache> &cache) {
        cache_ = cache ;
    }


    static FunctionFactory &getFunctionFactory() { return FunctionFactory::instance() ; }

    std::shared_ptr<TemplateLoader> getLoader() { return loader_ ; } 

protected:

    friend class detail::ExtensionBlockNode ;
    friend class detail::ImportBlockNode ;
    friend class detail::IncludeBlockNode ;
    friend class detail::EmbedBlockNode ;
    friend class detail::FormThemeBlockNode ;

    detail::DocumentNodePtr compile(const std::string &resource) ;
    detail::DocumentNodePtr compileString(const std::string &resource) ;

    bool debug_ = false, ignore_missing_ = false ;
    std::shared_ptr<TemplateLoader> loader_ ;
    std::shared_ptr<Cache> cache_ ;
} ;

class Cache {
public:

using Entry = detail::DocumentNodePtr ;

void add(const std::string &key, const Entry &val) {
    std::lock_guard<std::mutex> lock(guard_);
    compiled_.insert({key, val}) ;
}

Entry fetch(const std::string &key) {
    std::lock_guard<std::mutex> lock(guard_);
    auto it = compiled_.find(key) ;
    if ( it != compiled_.end() ) return it->second ;
    else return nullptr ;
}

private:
    std::map<std::string, Entry> compiled_ ;
    std::mutex guard_ ;
};

}
#endif
