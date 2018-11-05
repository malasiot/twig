// json decoder

#include <twig/variant.hpp>

#include <regex>

#include <fstream>

using namespace std ;

namespace twig {
namespace detail {


class JSONParser {
public:
    JSONParser(const string &src): cursor_(src.begin()), end_(src.end()) {}

    bool parse(Variant &val) ;

private:

    string::const_iterator cursor_, end_ ;

private:

    bool parseValue(Variant &val) ;
    bool parseString(Variant &val) ;
    bool parseNumber(Variant &val) ;
    bool parseArray(Variant &val) ;
    bool parseObject(Variant &val) ;
    bool parseBoolean(Variant &val) ;
    bool parseNull(Variant &val) ;
    bool parseKeyValuePair(string &key, Variant &val) ;

    void skipSpace() ;
    bool expect(char c) ;
    bool expect(const char *str) ;
    bool decodeUnicode(uint &cp) ;
    static string unicodeToUTF8(uint cp) ;

    bool throwException(const std::string msg) ;
};


bool JSONParser::parse( Variant &value ) {
    if ( !parseValue(value) ) {
        throwException("Error parsing json value") ;
        return false ;
    }
    return true ;
}

bool JSONParser::parseValue(Variant &val) {
    return
            parseString(val) ||
            parseNumber(val) ||
            parseObject(val) ||
            parseArray(val) ||
            parseBoolean(val) ||
            parseNull(val) ;
}

bool JSONParser::parseString(Variant &val)
{
    string res ;

    if ( !expect("\"") ) return false ;
    while ( cursor_ != end_ ) {
        char c = *cursor_++ ;
        if ( c == '"' ) {
            val = Variant(res) ;
            return true ;
        }
        else if ( c == '\\' ) {
            if ( cursor_ == end_ ) return throwException("End of file while parsing string literal");
            char escape = *cursor_++ ;

            switch (escape) {
            case '"':
                res += '"'; break ;
            case '/':
                res += '/'; break ;
            case '\\':
                res += '\\'; break;
            case 'b':
                res += '\b'; break ;
            case 'f':
                res += '\f'; break ;
            case 'n':
                res += '\n'; break ;
            case 'r':
                res += '\r'; break ;
            case 't':
                res += '\t'; break ;
            case 'u': {
                unsigned int cp ;
                if ( !decodeUnicode(cp) ) return throwException("Error while decoding unicode code point") ;
                res += unicodeToUTF8(cp);
            } break;
            default:
                return throwException("Invalid character found while decoding string literal") ;
            }

        } else {
            res += c;
        }
    }

    return false ;
}

bool JSONParser::parseNumber(Variant &val) {
    static regex rx_number(R"(^-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?)") ;

    skipSpace() ;

    smatch what ;
    if ( !regex_search(cursor_, end_, what, rx_number) ) return false ;
    else {
        string c = what[0] ;
        cursor_ += c.length() ;

        try {
            int64_t number = stoll(c) ;
            val = Variant(number) ;
        }
        catch ( ... ) {
            try {
                float number = stof(c) ;
                val = Variant(number) ;
            }
            catch ( ... ) {
                double number = stod(c) ;
                val = Variant(number) ;
            }
        }


        return true ;
    }



}

bool JSONParser::parseArray(Variant &val)
{
    Variant::Array elements ;

    if ( !expect("[") ) return false ;
    Variant element ;
    while ( 1 ) {
        if ( expect("]") ) {
            val = elements ;
            return true ;
        }
        else if ( !elements.empty() && !expect(",") ) {
            throwException("Expecting ','") ;
            return false ;
        }

        if ( !parseValue(element) ) break ;
        else elements.emplace_back(element) ;
    } ;

    return true ;
}

bool JSONParser::parseObject(Variant &val)
{
    Variant::Object elements ;

    if ( !expect("{") ) return false ;

    string key ;
    Variant element ;

    while ( 1 ) {
        if ( expect("}") ) {
            val = elements ;
            return true ;
        }
        else if ( !elements.empty() && !expect(",") ) {
            throwException("Expecting ','") ;
            return false ;
        }

        if ( !parseKeyValuePair(key, element) ) break ;
        else {
            if ( key.empty() ) key = "__empty__" ;
            elements.insert({key, element}) ;
        }
    }
    return true ;

}

bool JSONParser::parseBoolean(Variant &val)
{
    if ( expect("true") ) {
        val = Variant(true) ;
        return true ;
    }
    else if ( expect("false") ) {
        val = Variant("false") ;
        return true ;
    }

    return false ;

}

bool JSONParser::parseNull(Variant &val)
{
    if ( expect("null") ) {
        val = Variant() ;
        return true ;
    }

    return false ;
}

bool JSONParser::parseKeyValuePair(string &key, Variant &val) {
    Variant keyv ;
    if ( !parseString(keyv) ) return false ;
    key = keyv.toString() ;
    if ( !expect(":") ) return false ;
    if ( !parseValue(val) ) return false ;
    return true ;
}

void JSONParser::skipSpace() {
    while ( cursor_ != end_ ) {
        char c = *cursor_ ;
        if ( isspace(c) ) ++cursor_ ;
        else return ;
   }
}

bool JSONParser::expect(char c) {
    if ( cursor_ != end_ ) {
        if ( *cursor_ == c ) {
            ++cursor_ ;
            return true ;
        }
        return false ;
    }
    return false ;
}

bool JSONParser::expect(const char *str)
{
    const char *c = str ;

    skipSpace() ;
    auto cur = cursor_ ;
    while ( *c != 0 ) {
        if ( !expect(*c) ) {
            cursor_ = cur ;
            return false ;
        }
        else ++c ;
    }
    return true ;
}

// adopted from https://github.com/open-source-parsers/jsoncpp

bool JSONParser::decodeUnicode(uint &cp)
{
    int unicode = 0 ;

    for( uint i=0 ; i<4 ; i++ ) {
        if ( cursor_ == end_ ) return false ;
        char c = *cursor_++ ;
        unicode *= 16 ;
        if ( c >= '0' && c <= '9')
            unicode += c - '0';
        else if (c >= 'a' && c <= 'f')
            unicode += c - 'a' + 10;
        else if (c >= 'A' && c <= 'F')
            unicode += c - 'A' + 10;
        else
            return false ;
    }

    cp = static_cast<unsigned int>(unicode);
    return true;
}

bool JSONParser::throwException(const string msg)
{
    throw JSONParseException(msg) ;
}

/// Converts a unicode code-point to UTF-8.

string JSONParser::unicodeToUTF8(unsigned int cp) {
    string result ;

    if ( cp <= 0x7f ) {
        result.resize(1);
        result[0] = static_cast<char>(cp);
    } else if ( cp <= 0x7FF ) {
        result.resize(2);
        result[1] = static_cast<char>(0x80 | (0x3f & cp));
        result[0] = static_cast<char>(0xC0 | (0x1f & (cp >> 6)));
    } else if ( cp <= 0xFFFF ) {
        result.resize(3);
        result[2] = static_cast<char>(0x80 | (0x3f & cp));
        result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
        result[0] = static_cast<char>(0xE0 | (0xf & (cp >> 12)));
    } else if ( cp <= 0x10FFFF ) {
        result.resize(4);
        result[3] = static_cast<char>(0x80 | (0x3f & cp));
        result[2] = static_cast<char>(0x80 | (0x3f & (cp >> 6)));
        result[1] = static_cast<char>(0x80 | (0x3f & (cp >> 12)));
        result[0] = static_cast<char>(0xF0 | (0x7 & (cp >> 18)));
    }

    return result;
}

}

Variant Variant::fromJSONString(const std::string &src, bool throw_exception) {
    detail::JSONParser parser(src) ;

    Variant val ;
    try {
        parser.parse(val) ;
        return val ;
    }
    catch ( JSONParseException &e ) {
      //  cout << e.what() << endl ;
        if ( throw_exception ) throw e ;
        else return Variant() ;
    }
}

Variant Variant::fromJSONFile(const string &path, bool throw_exception) {
    std::ifstream t(path);
    std::string str((std::istreambuf_iterator<char>(t)),
                     std::istreambuf_iterator<char>());
    return fromJSONString(str, throw_exception) ;
}

}
