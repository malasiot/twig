#include "parser.hpp"

#include <iostream>
#include <regex>

using namespace std ;

namespace twig {
namespace detail {

bool Parser::parse(DocumentNodePtr node, const string &resourceId ) {
    while ( pos_ ) {
        Position cur(pos_) ;
        char c = *pos_ ++ ;
        if ( c == '{' )  {
            Position cur(pos_) ;
            c = *pos_++ ;
            if ( c == '{' ) {
               parseSubstitutionTag() ;
            }
            else if ( c == '%' ) {
               parseControlTag() ;
            }
            else {
                pos_ = cur ;
                parseRaw() ;
            }
        }
        else {
            pos_ = cur ;
            parseRaw() ;
        }

    }

    return true ;
}


void Parser::skipSpace() {
    while ( pos_ ) {
        char c = *pos_ ;
        if ( isspace(c) ) ++pos_ ;
        else return ;
    }
}

bool Parser::decodeUnicode(uint &cp)
{
    int unicode = 0 ;

    for( uint i=0 ; i<4 ; i++ ) {
        if ( !pos_ ) return false ;
        char c = *pos_++ ;
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

void Parser::throwException(const string msg) {
    throw ParseException(msg, pos_.line_, pos_.column_) ;
}

/// Converts a unicode code-point to UTF-8.

string Parser::unicodeToUTF8(unsigned int cp) {
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

bool Parser::parseString(std::string &res)
{
    Position cur = pos_ ;

    skipSpace() ;
    char oc = *pos_++ ;

    if ( oc == '"' || oc == '\'' ) {
        while ( pos_ ) {
            char c = *pos_++ ;
            if ( c == oc ) {
                return true ;
            }
            else if ( c == '\\' ) {
                if ( !pos_ )  throwException("End of file while parsing string literal");
                char escape = *pos_++ ;

                switch (escape) {
                case '"':
                    res += '"'; break ;
                case '\'':
                    res += '\''; break ;
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
                    if ( !decodeUnicode(cp) ) throwException("Error while decoding unicode code point") ;
                    res += unicodeToUTF8(cp);
                } break;
                default:
                    throwException("Invalid character found while decoding string literal") ;
                }

            } else {
                res += c;
            }
        }
    }

    pos_ = cur ;
    return false ;
}

bool Parser::parseNumber(string &val) {
    static std::regex rx_number(R"(^-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?)") ;

    Position cur = pos_ ;

    skipSpace() ;

    smatch what ;
    if ( !regex_search(pos_.cursor_, pos_.end_, what, rx_number) ) {
        pos_ = cur ;
        return false ;
    }
    else {
        val = what[0] ;
        size_t len = val.length() ;
        pos_.cursor_ += len ;
        pos_.column_ += len ;
        return true ;
    }
}

bool Parser::parseInteger(int64_t &val)
{
    string num ;
    if ( parseNumber(num) ) {
        try {
            val = stoll(num) ;
            return true ;
        } catch ( std::invalid_argument & ) {
            return false ;
        } catch ( std::out_of_range & ) {
            return false ;
        }
    }

    return false ;
}

bool Parser::parseDouble(double &val)
{
    string num ;
    if ( parseNumber(num) ) {
        try {
            val = stod(num) ;
            return true ;
        } catch ( std::invalid_argument & ) {
            return false ;
        } catch ( std::out_of_range & ) {
            return false ;
        }
    }

    return false ;
}

bool Parser::parseName(string &name)
{
    static regex rx_name(R"(([a-zA-Z_][a-zA-Z0-9_]*))") ;

    skipSpace() ;

    smatch what ;
    if ( !regex_search(pos_.cursor_, pos_.end_, what, rx_name) ) return false ;
    else {
        name = what[0] ;
        size_t len = name.length() ;
        pos_.cursor_ += len ;
        pos_.column_ += len ;
        return true ;
    }
}

ContainerNodePtr Parser::parseControlTag() {

    string res ;

    bool trim_left = false, trim_right = false  ;

    if ( pos_ && *pos_ == '-' ) {
        ++pos_ ;
        trim_left = true ;
    }

    auto e = parseControlTagDeclaration() ;
    skipSpace() ;

    if ( pos_ && *pos_ == '-' ) {
        ++pos_ ;
        trim_right = true ;
    }

    if ( !expect("%}") )
         throwException("Tag not closed while parsing block") ;

    return e ;
}

ContainerNodePtr Parser::parseControlTagDeclaration() {
    if ( expect("block") ) {
        string name ;
        if ( !parseName(name) )
            throwException("Expected identifier") ;

        return std::make_shared<NamedBlockNode>(name) ;

    } else if ( expect("endblock") ) {
        return nullptr ;

    }
}

ContentNodePtr Parser::parseSubstitutionTag() {
    string res ;

    bool trim_left = false, trim_right = false  ;

    if ( pos_ && *pos_ == '-' ) {
        ++pos_ ;
        trim_left = true ;
    }

    NodePtr expr ;
    if ( !(expr = parseExpression()) ) return nullptr ;

    skipSpace() ;

    if ( pos_ && *pos_ == '-' ) {
        ++pos_ ;
        trim_right = true ;
    }

    if ( !expect("}}") )
         throwException("Tag not closed while parsing substitution tag") ;

    return make_shared<SubstitutionBlockNode>(expr) ;

}

ContentNodePtr Parser::parseRaw() {
    string res ;

    while ( pos_ ) {
        char c = *pos_ ;
        if ( c == '{' ) break ;
        else res += c ;
        ++pos_ ;
    }

    cout << "raw: \"" << res << "\"" << endl ;

    return make_shared<RawTextNode>(res) ;
}

NodePtr Parser::parseExpression()
{
    NodePtr lhs, rhs ;

    if ( !(lhs = parseTerm()) ) return nullptr ;

    while ( pos_ ) {

        skipSpace() ;
        char c = *pos_ ;
        if ( c == '+' ) {
            ++pos_ ;
            rhs = parseTerm() ;
        }
        else if ( c == '-' ) {
            ++pos_ ;
            rhs = parseTerm() ;
        }
        else if ( c == '*' ) {
            ++pos_ ;
            rhs = parseTerm() ;
        }
        else if ( c == '/' ) {
            ++pos_ ;
            rhs = parseTerm() ;
        }
        else break ;

        return NodePtr(new BinaryOperator((int)c, lhs, rhs)) ;
    }



}

NodePtr Parser::parseTerm()
{
    skipSpace() ;

    if ( pos_ && *pos_ == '(' ) {
        NodePtr e = parseExpression() ;;
        if ( !e )
            throwException("expected expression") ;
        if ( !expect(')') )
             throwException("missing closing parentheses") ;

        return e ;
    }
    else if ( pos_ && *pos_ == '-' ) {
        NodePtr e = parseTerm() ;
        if ( !e )
            throwException("expected expression") ;
        return NodePtr(new UnaryOperator('-', e)) ;
    } else {
        double lit_d ;
        int64_t lit_i ;
        string lit_s ;
        if ( parseInteger(lit_i) ) {
            return NodePtr(new LiteralNode(lit_i)) ;
        } else if ( parseDouble(lit_d) ) {
            return NodePtr(new LiteralNode(lit_d)) ;
        } else if ( parseString(lit_s) ) {
            return NodePtr(new LiteralNode(lit_s)) ;
        } else if ( parseName(lit_s) ) {
            return NodePtr(new IdentifierNode(lit_s)) ;
        }
        else return nullptr ;
    }

}


#if 0
bool Parser::parseNumber(double &val) {
    static boost::regex rx_number(R"(^-?(?:0|[1-9]\d*)(?:\.\d+)?(?:[eE][+-]?\d+)?)") ;

    skipSpace() ;

    boost::smatch what ;
    if ( !boost::regex_search(pos_.cursor_, pos_.end_, what, rx_number) ) return false ;
    else {
        string c = what[0] ;
        pos_.cursor_ += c.length() ;

        try {
            int64_t number = boost::lexical_cast<int64_t>(c) ;
            val = Variant(number) ;
        }
        catch ( boost::bad_lexical_cast & ) {
            try {
                float number = boost::lexical_cast<float>(c) ;
                val = Variant(number) ;
            }
            catch ( boost::bad_lexical_cast & ) {
                double number = boost::lexical_cast<double>(c) ;
                val = Variant(number) ;
            }
        }


        return true ;
    }

}

bool Parser::parseArray(Variant &val)
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

}

bool Parser::parseObject(Variant &val)
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


}

bool Parser::parseBoolean(Variant &val)
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

bool Parser::parseNull(Variant &val)
{
    if ( expect("null") ) {
        val = Variant() ;
        return true ;
    }

    return false ;
}

bool Parser::parseKeyValuePair(string &key, Variant &val) {
    Variant keyv ;
    if ( !parseString(keyv) ) return false ;
    key = keyv.toString() ;
    if ( !expect(":") ) return false ;
    if ( !parseValue(val) ) return false ;
    return true ;
}

void Parser::skipSpace() {
    while ( pos_ ) {
        char c = *pos_ ;
        if ( isspace(c) ) ++pos_ ;
        else return ;
    }
}
#endif

bool Parser::expect(char c) {
    if ( pos_ ) {
        if ( *pos_ == c ) {
            ++pos_ ;
            return true ;
        }
        return false ;
    }
    return false ;
}

bool Parser::expect(const char *str)
{
    const char *c = str ;

    skipSpace() ;
    Position cur = pos_ ;
    while ( *c != 0 ) {
        if ( !expect(*c) ) {
            pos_ = cur ;
            return false ;
        }
        else ++c ;
    }
    return true ;
}

}}
