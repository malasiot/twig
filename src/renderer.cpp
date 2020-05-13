#include <twig/renderer.hpp>
#include <twig/context.hpp>
#include "parser.hpp"

using namespace std ;
namespace twig {

string TemplateRenderer::render(const string &resource, const Variant::Object &ctx)
{
    auto ast = compile(resource) ;

    Context eval_ctx(*this, ctx) ;

    string res ;
    ast->eval(eval_ctx, res) ;
    return res ;
}

string TemplateRenderer::renderString(const string &str, const Variant::Object &ctx)
{

    auto ast = compileString(str) ;

    Context eval_ctx(*this, ctx) ;
    string res ;
    ast->eval(eval_ctx, res) ;
    return res ;
}

detail::DocumentNodePtr TemplateRenderer::compile(const std::string &resource)
{
    static Cache g_cache ;

    if ( resource.empty() ) return nullptr ;

    if ( caching_ ) {
        auto stored = g_cache.fetch(resource) ;
        if ( stored ) return stored ;
    }

    string src = loader_->load(resource);

    detail::Parser parser(src) ;

    detail::DocumentNodePtr root(new detail::DocumentNode(*this)) ;

    try {
    parser.parse(root, resource) ;
    } catch ( detail::ParseException & e ) {
        stringstream s ;
        s << "Error compiling '" << resource << "': " << e.msg_ << " at " << e.line_ << '(' << e.col_ << ')' ;
        throw TemplateCompileException(s.str()) ;
    }

    if ( caching_ ) g_cache.add(resource, root) ;

    return root ;
}

detail::DocumentNodePtr TemplateRenderer::compileString(const std::string &src)
{

    detail::Parser parser(src) ;

    detail::DocumentNodePtr root(new detail::DocumentNode(*this)) ;

    parser.parse(root, "--string--") ;

    return root ;
}

}
