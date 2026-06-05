#include <twig/functions.hpp>
#include <twig/date_helpers.hpp>
#include <twig/exceptions.hpp>
#include <twig/renderer.hpp>
#include <twig/translator.hpp>

#include "ast.hpp"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iomanip>
#include <sstream>
#include <cstring>
#include <regex>
#include <map>

#include <unicode/unistr.h>
#include <unicode/locid.h>



using namespace std ;


extern std::string format(const std::string& fmt, const std::vector<Variant>& args) ;

namespace twig {

Variant FunctionFactory::invokeFunction(const string &name, const Variant &args, Context &ctx) {
    auto it = functions_.find(name) ;
    if ( it == functions_.end() )
        throw TemplateRuntimeException("Unknown function '" + name + "'") ;

    return (it->second)(args, ctx) ;
}

Variant FunctionFactory::invokeFilter(const string &name, const Variant &target, const Variant &args, Context &ctx)
{
    auto it = filters_.find(name) ;
    if ( it == filters_.end() )
        throw TemplateRuntimeException("Unknown filter '" + name + "'") ;

    return (it->second)(target, args, ctx) ;
}

Variant FunctionFactory::invokeTest(const string &name, const Variant &target, const Variant &args, Context &ctx)
{
    auto it = tests_.find(name) ;
    if ( it == tests_.end() )
        throw TemplateRuntimeException("Unknown test " + name + "'") ;

    return (it->second)(target, args, ctx) ;
}

void FunctionFactory::registerFunction(const string &name, const TemplateFunction &f) {
    functions_[name] = f ;
}

void FunctionFactory::registerFilter(const string &name, const FilterFunction &f) {
    filters_[name] = f ;
}

void FunctionFactory::registerTest(const string &name, const TestFunction &f) {
    tests_[name] = f ;
}

void unpack_args(const Variant &args, const std::vector<std::string> &named_args, Variant::Array &res) {

    uint n_args = named_args.size() ;

    res.resize(n_args, Variant::undefined()) ;

    std::vector<bool> provided(n_args, false) ;

    const Variant &pos_args = args["args"] ;

    for ( uint pos = 0 ; pos < n_args && pos < pos_args.length() ; pos ++ )  {
        res[pos] = pos_args.at(pos) ;
        provided[pos] = true ;
    }

    const Variant &kw_args = args["kw"] ;

    for ( auto it = kw_args.begin() ; it != kw_args.end() ; ++it ) {
        string key = it.key() ;
        const Variant &val = it.value() ;

        for( uint k=0 ; k<named_args.size() ; k++ ) {
            const auto &named_arg = named_args[k] ;
            string arg_name ;
            if ( named_arg.back() == '?') arg_name = named_arg.substr(0, named_arg.length() - 1) ;
            else arg_name = named_arg ;

            if ( key == arg_name && !provided[k] ) {
                res[k] = val ;
                provided[k] = true ;
            }
        }
    }


    uint required = std::count_if(named_args.begin(), named_args.end(), [](const string &b) { return b.back() != '?' ;});

    if ( std::count(provided.begin(), provided.end(), true) < required ) {
        throw TemplateRuntimeException("function call missing required arguments") ;
    }
}


static Variant _join(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"glue?", "and?"}, unpacked);
   
    string sep = ( unpacked[0].isUndefined() ) ? "" : unpacked[0].toString() ;
    string key = ( unpacked[1].isUndefined() ) ? "" : unpacked[1].toString() ;

    bool is_first = true ;
    string res ;
    for( const auto &i: target ) {
        if ( !is_first ) res.append(sep) ;
        if ( !key.empty() )
            res.append(i.at(key).toString()) ;
        else
            res.append(i.toString()) ;
        is_first = false ;
    }
    return res ;
}

static Variant _lower(const Variant &target, const Variant &args, Context &ctx) {
    string str = target.toString() ;
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
    return str ;
}

static Variant _upper(const Variant &target, const Variant &args, Context &ctx) {
    string str = target.toString() ;
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::toupper(c); });
    return str ;
}


static Variant _default(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"default"}, unpacked) ;
    return ( target.isUndefined() || target.isNull() ) ? unpacked[0] : target ;
}

