#include <twig/functions.hpp>
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

using namespace std ;

namespace twig {

Variant FunctionFactory::invoke(const string &name, const Variant &args, Context &ctx)
{
    auto it = functions_.find(name) ;
    if ( it == functions_.end() )
        throw TemplateRuntimeException("Unknown function or filter: " + name) ;

    return (it->second)(args, ctx) ;
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

static string to_lower_copy(const string &src) {
    string tmp = src ;
    std::transform(tmp.begin(), tmp.end(), tmp.begin(), [](unsigned char c){ return std::tolower(c); });
    return tmp ;
}

static string php_date_format_to_strftime(const string &format) {
    static const std::map<char, string> token_map = {
        {'d', "%d"}, {'D', "%a"}, {'j', "%-d"}, {'l', "%A"}, {'F', "%B"},
        {'m', "%m"}, {'M', "%b"}, {'n', "%-m"}, {'Y', "%Y"}, {'y', "%y"},
        {'H', "%H"}, {'h', "%I"}, {'G', "%-H"}, {'g', "%-I"},
        {'i', "%M"}, {'s', "%S"}, {'a', "%p"}, {'A', "%p"}, {'U', "%s"}
    };

    string result ;
    result.reserve(format.size() * 2);

    for ( size_t i = 0 ; i < format.size() ; ++i ) {
        char c = format[i] ;
        if ( c == '\\' && i + 1 < format.size() ) {
            result.push_back(format[++i]) ;
            continue ;
        }

        auto it = token_map.find(c) ;
        if ( it != token_map.end() ) {
            result.append(it->second) ;
        } else {
            result.push_back(c) ;
        }
    }

    return result ;
}

static bool parse_fixed_date_formats(const string &src, std::tm &tm) {
    static const vector<string> formats = {
        "%Y-%m-%d %H:%M:%S",
        "%Y-%m-%d %H:%M",
        "%Y-%m-%d",
        "%Y/%m/%d %H:%M:%S",
        "%Y/%m/%d %H:%M",
        "%Y/%m/%d",
        "%d.%m.%Y %H:%M:%S",
        "%d.%m.%Y %H:%M",
        "%d.%m.%Y",
        "%b %d %Y %H:%M:%S",
        "%b %d %Y %H:%M",
        "%b %d %Y",
        "%B %d %Y %H:%M:%S",
        "%B %d %Y %H:%M",
        "%B %d %Y"
    };

    for ( const auto &fmt: formats ) {
        std::memset(&tm, 0, sizeof(tm));
        char *end = strptime(src.c_str(), fmt.c_str(), &tm) ;
        if ( end && *end == '\0' ) {
            tm.tm_isdst = -1 ;
            return true ;
        }
    }

    return false ;
}

static bool apply_relative_offset(time_t &base, int64_t amount, const string &unit) {
    auto tp = std::chrono::system_clock::from_time_t(base) ;
    string lower_unit = to_lower_copy(unit) ;

    if ( lower_unit == "second" || lower_unit == "seconds" ) {
        tp += std::chrono::seconds(amount) ;
    } else if ( lower_unit == "minute" || lower_unit == "minutes" ) {
        tp += std::chrono::minutes(amount) ;
    } else if ( lower_unit == "hour" || lower_unit == "hours" ) {
        tp += std::chrono::hours(amount) ;
    } else if ( lower_unit == "day" || lower_unit == "days" ) {
        tp += std::chrono::hours(amount * 24) ;
    } else if ( lower_unit == "week" || lower_unit == "weeks" ) {
        tp += std::chrono::hours(amount * 24 * 7) ;
    } else if ( lower_unit == "month" || lower_unit == "months" ) {
        std::tm tm = *std::localtime(&base) ;
        tm.tm_mon += static_cast<int>(amount) ;
        tm.tm_isdst = -1 ;
        base = std::mktime(&tm) ;
        return true ;
    } else if ( lower_unit == "year" || lower_unit == "years" ) {
        std::tm tm = *std::localtime(&base) ;
        tm.tm_year += static_cast<int>(amount) ;
        tm.tm_isdst = -1 ;
        base = std::mktime(&tm) ;
        return true ;
    } else {
        return false ;
    }

    base = std::chrono::system_clock::to_time_t(tp) ;
    return true ;
}

static time_t strtotime(const string &src) {
    string lower_src = to_lower_copy(src) ;
    if ( lower_src.empty() || lower_src == "now" )
        return time(nullptr) ;

    if ( lower_src == "today" ) {
        auto now = time(nullptr) ;
        std::tm tm = *std::localtime(&now) ;
        tm.tm_hour = 0 ;
        tm.tm_min = 0 ;
        tm.tm_sec = 0 ;
        tm.tm_isdst = -1 ;
        return std::mktime(&tm) ;
    }

    if ( lower_src == "tomorrow" ) {
        auto now = time(nullptr) ;
        std::tm tm = *std::localtime(&now) ;
        tm.tm_mday += 1 ;
        tm.tm_hour = 0 ;
        tm.tm_min = 0 ;
        tm.tm_sec = 0 ;
        tm.tm_isdst = -1 ;
        return std::mktime(&tm) ;
    }

    if ( lower_src == "yesterday" ) {
        auto now = time(nullptr) ;
        std::tm tm = *std::localtime(&now) ;
        tm.tm_mday -= 1 ;
        tm.tm_hour = 0 ;
        tm.tm_min = 0 ;
        tm.tm_sec = 0 ;
        tm.tm_isdst = -1 ;
        return std::mktime(&tm) ;
    }

    bool all_digits = !src.empty() && (std::all_of(src.begin(), src.end(), ::isdigit) || (src[0] == '-' && src.size() > 1 && std::all_of(src.begin() + 1, src.end(), ::isdigit)));
    if ( all_digits ) {
        try {
            return static_cast<time_t>(std::stoll(src)) ;
        } catch (...) {
        }
    }

    std::tm tm = {} ;
    if ( parse_fixed_date_formats(src, tm) ) {
        return std::mktime(&tm) ;
    }

    std::regex rel_re(R"(^([+-]?\d+)\s*(second|minute|hour|day|week|month|year)s?$)", std::regex::icase) ;
    std::smatch match ;
    if ( std::regex_match(lower_src, match, rel_re) ) {
        time_t base = time(nullptr) ;
        int64_t value = stoll(match[1].str()) ;
        if ( apply_relative_offset(base, value, match[2].str()) )
            return base ;
    }

    std::istringstream in(src) ;
    in >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S") ;
    if ( !in.fail() ) {
        tm.tm_isdst = -1 ;
        return std::mktime(&tm) ;
    }

    throw TemplateRuntimeException("Failed to parse date string: " + src) ;
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

static Variant _range(const Variant &args, Context &ctx) {
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

static Variant _cycle(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src", "position?" }, unpacked) ;
    if ( !unpacked[0].isArray() )
        throw TemplateRuntimeException("cycle function expects an array as first argument") ;

    int position = ( unpacked.size() > 1 ) ? unpacked[1].toInteger() : 0 ;
    return unpacked[0].at(position % unpacked[0].length()) ;
}

static Variant _date(const Variant &args, Context &ctx) {
    Variant::Array unpacked ;
    unpack_args(args, { "src?", "format?", "tz?" }, unpacked) ;

    string format = ( unpacked[1].isUndefined() ) ? "F j, Y H:i" : unpacked[1].toString() ;
    string src = ( unpacked[0].isUndefined() ) ? "now" : unpacked[0].toString() ;

    time_t t = strtotime(src) ;
    std::tm tm = *std::localtime(&t) ;
    string strftime_format = php_date_format_to_strftime(format) ;

    char buffer[256] ;
    if ( std::strftime(buffer, sizeof(buffer), strftime_format.c_str(), &tm) == 0 )
        throw TemplateRuntimeException("Failed to format date string") ;

    return string(buffer) ;
}

FunctionFactory::FunctionFactory() {
    registerFunction("join", _join);
    registerFunction("lower", _lower);
    registerFunction("upper", _upper);
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
    registerFunction("cycle", _cycle) ;
    registerFunction("date", _date) ;
}

bool FunctionFactory::hasFunction(const string &name)
{
    return functions_.count(name) ;
}

}
