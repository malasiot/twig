#include <twig/functions.hpp>
#include <twig/exceptions.hpp>
#include <twig/renderer.hpp>

#include <algorithm>
#include <cmath>

using namespace std ;

namespace twig {

Variant FunctionFactory::invoke(const string &name, const Variant &args)
{
    auto it = functions_.find(name) ;
    if ( it == functions_.end() )
        throw TemplateRuntimeException("Unknown function or filter: " + name) ;

    return (it->second)(args) ;
}

void FunctionFactory::registerFunction(const string &name, const TemplateFunction &f) {
    functions_[name] = f ;
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

static Variant _join(const Variant &args) {
    Variant::Array unpacked ;
    unpack_args(args, { "string_list", "sep?", "key?" },  unpacked) ;

    string sep = ( unpacked[1].isUndefined() ) ? "" : unpacked[1].toString() ;
    string key = ( unpacked[2].isUndefined() ) ? "" : unpacked[2].toString() ;

    bool is_first = true ;
    string res ;
    for( auto &i: unpacked[0] ) {
        if ( !is_first ) res.append(sep) ;
        if ( !key.empty() )
            res.append(i.at(key).toString()) ;
        else
            res.append(i.toString()) ;
        is_first = false ;
    }
    return res ;

}
/*
static Variant _lower(const Variant &args) {
    Variant::Array unpacked ;
    unpack_args(args, { "str" }, unpacked) ;
    return boost::to_lower_copy(unpacked[0].toString()) ;
}

static Variant _upper(const Variant &args) {
    Variant::Array unpacked ;
    unpack_args(args, { "str" }, unpacked) ;
    return boost::to_upper_copy(unpacked[0].toString()) ;
}
*/

static Variant _default(const Variant &args) {
    Variant::Array unpacked ;
    unpack_args(args, { "str", "default" }, unpacked) ;
    return ( unpacked[0].isUndefined() || unpacked[0].isNull() ) ? unpacked[1] : unpacked[0] ;
}

static Variant _raw(const Variant &args) {
    Variant::Array unpacked ;
    unpack_args(args, { "str" }, unpacked) ;

    if ( unpacked[0].isString() )
        return Variant(unpacked[0].toString(), true) ; // make it safe
    else
        return unpacked[0] ;
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


Variant escape(const Variant &src, const string &escape_mode)
{
    if ( src.isSafe() ) return src ;

    if ( escape_mode == "html" )
        return Variant(escape_html(src.toString()), true) ;
    else return src ;
}

static Variant _escape(const Variant &args) {
    Variant::Array unpacked ;
    unpack_args(args, { "str", "mode?" }, unpacked) ;

    string mode = unpacked[1].isUndefined() ? "html" : unpacked[1].toString() ;

    return escape(unpacked[0], mode) ;
}

static Variant _defined(const Variant &args) {
    Variant::Array unpacked ;
    unpack_args(args, { "variable" }, unpacked) ;
    return !(unpacked[0].isUndefined() ) ;
}

static Variant _range(const Variant &args) {
    Variant::Array unpacked, result ;
    unpack_args(args, { "start", "end", "step?" }, unpacked) ;

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

static Variant _length(const Variant &args) {
    Variant::Array unpacked, result ;
    unpack_args(args, { "value" }, unpacked) ;

    return unpacked[0].length() ;
}

static Variant _last(const Variant &args) {
    Variant::Array unpacked ;
    unpack_args(args, { "value" }, unpacked) ;

    if ( unpacked[0].isArray() )
        return unpacked[0].at(unpacked[0].length() - 1) ;
    else if ( unpacked[0].isString() ) {
        char last = unpacked[0].toString().back() ;
        string result ;
        result += last ;
        return result ;
    }
    else return Variant::null() ;
}

static Variant _first(const Variant &args) {
    Variant::Array unpacked ;
    unpack_args(args, { "value" }, unpacked) ;

    if ( unpacked[0].isArray() )
        return unpacked[0].at(0) ;
    else if ( unpacked[0].isString() ) {
        char first = unpacked[0].toString().front() ;
        string result ;
        result += first ;
        return result ;
    }
    else return Variant::null() ;
}

static Variant _batch(const Variant &args) {
    Variant::Array unpacked, out ;
    unpack_args(args, { "items", "size", "fill?" }, unpacked) ;

    if ( !unpacked[0].isArray() )
        throw TemplateRuntimeException("batch filter expects an array") ;

    int len = unpacked[0].length() ;
    int size = ceil(unpacked[1].toFloat()) ;
    int batches = ceil(len/(float)size) ;

    if ( size <= 0 )
        throw TemplateRuntimeException("batch filter size parameter should be a positive integer") ;

    out.resize(batches) ;

    uint idx = 0 ;
    for ( uint k = 0 ; k<batches ; k++ ) {
        Variant::Array ba ;
        for( uint i = 0 ; i<size ; i++, idx++ ) {
            if ( idx < len )
                ba.push_back(unpacked[0].at(idx)) ;
            else if ( !unpacked[2].isUndefined() )
                ba.push_back(unpacked[2]) ;
        }
        out[k] = ba ;
    }

    return out ;
}

static Variant _merge(const Variant &args) {
    Variant::Array unpacked ;
    unpack_args(args, { "src", "other" }, unpacked) ;

    if ( unpacked[0].isArray() ) {
        Variant::Array res ;

        for( auto &&e: unpacked[0] )
            res.emplace_back(std::move(e)) ;

        for( auto &&e: unpacked[1] )
            res.emplace_back(std::move(e)) ;

        return res ;
    } else if ( unpacked[0].isObject() ) {
        Variant::Object res ;

        for( auto it = unpacked[0].begin() ; it != unpacked[0].end() ; ++it )
            res[it.key()] = it.value() ;

        for( auto it = unpacked[1].begin() ; it != unpacked[1].end() ; ++it )
            res[it.key()] = it.value() ;

        return res ;
    }

    else return unpacked[0] ;
}

FunctionFactory::FunctionFactory() {
    registerFunction("join", _join);
 //   registerFunction("lower", _lower);
 //   registerFunction("upper", _upper);
    registerFunction("default", _default);
    registerFunction("e", _escape);
    registerFunction("escape", _escape);
    registerFunction("defined", _defined);
    registerFunction("range", _range);
    registerFunction("length", _length);
    registerFunction("first", _first);
    registerFunction("last", _last);
    registerFunction("raw", _raw);
    registerFunction("safe", _raw);
    registerFunction("batch", _batch);
    registerFunction("merge", _merge);
}

bool FunctionFactory::hasFunction(const string &name)
{
    return functions_.count(name) ;
}

}