static Variant _raw(const Variant &target, const Variant &args, Context &ctx) {
    if ( target.isString() )
        return Variant(target.toString(), true) ; // make it safe
    else
        return target ;
}

static string escape_html(const string &src) {
    string buffer ;
    for ( char c: src ) {
        switch(c) {
        case '&':  buffer.append("&amp;");       break;
        case '\"': buffer.append("&quot;");      break;
        case '\'': buffer.append("&apos;");      break;
        case '<':  buffer.append("&lt;");        break;
        case '>':  buffer.append("&gt;");        break;
        default:   buffer.push_back(c);          break;
        }
    }
    return buffer ;
}

static string escape_js(const string &src) {
   std::ostringstream escaped;

    for (unsigned char c : src) {
        switch (c) {
            case '\\': escaped << "\\\\"; break;
            case '"':  escaped << "\\\""; break;
            case '\'': escaped << "\\'";  break;
            case '\n': escaped << "\\n";  break;
            case '\r': escaped << "\\r";  break;
            case '\t': escaped << "\\t";  break;
            case '\b': escaped << "\\b";  break;
            case '\f': escaped << "\\f";  break;
            case '/':  escaped << "\\/";  break;  // prevents </script> injection
            case '\0': escaped << "\\0";  break;
            case 0xE2: // check for U+2028 / U+2029 (UTF-8: E2 80 A8 / E2 80 A9)
                // These are "line terminators" in JS and must be escaped
                // We'd need lookahead; fall through to hex escape
            default:
                if (c < 0x20 || c == 0x7F) {
                    // Control characters: use \xNN
                    escaped << "\\x"
                            << "0123456789abcdef"[c >> 4]
                            << "0123456789abcdef"[c & 0xF];
                } else {
                    escaped << c;
                }
        }
    }

    return escaped.str();
}


Variant escape(const Variant &src, const string &escape_mode)
{
    if ( src.isSafe() ) return src ;

    if ( escape_mode == "html" )
        return Variant(escape_html(src.toString()), true) ;
    else if ( escape_mode == "js" ) 
        return Variant(escape_js(src.toString()), true) ;
    else return src ;
}

static Variant _escape(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"strategy?", "charset?"}, unpacked) ;

    string mode = unpacked[0].isUndefined() ? "html" : unpacked[0].toString() ;
    return escape(target, mode) ;
}

static Variant _defined(const Variant &target, const Variant &args, Context &ctx) {
    return !(target.isUndefined() ) ;
}

static Variant range(const Variant &args, Context &ctx) {
    Variant::Array unpacked, result ;
    unpack_args(args, {"low", "high", "step?"}, unpacked) ;

    if ( unpacked[0].type() == Variant::Type::Integer ) {
        int64_t start = unpacked[0].toInteger() ;
        int64_t stop = unpacked[1].toInteger() ;
        int64_t step = unpacked[2].isUndefined() ? 1 : unpacked[2].toInteger() ;
        if ( step == 0 ) throw TemplateRuntimeException("Zero step is provided in range function") ;
        if ( ( step > 0 && start > stop ) ||
             ( step < 0 && start < stop ) )
            throw TemplateRuntimeException("Invalid arguments provided in range function") ;

        for( int64_t i = start ; i <= stop ; i += step )
            result.push_back(i) ;
    }

    return result ;
}

static Variant _length(const Variant &target, const Variant &args, Context &ctx) {
    return Variant(static_cast<int64_t>(target.length())) ;
}

static Variant _last(const Variant &target, const Variant &args, Context &ctx) {
    if ( target.isArray() )
        return target.at(target.length() - 1) ;
    else if ( target.isString() ) {
        char last = target.toString().back() ;
        string result ;
        result += last ;
        return result ;
    }
    else return Variant::null() ;
}

static Variant _find(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"arrow"}, unpacked) ;   

    Variant lambda = unpacked[0] ;
    if ( lambda.type() != Variant::Type::Function )
        throw TemplateRuntimeException("find filter expects a lambda as an argument") ;
    if ( target.isArray() ) {
        Variant::Array res ;
        for( auto &e: target ) {
            Variant::Array lambda_args { e } ;
            if ( lambda.invoke(lambda_args).toBoolean() )
                return e ;
        }
        return Variant::undefined() ;
    } else if ( target.isObject() ) {
        Variant::Object res ;
        for( auto it = target.begin() ; it != target.end() ; ++it ) {
            Variant::Array kv { it.key(), it.value() } ;
            if ( lambda.invoke(kv).toBoolean() )
                return it.value() ;
        }
        return Variant::undefined() ;
    } else
        return Variant::undefined() ;
}

