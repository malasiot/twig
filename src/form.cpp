#include <twig/context.hpp>
#include <twig/functions.hpp>
#include <twig/exceptions.hpp>
#include "ast.hpp"

using namespace std ;

namespace twig {

bool find_and_render_theme_block(Context &ctx, const Variant &form_data, const std::string &block_name, string &res) {
    std::vector<std::string> themes;
    
    auto it = ctx.form_to_themes_map_.find(ctx.active_form_id_) ;
    
    if ( it != ctx.form_to_themes_map_.end() )
        themes = it->second ;        

    if ( ctx.themes_.count("_default_theme_") != 0 ) 
        themes.push_back("_default_theme_");

    for (auto it = themes.rbegin(); it != themes.rend(); ++it) {
        std::string theme_key = *it;
        auto& active_theme = ctx.themes_[theme_key];
        
        auto block_it = active_theme->findBlock(block_name);

        Context lctx(ctx.rdr_, form_data.toObject()) ;
        for( auto &&c: block_it->children_ ) {
            c->eval(lctx, res) ;
        }
        return true ;
    }

    return false ;
}

bool render_form_block(Context &ctx, const Variant &data, const std::string &postfix, string &res) {

    Variant prefixes = data["_prefixes"] ;
    if ( !prefixes.isUndefined() && prefixes.isArray() ) {

        for ( auto it = prefixes.rbegin() ; it != prefixes.rend() ; ++it ) { // TODO: reverse
            std::string target_row_block = it.value().toString() + "_" + postfix;

            if ( find_and_render_theme_block(ctx, data, target_row_block, res) ) 
                return true ;

        }    
    }    

    return false ;
}

Variant form_start(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"var"}, unpacked);

    Variant form_data = unpacked[0] ;
 
    if (!form_data.isObject()) {
        throw TemplateRuntimeException("form_start requires a form object") ;
    }

    ctx.active_form_id_ = form_data.get("_full_name", "form").toString();

    string res ;
    find_and_render_theme_block(ctx, unpacked[0], "form_start", res);
    return res ;
}

Variant form_end(const Variant &args, Context &ctx) {
   std::string automatic_fields_html = "";
    
   Variant::Array unpacked ;
   unpack_args(args, {"var"}, unpacked);

    // Recursively parse children inside the root json object
    for ( auto it = unpacked[0].begin() ; it != unpacked[0].end() ; ++it ) {

        // Skip metadata tags starting with underscore
        if ( it.key().rfind("_", 0) == 0 ) continue; 
        
        const auto &value = it.value() ;

        if ( value.isObject() && !value["_full_name"].isUndefined()) {
            std::string field_name = value["_full_name"].toString() ;
            
            // If the template layout forgot to call form_row/widget on this element
            if ( !ctx.rendered_form_fields_.count(field_name) ) {
                string res ;
                if ( render_form_block(ctx, value, "widget", res) ) {
                    ctx.rendered_form_fields_.insert(field_name);
                    return res ;
                }
            }
        }
    }
    
    return automatic_fields_html + "</form>";
}

Variant form_row(const Variant &args, Context &ctx) {

    Variant::Array unpacked ;
    unpack_args(args, {"var"}, unpacked);

    Variant data = unpacked[0] ;
 
    if (!data.isObject()) {
        throw TemplateRuntimeException("form_row requires a form object") ;
    }

    if ( data["_full_name"].isUndefined() )
        throw TemplateRuntimeException("form_row data missing _full_name attribute") ;
         
    std::string field_full_name = data["_full_name"].toString();

    ctx.rendered_form_fields_.insert(field_full_name) ;

    string res ;
    if ( render_form_block(ctx, data, "widget", res) ) return res ;
    
    if ( find_and_render_theme_block(ctx, data, "form_row", res) ) 
        return res ;

    return "";
}


}