#include "parser.hpp"

#include <iostream>
#include <regex>

using namespace std ;

namespace twig {
namespace detail {

bool Parser::parse(DocumentNodePtr node, const string &resourceId ) {
    stack_.push_back(node) ;

    while ( pos_ ) {
        Position cur(pos_) ;
        char c = *pos_ ++ ;
        if ( c == '{' )  {
            Position cur(pos_) ;
            c = *pos_++ ;
            if ( c == '{' ) {
               auto n = parseSubstitutionTag() ;
               addNode(n) ;

            }
            else if ( c == '%' ) {
               parseControlTag() ;
            }
            else {
                pos_ = cur ;
                auto n = parseRaw() ;
                addNode(n) ;
            }
        }
        else {
            pos_ = cur ;
            auto n = parseRaw() ;
            addNode(n) ;
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



bool Parser::expect(char c) {
    skipSpace() ;

    if ( pos_ && *pos_ == c ) {
        ++pos_ ;
        return true ;
    }

    return false ;
}

bool Parser::expect(const char *str) {
    skipSpace() ;

    const char *c = str ;

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

bool Parser::parseIdentifier(string &name)
{
    string part ;
    while ( parseName(part) ) {
        name.append(part) ;
        if ( expect('.') ) {
            name += '.' ;
            continue ;
        }
    }

    return !name.empty() ;

}

bool Parser::parseName(string &name)
{
    static regex rx_name(R"(^([a-zA-Z_][a-zA-Z0-9_]*))") ;

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

void  Parser::parseControlTag() {

    string res ;

    bool trim_left = false, trim_right = false  ;

    if ( expect('-') )
        trim_left = true ;

    parseControlTagDeclaration() ;

    if ( expect('-' ))
        trim_right = true ;

    if ( !expect("%}") )
         throwException("Tag not closed while parsing block") ;

 //   e->setTrimLeft(trim_left) ;
 //   e->setTrimRight(trim_right) ;
}

void Parser::parseControlTagDeclaration() {
    if ( expect("block") ) {
        string name ;
        if ( !parseName(name) )
            throwException("Expected name") ;

        auto expr = parseExpression() ;

        auto n = std::make_shared<NamedBlockNode>(name, expr) ;
        addNode(n) ;
        pushControlBlock(n) ;


    } else if ( expect("endblock") ) {
        popControlBlock("block") ;
    }
}

ContentNodePtr Parser::parseSubstitutionTag() {
    string res ;

    bool trim_left = false, trim_right = false  ;

    if ( expect('-') )
        trim_left = true ;

    NodePtr expr = parseExpression();
    if ( !expr )
        throwException("missing expression") ;

    if ( expect('-') )
        trim_right = true ;

    if ( !expect("}}") )
         throwException("Tag not closed while parsing substitution tag") ;

    return make_shared<SubstitutionBlockNode>(expr, trim_left, trim_right) ;

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

    if ( (lhs = parseTerm()) ) {

        if ( expect('+') ) {
            rhs = parseExpression() ;
            return NodePtr(new BinaryOperator('+', lhs, rhs)) ;
        }
        else if ( expect('-') ) {
            rhs = parseExpression() ;
            return NodePtr(new BinaryOperator('-', lhs, rhs)) ;
        }
        else return lhs ;
    }

    return nullptr ;
}

NodePtr Parser::parseTerm()
{
    NodePtr lhs, rhs ;

    if ( (lhs = parseFactor()) ) {

        if ( expect('*') ) {
            rhs = parseTerm() ;
            return NodePtr(new BinaryOperator('*', lhs, rhs)) ;
        }
        else if ( expect('/') ) {
            rhs = parseTerm() ;
            return NodePtr(new BinaryOperator('/', lhs, rhs)) ;
        }
        else return lhs ;
    }

    return nullptr ;



}
NodePtr Parser::parseFactor()
{
    bool negative = false ;
    if ( expect('-') )
        negative = true ;
    else if ( expect('+') )
        negative = false ;

    NodePtr e = parsePrimary() ;

    if ( e ) {
        if ( negative )
            return NodePtr(new UnaryOperator('-', e)) ;
        else
            return e ;
    } else
        return nullptr ;
}


NodePtr Parser::parseArray()
{
    Variant::Array elements ;

    if ( expect('[') ) {
        std::vector<NodePtr> elements ;

        auto e = parseExpression() ;
        while ( e ) {
            elements.push_back(e);

            if ( expect(',') ) {
                e = parseExpression() ;
                if ( !e )
                    throwException("expression required");
            } else e = nullptr ;
        }
        if ( expect(']') )
          return NodePtr(new ArrayNode(std::move(elements))) ;

    }

    return nullptr ;

}

bool Parser::parseKeyValuePair(string &key, NodePtr &val) {
    key.clear() ;
    if ( !parseString(key) ) return false ;
    if ( !expect(":") ) throwException("expected ':'") ;
    val = parseExpression() ;
    if ( !val ) throwException("expected expression") ;
    return true ;
}

NodePtr Parser::parseObject()
{

    if ( expect('{') ) {
        std::map<std::string, NodePtr> elements ;

        string key ;
        NodePtr e ;

        while ( parseKeyValuePair(key, e) ) {
            elements.emplace(key, e);
            if ( expect(',') ) continue ;
            else break ;
        }
        if ( expect('}') )
          return NodePtr(new DictionaryNode(std::move(elements))) ;

    }

    return nullptr ;

}

NodePtr Parser::parseBoolean()
{
    if ( expect("true") )
        return NodePtr(new LiteralNode(true)) ;

    if ( expect("false") )
        return NodePtr(new LiteralNode(false)) ;

    return nullptr ;
}

NodePtr Parser::parseNull()
{
    if ( expect("null") )
        return NodePtr(new LiteralNode(Variant::null()));

    return nullptr ;
}


NodePtr Parser::parsePrimary() {


    int64_t lit_i ;
    if ( parseInteger(lit_i) )
        return NodePtr(new LiteralNode(lit_i)) ;

    double lit_d ;
    if ( parseDouble(lit_d) )
        return NodePtr(new LiteralNode(lit_d)) ;

    string lit_s ;
    if ( parseString(lit_s) )
        return NodePtr(new LiteralNode(lit_s)) ;

    auto a = parseArray() ;
    if ( a ) return a ;

    auto o = parseObject() ;
    if ( o ) return o ;

    if ( expect('(') ) {
        NodePtr e = parseExpression() ;
        if ( !e )
            throwException("expected expression") ;
        if ( !expect(')') )
             throwException("missing closing parentheses") ;

        return e ;
    }

    NodePtr e = parseVariable() ;
    if ( e ) return e ;

    return nullptr ;
}

bool Parser::parseFunctionArg(key_val_t &arg) {
    string key ;
    NodePtr val ;

    if ( parseName(key) ) {
        if ( expect('=') ) {
            val = parseExpression() ;
        }
        else val = NodePtr(new IdentifierNode(key)) ;
    } else {
       val = parseExpression() ;
    }

    if ( !val )
        throwException("function argument parse error") ;

    arg = make_pair(key, val) ;
    return true ;
}

NodePtr Parser::parseVariable() {
    string name ;

    if ( parseIdentifier(name) ) {
        if ( expect('[') ) {
            auto e = parseExpression() ;
            if ( e == nullptr )
                throwException("expecting expression") ;
            if ( expect(']') )
                return NodePtr(new SubscriptIndexingNode(name, e)) ;
        }
        else if ( expect('(') ) {


            key_val_list_t args ;
            key_val_t arg ;
            while ( parseFunctionArg(arg) ) {
                args.push_back(arg);

                if ( expect(',') ) continue ;
                else break ;
            }
            if ( expect(')') )
                return NodePtr(new InvokeFunctionNode(name, std::move(args))) ;
            else
                throwException("missing closing ')'");

        }
        return NodePtr(new IdentifierNode(name)) ;
    }

    return nullptr ;
}

void Parser::pushControlBlock(ContainerNodePtr node) {
    stack_.push_back(node) ;
}

void Parser::popControlBlock(const char *tag_name) {
    auto it = stack_.rbegin() ;

     while ( it != stack_.rend() ) {
         auto n = *it ;
         if ( (*it)->tagName() == tag_name ) {
             stack_.pop_back() ; return ;
         } else if ( !(*it)->shouldClose() ) {
             ++it ; stack_.pop_back() ;
         }
         else throwException("unmatched tag")  ;
     }
}

void Parser::addNode(ContentNodePtr node) {
    stack_.back()->addChild(node) ;
}



}}