static Variant _first(const Variant &target, const Variant &args, Context &ctx) {
    if ( target.isArray() )
        return target.at(0) ;
    else if ( target.isString() ) {
        char first = target.toString().front() ;
        string result ;
        result += first ;
        return result ;
    }
    else return Variant::null() ;
}

static Variant _abs(const Variant &target, const Variant &args, Context &ctx) {
    try {
        Variant num = target.toNumber() ;
        if ( num.type() == Variant::Type::Integer )
            return Variant(std::abs(target.toInteger())) ;
        else if ( num.type() == Variant::Type::Float )
            return Variant(std::abs(target.toFloat())) ;
        else if ( num.type() == Variant::Type::Boolean )
            return Variant(target.toBoolean()) ;
        else
            return Variant(std::abs(target.toFloat())) ;
    } catch ( const TypeConversionException & ) {
        throw TemplateRuntimeException("abs filter expects a numeric value") ;
    }
}

static Variant _capitalize(const Variant &target, const Variant &args, Context &ctx) {
    string val = target.toString() ;
    if ( val.empty() )
        return val ;

    icu::UnicodeString u = icu::UnicodeString::fromUTF8(val);
    if ( u.isEmpty() )
        return val ;

    int32_t firstCharEnd = u.moveIndex32(0, 1);
    icu::UnicodeString first = u.tempSubStringBetween(0, firstCharEnd);
    first.toUpper(icu::Locale::getDefault());

    icu::UnicodeString rest = u.tempSubString(firstCharEnd);
    icu::UnicodeString result = first;
    result.append(rest);

    std::string output;
    result.toUTF8String(output);
    return output;
}

static Variant _filter(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"arrow"}, unpacked) ;

    Variant data = target ;
    Variant lambda = unpacked[0] ;
    if ( lambda.type() != Variant::Type::Function )
        throw TemplateRuntimeException("filter function expects a lambda as second argument") ;
    if ( data.isArray() ) {
        Variant::Array res ;
        for( auto &e: data ) {
            Variant::Array lambda_args { e } ;
            if ( lambda.invoke(lambda_args).toBoolean() )
                res.push_back(e) ;
        }
        return res ;
    } else if ( data.isObject() ) {
        Variant::Object res ;
        for( auto it = data.begin() ; it != data.end() ; ++it ) {
            Variant::Array kv { it.key(), it.value() } ;
            if ( lambda.invoke(kv).toBoolean() )
                res[it.key()] = it.value() ;
        }
        return res ;
    } else
        return data ;
       
}

static Variant _keys(const Variant &target, const Variant &args, Context &ctx) {
    if ( target.isObject() ) {
        Variant::Array res ;
        for( auto it = target.begin() ; it != target.end() ; ++it )
            res.push_back(it.key()) ;
        return res ;
    }
    else return Variant::null() ;
}

static Variant _batch(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked, out ;
    unpack_args(args, {"size", "fill", "preserve_keys?"}, unpacked) ;

    if ( !target.isArray() )
        throw TemplateRuntimeException("batch filter expects an array") ;

    int len = target.length() ;
    int size = ceil(unpacked[0].toFloat()) ;
    int batches = ceil(len/(float)size) ;

    if ( size <= 0 )
        throw TemplateRuntimeException("batch filter size parameter should be a positive integer") ;

    out.resize(batches) ;

    uint idx = 0 ;
    for ( uint k = 0 ; k<batches ; k++ ) {
        Variant::Array ba ;
        for( uint i = 0 ; i<size ; i++, idx++ ) {
            if ( idx < len )
                ba.push_back(target.at(idx)) ;
            else if ( !unpacked[1].isUndefined() )
                ba.push_back(unpacked[1]) ;
        }
        out[k] = ba ;
    }

    return out ;
}

