#include <twig/functions.hpp>
#include <twig/date_helpers.hpp>
#include <twig/exceptions.hpp>
#include <twig/renderer.hpp>

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


extern std::string format(const std::string& fmt, const std::vector<twig::Variant>& args) ;

namespace twig {

Variant FunctionFactory::invokeFunction(const string &name, const Variant &args, Context &ctx)
{
    auto it = functions_.find(name) ;
    if ( it == functions_.end() )
        throw TemplateRuntimeException("Unknown function: " + name) ;

    return (it->second)(args, ctx) ;
}

Variant FunctionFactory::invokeFilter(const string &name, const Variant &args, Context &ctx)
{
    auto it = filters_.find(name) ;
    if ( it == filters_.end() )
        throw TemplateRuntimeException("Unknown filter: " + name) ;

    return (it->second)(args, ctx) ;
}

Variant FunctionFactory::invokeTest(const string &name, const Variant &args, Context &ctx)
{
    auto it = tests_.find(name) ;
    if ( it == tests_.end() )
        throw TemplateRuntimeException("Unknown test: " + name) ;

    return (it->second)(args, ctx) ;
}

void FunctionFactory::registerFunction(const string &name, const TemplateFunction &f) {
    functions_[name] = f ;
}

void FunctionFactory::registerFilter(const string &name, const TemplateFunction &f) {
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

static Variant _join(const Variant &args, Context &ctx) {
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

static Variant _lower(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "str" }, unpacked) ;

    string str = unpacked[0].toString() ;
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
    return str ;
}

static Variant _upper(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "str" }, unpacked) ;

    string str = unpacked[0].toString() ;
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::toupper(c); });
    return str ;

}


static Variant _default(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "str", "default" }, unpacked) ;
    return ( unpacked[0].isUndefined() || unpacked[0].isNull() ) ? unpacked[1] : unpacked[0] ;
}

static Variant _raw(const Variant &args, Context &ctx) {
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

static Variant _escape(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "str", "mode?" }, unpacked) ;

    string mode = unpacked[1].isUndefined() ? "html" : unpacked[1].toString() ;

    return escape(unpacked[0], mode) ;
}

static Variant _defined(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "variable" }, unpacked) ;
    return !(unpacked[0].isUndefined() ) ;
}

static Variant range(const Variant &args, Context &ctx) {
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

static Variant _length(const Variant &args, Context &ctx) {
    Variant::Array unpacked, result ;
    unpack_args(args, { "value" }, unpacked) ;

    return Variant(static_cast<int64_t>(unpacked[0].length())) ;
}

static Variant _last(const Variant &args, Context &ctx) {
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

static Variant _first(const Variant &args, Context &ctx) {
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

static Variant _abs(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "value" }, unpacked) ;

    try {
        Variant num = unpacked[0].toNumber() ;
        if ( num.type() == Variant::Type::Integer )
            return Variant(std::abs(unpacked[0].toInteger())) ;
        else if ( num.type() == Variant::Type::Float )
            return Variant(std::abs(unpacked[0].toFloat())) ;
        else if ( num.type() == Variant::Type::Boolean )
            return Variant(unpacked[0].toBoolean()) ;
        else
            return Variant(std::abs(unpacked[0].toFloat())) ;
    } catch ( const TypeConversionException & ) {
        throw TemplateRuntimeException("abs filter expects a numeric value") ;
    }
}

static Variant _capitalize(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "value" }, unpacked) ;

    string val = unpacked[0].toString() ;
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

static Variant _filter(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "data", "lambda" }, unpacked) ;

    Variant data = unpacked[0] ;
    Variant lambda = unpacked[1] ;
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

static Variant _keys(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "data" }, unpacked) ;

    if ( unpacked[0].isObject() ) {
        Variant::Array res ;
        for( auto it = unpacked[0].begin() ; it != unpacked[0].end() ; ++it )
            res.push_back(it.key()) ;
        return res ;
    }
    else return Variant::null() ;
}

static Variant _batch(const Variant &args, Context &ctx) {
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

static Variant _merge(const Variant &args, Context &ctx) {
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

static Variant cycle(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src", "position?" }, unpacked) ;
    if ( !unpacked[0].isArray() )
        throw TemplateRuntimeException("cycle function expects an array as first argument") ;

    int position = ( unpacked.size() > 1 ) ? unpacked[1].toInteger() : 0 ;
    return unpacked[0].at(position % unpacked[0].length()) ;
}

static Variant _format(const Variant &args, Context &ctx) {
    Variant unpacked = args["args"] ;
    string fmt = unpacked[0].toString() ;
   
    Variant::Array params ;
    for(int i=1 ; i<unpacked.length() ; i++ ) {
        params.push_back(unpacked[i]) ;
    }
     
    return ::format(fmt, params);
}

static Variant _date(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src", "format?", "tz?" }, unpacked) ;

    Variant src = unpacked[0] ;
    string tz = unpacked[2].isUndefined() ? string() : unpacked[2].toString() ;
    
    string format = ( unpacked[1].isUndefined() ) ? "F j, Y H:i" : unpacked[1].toString() ;

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

static Variant date(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src?", "tz?" }, unpacked) ;

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

static Variant _trim(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "str", "chars?" }, unpacked) ;

    string str = unpacked[0].toString() ;
    string chars = unpacked[1].isUndefined() ? " \t\n\r\f\v" : unpacked[1].toString() ;

    size_t start = str.find_first_not_of(chars) ;
    if ( start == string::npos ) return "" ;
    size_t end = str.find_last_not_of(chars) ;

    return str.substr(start, end - start + 1) ;
}

static bool _even(const Variant &args, Context &ctx) {
  Variant::Array unpacked ;
    unpack_args(args, { "src" }, unpacked) ;
    return unpacked[0].toInteger() % 2 == 0 ;
}

static bool _divisible_by(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src, divisor" }, unpacked) ;
    return unpacked[0].toInteger() % unpacked[1].toInteger() == 0 ;
}

static bool _odd(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src" }, unpacked) ;
    return unpacked[0].toInteger() % 2 != 0 ;
}

static bool _is_defined(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src" }, unpacked) ;
    return !unpacked[0].isUndefined() ;
}

static bool _empty(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src" }, unpacked) ;
    return unpacked[0].isUndefined() || unpacked[0].isNull() || ( unpacked[0].isString() && unpacked[0].toString().empty() ) ||
           ( unpacked[0].isArray() && unpacked[0].length() == 0 ) ||
           ( unpacked[0].isObject() && unpacked[0].length() == 0 ) ;
}

static bool _iterable(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src" }, unpacked) ;
    return unpacked[0].isArray() ||
           unpacked[0].isObject()  ;
}

static bool _null(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src" }, unpacked) ;
    return unpacked[0].isNull() ;
}

static bool _sequence(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src" }, unpacked) ;
    return unpacked[0].isArray()  ;
}

static bool _map(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src" }, unpacked) ;
    return unpacked[0].isObject()  ;
}

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

    registerFunction("range", range);
    registerFunction("cycle", cycle) ;
    registerFunction("date",  date) ;

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
