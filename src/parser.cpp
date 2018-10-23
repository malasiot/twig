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
        } else break ;
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
    bool is_embed = false, is_include = false ;
    if ( expect("block") ) {
        string name ;
        if ( !parseName(name) )
            throwException("Expected name") ;

        auto expr = parseExpression() ;

        auto n = std::make_shared<NamedBlockNode>(name, expr) ;
        addNode(n) ;
        pushControlBlock(n) ;


    } else if ( expect("endblock") ) {
        string name ;
        parseName(name) ;
        popControlBlock("block") ;
    } else if ( expect("for") ) {
        identifier_list_t ids ;
        if ( !parseNameList(ids) )
            throwException("identifier list expected") ;
        if ( !expect("in") )
            throwException("'in' keyword expected") ;
        auto e = parseExpression() ;
        if ( !e )
            throwException("missing for loop expression") ;

        NodePtr c ;
        if ( expect("if") )
            c = parseConditional() ;

        auto n = make_shared<ForLoopBlockNode>(std::move(ids), e, c);
        addNode(n) ;
        pushControlBlock(n) ;
    } else if ( expect("endfor") ) {
        popControlBlock("for") ;
    } else if ( expect("else") ) {
        if ( ForLoopBlockNode *p = dynamic_cast<ForLoopBlockNode *>(stack_.back().get()) )
            p->startElseBlock() ;
        else if ( IfBlockNode *p = dynamic_cast<IfBlockNode *>(stack_.back().get()) )
            p->addBlock(nullptr) ;
    } else if ( expect("elif") ) {
        auto e = parseConditional() ;
        if ( !e )
            throwException("expecting conditional expression") ;
        if ( IfBlockNode *p = dynamic_cast<IfBlockNode *>(stack_.back().get()) )
            p->addBlock(e) ;
    } else if ( expect("endif") ) {
        popControlBlock("if") ;
    } else if ( expect("if") ) {
        auto e = parseConditional() ;
        if ( !e )
            throwException("expecting conditional expression") ;
        auto n = make_shared<IfBlockNode>(e);
        addNode(n) ;
        pushControlBlock(n) ;
    } else if ( expect("filter") ) {
        string name ;
        key_val_list_t args ;
        if ( parseName(name) ) {
            if ( expect('(') ) {
                key_val_t arg ;
                while ( parseFunctionArg(arg) ) {
                    args.emplace_back(arg) ;
                    if ( expect(',') ) continue ;
                    else break ;
                }
                if ( !expect(')') )
                    throwException("No closing parenthesis") ;
            }

        } else throwException("filter name expected") ;

        auto n = ContainerNodePtr(new FilterBlockNode(name, std::move(args))) ;
        addNode(n) ;
        pushControlBlock(n) ;
    }  else if ( expect("endfilter") ) {
        popControlBlock("filter") ;
    } else if ( expect("extends") ) {
        auto e = parseExpression() ;
        if ( !e )
            throwException("expecting expression") ;
        auto n = make_shared<ExtensionBlockNode>(e);
        addNode(n) ;
        pushControlBlock(n) ;
    }  else if ( expect("endextends") ) {
        popControlBlock("extends") ;
    }  else if ( expect("macro") ) {

        string name ;

        if ( parseName(name) ) {
            if ( expect('(') ) {
                identifier_list_t ids ;
                parseNameList(ids);
                if ( !expect(')') )
                    throwException("No closing parenthesis") ;
                auto n = make_shared<MacroBlockNode>(name, std::move(ids));
                addNode(n) ;
                pushControlBlock(n) ;
            }
        } else throwException("macro name expected") ;
    } else if ( expect("endmacro") ) {
        popControlBlock("macro") ;
    } else if ( expect("import") ) {
        NodePtr e ;
        if ( expect("self") )
            e = nullptr ;
        else {
            e = parseExpression() ;
            if ( !e )
                throwException("expected expression") ;
        }

        if ( expect("as") ) {
            string name ;
            if ( parseName(name) ) {
                auto n = make_shared<ImportBlockNode>(e, name);
                addNode(n) ;
                pushControlBlock(n) ;
             }
        } else throwException("name expected") ;

    } else if ( expect("from") ) {
        NodePtr e ;
        if ( expect("self") )
            e = nullptr ;
        else {
            e = parseExpression() ;
            if ( !e )
                throwException("expected expression") ;
        }

        if ( expect("import") ) {
            key_alias_list_t imports ;
            if ( parseImportList(imports) ) {
                auto n = make_shared<ImportBlockNode>(e, std::move(imports));
                addNode(n) ;
                pushControlBlock(n) ;
             }
        } else throwException("import definition expected") ;
    } else if ( expect("endimport") ) {
        popControlBlock("import") ;
    } else if ( ( is_embed = expect("embed") )|| ( is_include = expect("include") ) ) {
        auto e = parseExpression() ;
        if ( !e ) throwException("expected expression") ;
        bool ignore_missing = false, with_only = false ;
        if ( expect("ignore") && expect("missing") ) ignore_missing = true ;
        NodePtr w ;
        if ( expect("with") )  {
            w = parseExpression() ;
            if ( !w ) throwException("expected expression") ;
        }
        if ( expect("only"))
            with_only = true ;

        if ( is_embed ) {
            auto n = make_shared<EmbedBlockNode>(e, ignore_missing, w, with_only) ;
            addNode(n) ;
            pushControlBlock(n) ;
        }
        else {
            auto n = make_shared<IncludeBlockNode>(e, ignore_missing, w, with_only) ;
            addNode(n) ;
        }
    } else if ( expect("endembed") ) {
        popControlBlock("embed") ;
    }  else if ( expect("endinclude") ) {
        popControlBlock("include") ;
    } else if ( expect("autoescape") ) {
        string mode = "html";
        if ( expect("false") )
            mode = "no" ;
        else if ( parseString(mode)) ;
        auto n = make_shared<AutoEscapeBlockNode>(mode) ;
        addNode(n) ;
        pushControlBlock(n) ;
    } else if ( expect("endautoescape") ) {
        popControlBlock("autoescape") ;
    } else if ( expect("set") ) {
        string id ;
        if ( parseName(id) ) {
            if ( expect("=") ) {
                auto e = parseExpression() ;
                if ( e )  {
                    auto n = make_shared<AssignmentBlockNode>(id, e) ;
                    addNode(n) ;
                } else throwException("expecting expression") ;
            } else throwException("expected '='") ;
        } else throwException("expected identifier name") ;
    }  else if ( expect("endset") ) {
       popControlBlock("set") ;
    }

}