static Variant _merge(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"other"}, unpacked) ;

    if ( target.isArray() ) {
        Variant::Array res ;

        for( auto &&e: target )
            res.emplace_back(std::move(e)) ;

        for( auto &&e: unpacked[0] )
            res.emplace_back(std::move(e)) ;

        return res ;
    } else if ( target.isObject() ) {
        Variant::Object res ;

        for( auto it = target.begin() ; it != target.end() ; ++it )
            res[it.key()] = it.value() ;

        for( auto it = unpacked[0].begin() ; it != unpacked[0].end() ; ++it )
            res[it.key()] = it.value() ;

        return res ;
    }

    else return target ;
}

static Variant cycle(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"values", "position?"}, unpacked) ;
    if ( !unpacked[0].isArray() )
        throw TemplateRuntimeException("cycle function expects an array as first argument") ;

    int position = ( !unpacked[1].isUndefined() ) ? unpacked[1].toInteger() : 0 ;
    return unpacked[0].at(position % unpacked[0].length()) ;
}

static Variant _format(const Variant &target, const Variant &args, Context &ctx) {
    string fmt = target.toString() ;
    return ::format(fmt, args["args"].toArray());
}

static Variant _date(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"format?", "timezone?"}, unpacked) ;

    Variant src = target ;
    string tz = unpacked[1].isUndefined() ? string() : unpacked[1].toString() ;
    
    string format = ( unpacked[0].isUndefined() ) ? "F j, Y H:i" : unpacked[0].toString() ;

    int64_t tms ;
    if ( src.isString() ) {
        try {
            return strtotime(src.toString(), tz) ;
        } catch ( const std::runtime_error & ) {
            throw TemplateRuntimeException("Failed to parse date string: " + src.toString()) ;
        }
        
    }
    else if ( src.type() == Variant::Type::Integer ) {
        tms = src.toInteger() ;
    }
    else if ( src.isDateTime() ) {
        tms = src.toDateTime() ;
    }
    else if ( src.isDuration() ) {
        tms = src.toDuration() ;
    }
    else
        throw TemplateRuntimeException("date function expects a string, integer timestamp, DateTime or Duration as first argument") ;

    string strftime_format = php_date_format_to_strftime(format) ;

    std::time_t t = static_cast<std::time_t>(tms) ;
    tm tm ;
    if ( tz.empty() ) {
        // Use local timezone
        localtime_r(&t, &tm) ;
    } else {
        // Convert to the specified timezone
        int offset_seconds ;
        if ( parse_timezone_offset(tz, offset_seconds) ) {
            // Adjust the UTC timestamp by the timezone offset to get local time
            // gmtime_r will then interpret the adjusted timestamp as UTC epoch,
            // giving us the correct struct tm for that timezone
            std::time_t adjusted = t + offset_seconds ;
            gmtime_r(&adjusted, &tm) ;
        } else {
            // Fall back to local timezone if parsing fails
            localtime_r(&t, &tm) ;
        }
    }

    char buffer[256] ;
    if ( std::strftime(buffer, sizeof(buffer), strftime_format.c_str(), &tm) == 0 )
        throw TemplateRuntimeException("Failed to format date string") ;

    return Variant(buffer) ;

}

static Variant include(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"template", "variables?", "with_context?", "ignore_missing?", "sandboxed?"}, unpacked) ;

    Variant::Object variables ;
    if ( !unpacked[1].isUndefined() ) 
        variables = unpacked[1].toObject() ;

    bool ignore_missing = unpacked[3].isUndefined() ? false : unpacked[3].toBoolean() ;
    bool with_context = unpacked[2].isUndefined() ? true : unpacked[2].toBoolean() ;

    if ( with_context )
        variables.insert(ctx.data_.begin(), ctx.data_.end());

    return ctx.rdr_.render(unpacked[0].toString(), variables, ignore_missing) ;
}
namespace detail {
extern std::string resolve_and_render_block(const std::string &name, detail::DocumentNode *doc, Context &ctx) ;
}
static Variant parent(const Variant &args, Context &ctx) {

    if ( ctx.active_block_ == nullptr ) return Variant::undefined() ;

    detail::DocumentNode* current_block_owner = ctx.active_block_->root();

    // Guard: If there is no parent template above this layer, parent() does nothing
    if ( !current_block_owner || current_block_owner->parent_ == nullptr) {
        return Variant::undefined();
    }

    // Step exactly one file layer up the chain
    detail::DocumentNode* parent_layer = current_block_owner->parent_.get();

    // Create a local context copy to isolate this recursive branch pass
    Context nested_ctx(ctx) ;

    // Ask the engine to resolve the block starting strictly from the parent layer upward
    try {
        return resolve_and_render_block(ctx.active_block_->name_, parent_layer, nested_ctx);
    } catch ( TemplateRuntimeException &e ) {
        current_block_owner->throwException(e.what()) ;
    }

    return std::string() ;
}

