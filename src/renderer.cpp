#include <twig/renderer.hpp>
#include <twig/context.hpp>
#include "parser.hpp"

using namespace std ;
namespace twig {

string TemplateRenderer::render(const string &resource, const Variant::Object &ctx, bool ignore_missing)
{
    try {
        auto ast = compile(resource) ;

        Context eval_ctx(*this, ctx) ;

        string res ;
        ast->eval(eval_ctx, res) ;
        return res ;
    } catch ( detail::ParseException &e ) {
        throw TemplateCompileException(string("Error compiling template \"") + resource + "\": " + e.what()) ;
        return string() ;
    } catch ( TemplateLoadException &e ) {
        if ( ignore_missing ) return std::string() ;
        else throw e ;
    }
}

string TemplateRenderer::renderString(const string &str, const Variant::Object &ctx) {
    try {
         auto ast = compileString(str) ;
         Context eval_ctx(*this, ctx) ;
         string res ;
         ast->eval(eval_ctx, res) ;
         return res ;
    } catch ( detail::ParseException &e ) {
        throw TemplateCompileException(string("Error compiling template string: ") + e.what()) ;
        return string() ;   
    }
}

detail::DocumentNodePtr TemplateRenderer::compile(const std::string &resource)
{
    if ( resource.empty() ) return nullptr ;
    if ( cache_ ) {
        auto stored = cache_->fetch(resource) ;
        if ( stored ) return stored ;
    }

    string src = loader_->load(resource);

    detail::Parser parser(src, this) ;

    detail::DocumentNodePtr root(new detail::DocumentNode(resource)) ;

    try {
        parser.parse(root, resource) ;
        root->populateBlocks() ;
    } catch ( detail::ParseException & e ) {
        stringstream s ;
        s << "Error compiling '" << resource << "': " << e.msg_ << " at " << e.line_ << '(' << e.col_ << ')' ;
        throw TemplateCompileException(s.str()) ;
    }

    if ( cache_ ) cache_->add(resource, root) ;

    return root ;
}

detail::DocumentNodePtr TemplateRenderer::compileString(const std::string &src) {
    detail::Parser parser(src, this) ;

    detail::DocumentNodePtr root(new detail::DocumentNode()) ;

    parser.parse(root, "--string--") ;

    return root ;
}

}
