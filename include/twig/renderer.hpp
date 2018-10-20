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

    typedef std::shared_ptr<DocumentNode> DocumentNodePtr ;
}

class FunctionFactory ;

class TemplateRenderer {
public:
    TemplateRenderer(std::shared_ptr<TemplateLoader> loader): loader_(loader) {}

    std::string render(const std::string &resource, const Variant::Object &ctx) ;
    std::string renderString(const std::string &str, const Variant::Object &ctx) ;

    bool setDebug(bool debug = true) {
        debug_ = debug ;
    }

    bool setCaching(bool cache = true) {
        caching_ = cache ;
    }

    static FunctionFactory &getFunctionFactory() { return FunctionFactory::instance() ; }

protected:

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


    friend class detail::ExtensionBlockNode ;
    friend class detail::ImportBlockNode ;
    friend class detail::IncludeBlockNode ;
    friend class detail::EmbedBlockNode ;

    detail::DocumentNodePtr compile(const std::string &resource) ;
    detail::DocumentNodePtr compileString(const std::string &resource) ;

    bool debug_ = false, caching_ = true ;
    std::shared_ptr<TemplateLoader> loader_ ;
} ;

}
#endif