static Variant block(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"name", "template?"}, unpacked) ;

    string name = unpacked[0].toString() ;
   
    try {
        Context nested_ctx(ctx) ;
        return resolve_and_render_block(name, ctx.root_tmpl_, nested_ctx);
    } catch ( TemplateRuntimeException &e ) {
        ctx.root_tmpl_->throwException(e.what()) ;
        
    }
    return std::string() ;
}

static Variant html_attr(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"dict"}, unpacked) ;

    if ( !unpacked[0].isObject() ) return "" ;

    std::ostringstream ss;
    for( const auto &[key, val]: unpacked[0].toObject() ) {
        if ( val.isNull() || ( val.isBoolean() && !val.toBoolean()) ) 
            continue ;
        else if ( val.isBoolean() && val.toBoolean() ) {
            ss << ' ' << key;
        } else if ( val.isArray() ) {
            ss << ' ' << key << "=\"";
            bool first = true;
            for (const auto& item : val) {
                if ( !first ) ss << " ";
                // Safely convert items to string context strings
                ss << item.toString();
                first = false;
            }
            ss << "\"";
        } else {
            ss << ' ' << key << "=\"" << escape_html(val.toString()) << "\"";
        }
    }
   
    string res = ss.str() ;
    return res.substr(1) ;
}

static Variant date(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"date?", "timezone?"}, unpacked) ;

    auto t = unpacked[0] ;
    string tz = unpacked[1].isUndefined() ? string() : unpacked[1].toString() ;

    if ( t.isUndefined() ) return Variant::now() ;

    if ( t.isString() ) {
        try {
            return strtotime(t.toString(), tz) ;
        } catch ( const std::runtime_error & ) {
            throw TemplateRuntimeException("Failed to parse date string: " + unpacked[0].toString()) ;
        }
    } else if ( t.type() == Variant::Type::Integer ) {
        return unpacked[0].toInteger() ;
    }
    else if ( t.isDateTime() ) {
        return t ;
    }
    else if ( t.isDuration() ) {
        return t ;
    }
    else
        throw TemplateRuntimeException("date function expects a string, integer timestamp, DateTime or Duration as first argument") ;
}

static Variant _trim(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"character_mask?", "side?"}, unpacked) ;

    string str = target.toString() ;
    string chars = unpacked[0].isUndefined() ? " \t\n\r\f\v" : unpacked[0].toString() ;
    string side = unpacked[1].isUndefined() ? "both" : unpacked[1].toString() ;

    size_t start = str.find_first_not_of(chars) ;
    if ( start == string::npos ) return "" ;
    size_t end = str.find_last_not_of(chars) ;

    return str.substr(start, end - start + 1) ;
}

static Variant _map_filter(const Variant &target, const Variant &args, Context &ctx) {
   Variant::Array unpacked ;
    unpack_args(args, {"arrow"}, unpacked) ;

    Variant data = target ;
    Variant lambda = unpacked[0] ;
    if ( lambda.type() != Variant::Type::Function )
        throw TemplateRuntimeException("filter function expects a lambda as second argument") ;
    if ( data.isArray() ) {
        Variant::Array res ;
        for( auto &e: data ) {
            Variant::Array lambda_args { e } ;
            Variant r = lambda.invoke(lambda_args) ;
            res.push_back(r) ;
        }
        return res ;
    } else if ( data.isObject() ) {
        Variant::Object res ;
        for( auto it = data.begin() ; it != data.end() ; ++it ) {
            Variant::Array kv { it.key(), it.value() } ;
            Variant r = lambda.invoke(kv) ;
            res.emplace(it.key(), r) ;
        }
        return res ;
    } else
        return data ;
}