ContentNodePtr Parser::parseSubstitutionTag() {
    string res ;

    bool trim_left = false, trim_right = false  ;

    if ( expect('-') )
        trim_left = true ;

    NodePtr expr = parseFilterExpression();
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


// f = e g | e
// g = '|' name '(' args ')' g

NodePtr Parser::parseFilterExpression()
{
    NodePtr lhs ;

    if ( ( lhs = parseConditional())) {
        auto g = parseFilterExpressionReminder(lhs) ;
        if( !g ) return lhs ;
        else return g ;
    }

    return nullptr ;
}

NodePtr Parser::parseFilterExpressionReminder(NodePtr parent)
{
    key_val_list_t args ;

    if ( expect('|') ) {
        string name ;
        if ( parseName(name) ) {
            if ( expect('(') ) {

                key_val_t arg ;
                while ( parseFunctionArg(arg) ) {
                    args.emplace_back(arg) ;
                    if ( expect(',') ) continue ;
                    else break ;
                }
                if ( !expect(')') )
                    throwException("No closing parenthesis") ;
            }

        } else throwException("filter name expected") ;

        auto n = NodePtr(new InvokeFilterNode(parent, name, std::move(args))) ;
        auto e = parseFilterExpressionReminder(n) ;
        if ( e ) return e ;
        else return n;
    }
    else return nullptr ;
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

bool Parser::parseNameList(identifier_list_t &ids) {
    string name ;
    while ( parseName(name) ) {
        ids.push_back(name) ;
        if ( expect(',') ) continue ;
        else break ;
    }

    return !ids.empty() ;
}

bool Parser::parseImportList(key_alias_list_t &ids) {
    string key, alias ;
    while ( parseName(key) ) {
        if ( expect("as") ) {
            if ( parseName(alias) ) {
                ids.emplace_back(make_pair(key, alias)) ;
            } else throwException("name expected") ;
        } else {
            ids.emplace_back(make_pair(key, string())) ;
        }

        if ( expect(',') ) continue ;
        else break ;
    }

    return !ids.empty() ;
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

NodePtr Parser::parseConditional()
{
    NodePtr lhs, rhs ;

    if ( (lhs = parseBooleanTerm()) ) {
        if ( expect("||") ) {
            rhs = parseConditional() ;
            return NodePtr(new BooleanOperator(BooleanOperator::Or, lhs, rhs)) ;
        }
        else return lhs ;
    }

    return nullptr ;
}

NodePtr Parser::parseBooleanTerm()
{
    NodePtr lhs, rhs ;

    if ( (lhs = parseBooleanFactor()) ) {
        if ( expect("&&") ) {
            rhs = parseBooleanTerm() ;
            return NodePtr(new BooleanOperator(BooleanOperator::And, lhs, rhs)) ;
        }
        else return lhs ;
    }

    return nullptr ;
}

NodePtr Parser::parseBooleanFactor() {
    bool negative = false ;
    if ( expect('!') )
        negative = true ;

    auto e = parseBooleanPrimary() ;

    if ( e ) {
        if ( negative )
            return NodePtr(new BooleanNegationOperator(e)) ;
        else
            return e ;
    } else
        return nullptr ;
}


NodePtr Parser::parseBooleanPrimary() {
    auto e = parseBooleanPredicate() ;
    if ( e ) return e ;

    if ( expect('(') ) {
        auto e = parseConditional() ;
        if ( !e ) throwException("expected expression") ;
        if ( !expect(')') )
            throwException("closing parenthesis missing") ;
        return e ;
    }

    return nullptr ;
}

NodePtr Parser::parseBooleanPredicate() {

    auto e = parseExpression() ;
    if ( !e ) return nullptr ;

    NodePtr p ;
    p = parseComparisonPredicate(e) ;
    if ( p ) return p ;

    p = parseContainmentPredicate(e) ;
    if ( p ) return p ;

    p = parseMatchesPredicate(e) ;
    if ( p ) return p ;

    p = parseTestPredicate(e) ;
    if ( p ) return p ;

    return e ;

}

NodePtr Parser::parseComparisonPredicate(NodePtr lhs) {
    ComparisonPredicate::Type type ;

    if ( expect("!=") )
        type = ComparisonPredicate::NotEqual ;
    else if ( expect("==") )
        type = ComparisonPredicate::Equal ;
    else if ( expect('<') )
        type = ComparisonPredicate::Less ;
    else if ( expect('>') )
        type = ComparisonPredicate::Greater ;
    else if ( expect(">=") )
        type = ComparisonPredicate::GreaterOrEqual ;
    else if ( expect("<=") )
        type = ComparisonPredicate::LessOrEqual ;
    else
        return nullptr ;

    auto rhs = parseExpression() ;
    if ( rhs )
        return NodePtr(new ComparisonPredicate(type, lhs, rhs)) ;
    else
        throwException("expecting expression");
}

NodePtr Parser::parseContainmentPredicate(NodePtr lhs) {
    bool positive = true ;
    if ( expect("not") )
        positive = false ;

    if ( expect("in") ) {
        auto rhs = parseExpression() ;
        if ( rhs )
            return NodePtr(new ContainmentNode(lhs, rhs, positive)) ;
        else
        throwException("expecting expression");
    }

    return nullptr ;
}

NodePtr Parser::parseTestPredicate(NodePtr lhs) {

    if ( expect("is") ) {

        bool positive = true ;
        if ( expect("not") )
            positive = false ;

        key_val_list_t args ;
        string name ;

        if ( parseName(name) ) {
            if ( expect('(') ) {

                key_val_t arg ;
                while ( parseFunctionArg(arg) ) {
                    args.emplace_back(arg) ;
                    if ( expect(',') ) continue ;
                    else break ;
                }
                if ( !expect(')') )
                    throwException("No closing parenthesis") ;
            }

            return NodePtr(new InvokeTestNode(lhs, name, std::move(args), positive)) ;
        } else throwException("function name expected") ;
    }

    return nullptr ;
}

NodePtr Parser::parseMatchesPredicate(NodePtr lhs) {

    bool positive = true ;

    if ( expect('~') )
        positive = true ;
    else if ( expect("!~") )
        positive = false ;
    else
        return nullptr ;

    string rx ;
    if ( !parseString(rx) )
        throwException("expecting regular expression literal") ;

    return NodePtr(new MatchesNode(lhs, rx, positive))  ;

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