static Variant _reduce(const Variant &target, const Variant &args, Context &ctx) {
   Variant::Array unpacked ;
    unpack_args(args, {"arrow", "initial?"}, unpacked) ;

    Variant data = target ;
    Variant lambda = unpacked[0] ;
    double initial = unpacked[1].isUndefined() ? 0 : unpacked[1].toFloat() ;

    if ( lambda.type() != Variant::Type::Function )
        throw TemplateRuntimeException("reduce function expects a lambda as an argument") ;
    
    double result = initial ;
    if ( data.isArray() ) {
        Variant::Array res ;
        for( size_t i=0 ; i<data.length() ; i++ ) {
            Variant::Array lambda_args { result, (int64_t)i, data.at(i) } ;
            result = lambda.invoke(lambda_args).toFloat() ;
        }
        return result ;
    } else if ( data.isObject() ) {
        Variant::Object res ;
        for( auto it = data.begin() ; it != data.end() ; ++it ) {
            Variant::Array kv { result, it.key(), it.value() } ;
            result = lambda.invoke(kv).toFloat() ;
        }
        return result ;
    } else
        return 0 ;
}

static Variant _json_encode(const Variant &target, const Variant &args, Context &ctx) {
    return target.toJSON() ;
}

static Variant _round(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"precision?", "method?"}, unpacked) ;

    int precision = unpacked[0].isUndefined() ? 0 : unpacked[0].toInteger() ;
    string method = unpacked[1].isUndefined() ? "common" : unpacked[1].toString() ;

    double v = target.toFloat() ;
    double scale = std::pow(10.0, precision);
    if ( method == "ceil" ) return std::ceil(v * scale)/scale ;
    else if ( method == "floor" ) return std::floor(v * scale)/scale ;
    else return std::round(v * scale) / scale;
}

static Variant _slice(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"start?", "length?", "preserve_keys?"}, unpacked) ;

    int start = unpacked[0].isUndefined() ? 0 : unpacked[0].toInteger() ;
    bool has_length = !unpacked[1].isUndefined();
    int length = has_length ? unpacked[1].toInteger() : 0 ;
    bool preserve_keys = unpacked[2].isUndefined() ? false : unpacked[2].toBoolean() ;

    // Work with the correct size depending on the type
    int total = static_cast<int>(target.length());

    // Normalize start (support negative)
    if ( start < 0 ) start = total + start;
    if ( start < 0 ) start = 0;
    if ( start > total ) start = total;

    // If length was not provided, default to remaining elements
    if ( !has_length ) {
        length = total - start;
    }

    // Normalize length (support negative)
    if ( length < 0 ) {
        length = (total - start) + length;
    }
    if ( length < 0 ) length = 0;
    if ( start + length > total ) length = total - start;

    if ( target.isArray() ) {
        Variant::Array out ;
        Variant::Object outobj ;

        if ( length <= 0 ) {
            return preserve_keys ? Variant(outobj) : Variant(out) ;
        }

        for ( int i = 0 ; i < length ; i++ ) {
            int idx = start + i ;
            const Variant &v = target.at(static_cast<uint>(idx)) ;
            if ( preserve_keys )
                outobj[std::to_string(idx)] = v ;
            else
                out.push_back(v) ;
        }

        return preserve_keys ? Variant(outobj) : Variant(out) ;
    }
    else if ( target.isObject() ) {
        Variant::Object outobj ;
        Variant::Array out ;

        std::vector<std::string> keys = target.keys() ;
        // adjust bounds for keys
        if ( start > static_cast<int>(keys.size()) ) start = static_cast<int>(keys.size()) ;
        if ( start + length > static_cast<int>(keys.size()) ) length = static_cast<int>(keys.size()) - start ;

        if ( length <= 0 ) return preserve_keys ? Variant(outobj) : Variant(out) ;

        for ( int i = 0 ; i < length ; i++ ) {
            const std::string &k = keys[start + i] ;
            const Variant &v = target[k] ;
            if ( preserve_keys ) outobj[k] = v ;
            else out.push_back(v) ;
        }

        return preserve_keys ? Variant(outobj) : Variant(out) ;
    }
    else if ( target.isString() ) {
        std::string s = target.toString() ;
        if ( start >= static_cast<int>(s.size()) || length <= 0 ) return string() ;
        return s.substr(static_cast<size_t>(start), static_cast<size_t>(length)) ;
    }

    return target ;

}

static Variant _trans(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"params?"}, unpacked) ;

    string msg = target.toString() ;
    auto params = unpacked[0] ;
    if ( ctx.mgr_ != nullptr )
        return ctx.mgr_->translate(msg, ctx.locale_,
            params.isUndefined() ? Variant::Object{} : params.toObject() );
    else
        return msg ;
}

static bool _even(const Variant &target, const Variant &args, Context &ctx) {
    return target.toInteger() % 2 == 0 ;
}

static bool _divisible_by(const Variant &target, const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, {"divisor"}, unpacked) ;
    return target.toInteger() % unpacked[0].toInteger() == 0 ;
}

static bool _odd(const Variant &target, const Variant &args, Context &ctx) {
    return target.toInteger() % 2 != 0 ;
}

static bool _is_defined(const Variant &target, const Variant &args, Context &ctx) {
    return !target.isUndefined() ;
}

static bool _empty(const Variant &target, const Variant &args, Context &ctx) {
    return target.isUndefined() || target.isNull() || ( target.isString() && target.toString().empty() ) ||
           ( target.isArray() && target.length() == 0 ) ||
           ( target.isObject() && target.length() == 0 ) ;
}

static bool _iterable(const Variant &target, const Variant &args, Context &ctx) {
    return target.isArray() ||
           target.isObject()  ;
}

static bool _null(const Variant &target, const Variant &args, Context &ctx) {
    return target.isNull() ;
}

static bool _sequence(const Variant &target, const Variant &args, Context &ctx) {
    return target.isArray()  ;
}

static bool _map(const Variant &target, const Variant &args, Context &ctx) {
    return target.isObject()  ;
}

extern Variant form_start(const Variant &args, Context &ctx) ;
extern Variant form_end(const Variant &args, Context &ctx) ;
extern Variant form_row(const Variant &args, Context &ctx) ;

FunctionFactory::FunctionFactory() {
    registerFilter("join", _join);
    registerFilter("lower", _lower);
    registerFilter("upper", _upper);
    registerFilter("default", _default);
    registerFilter("e", _escape);
    registerFilter("escape", _escape);
    registerFilter("defined", _defined);
    registerFilter("length", _length);
    registerFilter("first", _first);
    registerFilter("last", _last);
    registerFilter("raw", _raw);
    registerFilter("safe", _raw);
    registerFilter("batch", _batch);
    registerFilter("merge", _merge);
    registerFilter("date", _date) ;
    registerFilter("abs", _abs) ;
    registerFilter("capitalize", _capitalize) ;
    registerFilter("filter", _filter) ;
    registerFilter("trim", _trim) ;
    registerFilter("keys", _keys) ;
    registerFilter("format", _format) ;
    registerFilter("json_encode", _json_encode) ;
    registerFilter("find", _find) ;
    registerFilter("map", _map_filter) ;
    registerFilter("reduce", _reduce) ;
    registerFilter("round", _round) ;
    registerFilter("slice", _slice) ;
    registerFilter("trans", _trans) ;

    registerFunction("range", range);
    registerFunction("cycle", cycle) ;
    registerFunction("date",  date) ;
    registerFunction("include",  include) ;
    registerFunction("parent", parent);
    registerFunction("block", block);
    registerFunction("html_attr", html_attr);
  
    registerTest("divisible by", _divisible_by) ;
    registerTest("even", _even) ;
    registerTest("odd", _odd) ;
    registerTest("defined", _is_defined) ;
    registerTest("empty", _empty) ;
    registerTest("iterable", _iterable) ;
    registerTest("null", _null) ;
    registerTest("sequence", _sequence) ;
    registerTest("mapping", _map) ;
}

bool FunctionFactory::hasFunction(const string &name)
{
    return functions_.count(name) ;
}

bool FunctionFactory::hasFilter(const string &name)
{
    return filters_.count(name) ;
}

}
