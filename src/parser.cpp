#include "parser.hpp"

#include <iostream>
#include <regex>

using namespace std ;

namespace twig {
namespace detail {

bool Parser::parse(DocumentNodePtr node, const string &resourceId ) {
    root_ = node ;
    stack_.push_back(node) ;

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
            else if ( c == '#' ) {
                ++pos_ ;
                eatComment() ;
            }
            else {
                pos_ = cur ;
                auto n = parseRaw(true) ;
                addNode(n) ;
            }
        }
        else {
            pos_ = cur ;
            auto n = parseRaw(false) ;
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

bool Parser::expect(const char *str, bool backtrack) {
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

    if ( backtrack ) {
        pos_ = cur ;
    }
    return true ;
}

void Parser::eatComment()
{
    while ( pos_ ) {
        char c = *pos_++ ;
        if ( c == '#' ) {
            char c = *pos_++ ;
            if ( c == '}' )  return ;
        }
    }

    throwException("unclosed comment") ;

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

bool Parser::parseRegexString(std::string &res)
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
                case 'f':
                case 'n':
                case 'r':
                case 't':
                    res += '\\';
                    res += escape;
                    break ;
                case 'u': {
                    // Preserve unicode escapes in regex patterns, do not decode them as a literal character.
                    res += "\\u";

                    for ( int i = 0 ; i < 4 ; ++i ) {
                        if ( !pos_ ) throwException("End of file while parsing regex string") ;
                        char hex = *pos_++ ;
                        res += hex;
                    }
                } break;
                default:
                    // Preserve any other escaped regex token as-is.
                    res += '\\';
                    res += escape;
                    break;
                }

            } else {
                res += c;
            }
        }
    }

    pos_ = cur ;
    return false ;
}

NodePtr Parser::parseNumber() {
    static std::regex rx_number(R"(^(-?(?:0|[1-9]\d*))(\.\d+)?([eE][+-]?\d+)?)") ;

    Position cur = pos_ ;

    skipSpace() ;

    smatch what ;
    if ( !regex_search(pos_.cursor_, pos_.end_, what, rx_number) ) {
        pos_ = cur ;
        return nullptr ;
    }
    else {
        string val = what[0] ;
        string dec = what[2] ;
        string exp = what[3] ;
        size_t len = val.length() ;
        pos_.cursor_ += len ;
        pos_.column_ += len ;
        if ( dec.empty() && exp.empty() ) {
          try {
              int64_t i = stoll(val) ;
              return NodePtr(new LiteralNode(i)) ;
          } catch ( std::invalid_argument & ) {
              pos_ = cur ;
              return nullptr ;
          } catch ( std::out_of_range & ) {
              pos_ = cur ;
              return nullptr ;
          }
        } else {
            try {
                double f = stod(val) ;
                return NodePtr(new LiteralNode(f)) ;
             } catch ( std::invalid_argument & ) {
                pos_ = cur ;
                return nullptr ;
            } catch ( std::out_of_range & ) {
                pos_ = cur ;
                return nullptr ;
            }
        }
    }
}
/**
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
}*/

bool Parser::parseIdentifier(string &name)
{
    string part ;
    while ( parseName(part) ) {
        name.append(part) ;
        Position cur = pos_ ;
        if ( pos_ && *pos_ == '.' ) {
            ++pos_ ;
            if ( pos_ && *pos_ == '.' ) {
                pos_ = cur ;
                break ;
            } else {
                name += '.' ;
            }
        }
        else break ;
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

    if ( expect('-') )
        trimWhiteBefore() ;

    parseControlTagDeclaration() ;


    if ( expect('-' ))
        trimWhiteAfter() ;

    if ( !expect("%}") )
        throwException("Tag not closed while parsing block") ;

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
                addMacroBlock(name, n) ;
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
            e = parseFilterExpression() ;
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

        auto e = parseAssignment() ;
        if ( e ) {
            auto n = make_shared<AssignmentBlockNode>(e) ;
            addNode(n) ;
            pushControlBlock(n) ;
        } else throwException("invalid set block assignment") ;
       
    }  else if ( expect("endset") ) {
       popControlBlock("set") ;
    }

}

void Parser::parseSubstitutionTag() {
    if ( expect('-') )
        trimWhiteBefore();

    NodePtr expr = parseExpression();
    if ( !expr )
        throwException("missing expression") ;

    bool trim_after = false ;
    if ( expect('-') )
        trim_after = true ;

    if ( !expect("}}") )
        throwException("Tag not closed while parsing substitution tag") ;

    auto n = make_shared<SubstitutionBlockNode>(expr) ;

    addNode(n) ;

    if ( trim_after )
        trimWhiteAfter();



}

ContentNodePtr Parser::parseRaw(bool br) {
    string res ;

    if ( br ) res += '{' ;

    while ( pos_ ) {
        char c = *pos_ ;
        if ( isspace(c) && trim_next_raw_block_ ) { ++pos_ ; continue ; }
        if ( c == '{' ) break ;
        else res += c ;
        ++pos_ ;
    }

    trim_next_raw_block_ = false ;

    return make_shared<RawTextNode>(res) ;
}


// f = e g | e
// g = '|' name '(' args ')' g

NodePtr Parser::parseFilterExpression()
{
    NodePtr lhs = parsePostfix() ;

    while ( true ) {
         auto g = parseFilterExpressionReminder(lhs) ;
         if ( !g ) break ;
         else lhs = g ;
    }

   return lhs ;
}

NodePtr Parser::parsePostfix()
{
    NodePtr lhs = parsePrimary() ;

    while ( true ) {
        Position cur = pos_ ;
         if ( expect('[') ) {
             auto index = parseExpression() ;
             if ( !index )
                throwException("expected expression after '[' in subscript operator") ;
             if ( !expect(']') )
                throwException("expected ']' in subscript operator") ;
             lhs = NodePtr(new SubscriptIndexingNode(lhs, index)) ;
         } else if ( expect('.') ) {
            if ( *pos_ == '.' ) { // maybe ".." operator
                pos_ = cur ;
                break ;
            }
             string name ;
             if ( !parseName(name) )
                throwException("expected name after '.' in member access operator") ;
             lhs = NodePtr(new AttributeIndexingNode(lhs, name, true)) ;
         } 
         else if ( expect("?.") ) {
             string name ;
             if ( !parseName(name) )
                throwException("expected name after '?.' in member access operator") ;
             lhs = NodePtr(new AttributeIndexingNode(lhs, name, false)) ;
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
                lhs = NodePtr(new InvokeFunctionNode(lhs, std::move(args))) ;
            else
                throwException("missing closing ')'");

        }
            else break ;
    }
  
    return lhs ;
}

NodePtr Parser::parseRangeExpression()
{
    NodePtr lhs ;

    if ( ( lhs = parseExpression()) ) {
        if ( expect("..") ) {
            auto rhs = parseExpression() ;
            if ( !rhs )
                throwException("expecting expression after '..' in range expression") ;
            return NodePtr(new RangeOperatorNode(lhs, rhs)) ;
        } else return lhs ;
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

NodePtr Parser::parseLambda() {
    string name ;
    Position cur = pos_ ;
    if ( parseName(name) ) {
        identifier_list_t args ;
        args.emplace_back(name) ;

        if ( expect("=>") ) {
             auto body = parseTernary() ;

            if ( !body )
                throwException("expected expression in lambda body") ;

            return NodePtr(new LambdaNode(std::move(args), body)) ;
        }
       
    } 
            
    pos_ = cur ; // backtrack
    
    if ( expect("(") ) {
        identifier_list_t args ;
        if ( parseNameList(args) && expect(')') ) {

            if ( expect("=>") ) {
                auto body = parseTernary() ;

                if ( !body )
                    throwException("expected expression in lambda body") ;

                return NodePtr(new LambdaNode(std::move(args), body)) ;
            }
        } 
    } 

    pos_ = cur ; // backtrack
    return nullptr ;
}

NodePtr Parser::parseAssignment() {
    string name ;
    Position cur = pos_ ;
    if ( parseName(name) ) {
        if ( expect('=') ) {
            auto expr = parseExpression() ;
            if ( !expr )
                throwException("expected expression after '=' in assignment") ;
            identifier_list_t args{ name } ;
            return NodePtr(new AssignmentNode(name, expr)) ;
        }
    } else if ( expect('[') ){
        identifier_list_t args ;
        if ( parseKeyList(args) && expect(']') ) {
            if ( expect('=') ) {
                auto expr = parseExpression() ;
                if ( !expr )
                    throwException("expected expression after '=' in assignment") ;

           
                return NodePtr(new AssignmentNode(args, expr)) ;
            }
        }
    } else if ( expect('{') ){
        std::vector<AssignmentNode::KeyAlias> args ;
        if ( parseKeyAliases(args) && expect('}') ) {
            if ( expect('=') ) {
                auto expr = parseExpression() ;
                if ( !expr )
                    throwException("expected expression after '=' in assignment") ;

                return NodePtr(new AssignmentNode(args, expr)) ;
            }
        }
    }
    pos_ = cur ; // backtrack
    return nullptr ;
}

NodePtr Parser::parseExpression() {
    // check for lambda expression first since it can start with an open parenthesis which is also valid for other expressions.
   
    if ( auto lambda = parseLambda() ) return lambda ;

    // test for assignment expression before ternary since the right-hand side of an assignment can be a ternary expression and we want to make sure that we parse it as an assignment and not as a ternary with an assignment in the true branch which is not what we want.

    if ( auto assign = parseAssignment() ) return assign ;

   return parseTernary() ;

}

NodePtr Parser::parseTernary() {
    NodePtr lhs = parseNullCoalescing() ;

    if ( lhs ) {
        if ( expect("?:") ) {
            auto false_expr = parseExpression() ;
            if ( !false_expr )
                throwException("expected expression after '?:' in ternary operator") ;
      
            return NodePtr(new TernaryOperatorNode(lhs, lhs, false_expr)) ;
        }
        else if ( expect('?') ) {
            auto true_expr = parseExpression() ;
            if ( !true_expr )
                throwException("expected expression after ':' in ternary operator") ;
            
            NodePtr false_expr ;
            if ( expect(':') ) {
              false_expr = parseExpression() ;
              if ( !false_expr )throwException("expected ':' in ternary operator") ;
            }
            return NodePtr(new TernaryOperatorNode(lhs, true_expr, false_expr)) ;
        } 
        else return lhs ;
    }

    return nullptr ;
}

NodePtr Parser::parseNullCoalescing() {
    auto left = parseOr();
    if ( expect("??") ) {
        auto right = parseNullCoalescing(); // right-associative
        return NodePtr(new BinaryOperator("??", left, right)) ;
    } else 
        return left ;
}

NodePtr Parser::parseOr() {
    auto left = parseAnd();
    if ( expect("or") || expect("||") ) {
        auto right = parseOr(); // right-associative
        return NodePtr(new BooleanOperator(BooleanOperator::Or, left, right)) ;
    } else 
        return left ;
}

NodePtr Parser::parseAnd() {
    auto left = parseNot();
    if ( expect("and") || expect("&&") ) {
        auto right = parseAnd(); // right-associative
        return NodePtr(new BooleanOperator(BooleanOperator::And, left, right)) ;
    } else 
        return left ;
}

NodePtr Parser::parseNot() {
    if ( expect("not") || expect("!") ) {
        auto expr = parseNot();
        return NodePtr(new UnaryOperator('!', expr)) ;
    } else {
        return parseComparison() ;
    }
}

NodePtr Parser::parseComparison() {
    auto lhs = parseRange() ;

    ComparisonPredicate::Type type ;

    if ( expect("!==" ) )
        type = ComparisonPredicate::NotIdentical ;
    else if ( expect("===") )
        type = ComparisonPredicate::Identical ;
    else if ( expect("!=") )
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
    else if ( expect("matches") ) {
        string rx ;
        if ( parseRegexString(rx) ) {
            return NodePtr(new MatchesNode(lhs, rx, true)) ;
        }
        else           
            throwException("expecting regex string literal after 'matches' in predicate expression") ;
    }
     else if ( expect("not") && expect("in") )
        type = ComparisonPredicate::NotIn ;
     else if ( expect("in") )    
        type = ComparisonPredicate::In ;
    else if ( expect("starts") && expect("with") )
        type = ComparisonPredicate::StartsWith ;
    else if ( expect("ends") && expect("with") )
        type = ComparisonPredicate::EndsWith ;
    else if ( expect("has") ) {
        if ( expect("some") )
            type = ComparisonPredicate::HasSome ;
        else if ( expect("every") )
            type = ComparisonPredicate::HasEvery ;
        else throwException("expected 'some' or 'every' after 'has' in predicate expression") ;
    }
    else
        return lhs ;

    if ( type == ComparisonPredicate::HasSome || type == ComparisonPredicate::HasEvery ) {
        auto rhs = parseLambda() ;
         if ( rhs )
            return NodePtr(new ComparisonPredicate(type, lhs, rhs)) ;
        else
            throwException("expecting arrow function");
    }

    auto rhs = parseRange() ;
    if ( rhs )
        return NodePtr(new ComparisonPredicate(type, lhs, rhs)) ;
    else
        throwException("expecting expression");
}

NodePtr Parser::parseRange() {
    auto lhs = parseConcat() ;
    if ( expect("..") ) {
        auto rhs = parseConcat() ;
        if ( !rhs )
            throwException("expecting expression after '..' in range expression") ;
        return NodePtr(new RangeOperatorNode(lhs, rhs)) ;
    } else return lhs ;
}

NodePtr Parser::parseConcat() {
    auto lhs = parseAddSub() ;
    while ( expect('~') ) {
        auto rhs = parseAddSub() ; 
        lhs = NodePtr(new BinaryOperator("~", lhs, rhs)) ;
    }
    return lhs ;
}

NodePtr Parser::parseAddSub()
{
    NodePtr lhs = parseMulDiv();

    while (true) {
        if (expect('+')) {
            auto rhs = parseMulDiv();
            if (rhs)
                return NodePtr(new BinaryOperator("+", lhs, rhs));
            else
                throwException("expecting expression after '+' in addition operator");
        }
        else if (expect("-}}", true)) { // ambiguity with '-' char
            return lhs;
        }
        else if (expect("-%}", true)) {
            return lhs;
        }
        else if (expect('-')) {
            auto rhs = parseMulDiv();
            if (rhs)
                return NodePtr(new BinaryOperator("-", lhs, rhs));
            else
                throwException("expecting expression after '-' in subtraction operator");
        }
        else
            return lhs;
    }
}

NodePtr Parser::parseMulDiv()
{
    NodePtr lhs = parseTest() ;

    while ( true ) {
        Position cur = pos_ ;
        if ( expect('*') ) {
            auto rhs = parseTest() ;
            if ( rhs )
                lhs = NodePtr(new BinaryOperator("*", lhs, rhs)) ;
            else
                throwException("expecting expression after '*' in multiplication operator") ;
        
         } else if ( expect("//") ) {
            auto rhs = parseTest() ;
            if ( rhs )
                lhs = NodePtr(new BinaryOperator("//", lhs, rhs)) ;
            else
                throwException("expecting expression after '//' in floor division operator") ;
        }
        else if ( expect('/') ) {
            auto rhs = parseTest() ;
            if ( rhs )
                lhs = NodePtr(new BinaryOperator("/", lhs, rhs)) ;
            else
                throwException("expecting expression after '/' in division operator") ;
        }
        else if ( expect('%') ) {
            if ( expect('}') ) {
                pos_ = cur ;
                return lhs ;
            }
            auto rhs = parseTest() ;
            if ( rhs )
                lhs = NodePtr(new BinaryOperator("%", lhs, rhs)) ;
            else
                throwException("expecting expression after '%' in modulus operator") ;
        }
        else
            return lhs ;
    }
     
}

NodePtr Parser::parseTest() {
    auto lhs = parseExponent() ;
    if ( expect("is") ) {

        bool negation = false ;
        if ( expect("not") ) {
            negation = true ;
        }

        string name ;
        
        while ( true ) {
            string part ;
            if ( parseName(part) ) {
                if ( !name.empty() ) name += ' ' ;
                name += part ;
            } else break ;
        }

        if ( name.empty() )
            throwException("test name expected") ;

        auto e = parseExpression() ;
       
        return NodePtr(new TestExpressionNode(lhs, name, e, negation)) ;
    } else return lhs ;
}

NodePtr Parser::parseExponent() {
    auto lhs = parseUnary() ;
    if ( expect("**") ) {
        auto rhs = parseExponent() ; // right-associative
        if ( rhs )
            return NodePtr(new BinaryOperator("**", lhs, rhs)) ;
        else
            throwException("expecting expression after '**' in exponentiation operator") ;
    } else return lhs ;
}

NodePtr Parser::parseUnary() {
    if ( expect("-") ) {
        auto rhs = parseUnary() ; // right-associative
        if ( rhs )
            return NodePtr(new UnaryOperator('-', rhs)) ;
        else
            throwException("expecting expression after '-' in unary minus operator") ;
    } else return parseFilterExpression() ;
}

NodePtr Parser::parseTerm()
{
    NodePtr lhs, rhs ;

    if ( (lhs = parseFactor()) ) {

        if ( expect('*') ) {
            rhs = parseTerm() ;
            return NodePtr(new BinaryOperator("*", lhs, rhs)) ;
        }
        else if ( expect('/') ) {
            rhs = parseTerm() ;
            return NodePtr(new BinaryOperator("/", lhs, rhs)) ;
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

        while ( true ) {
            NodePtr e ;
            if ( expect("...") ) {
                auto pe = parseExpression() ;
                if ( pe ) {
                    e = NodePtr(new SpreadOperator(pe)) ;
                } else throwException("missing expression after spread operator");
            } else {
                e = parseExpression() ;
            }

            if ( e == nullptr ) break ;

            elements.push_back(e);

            if ( !expect(',') ) break ;
        }
        if ( expect(']') )
            return NodePtr(new ArrayNode(std::move(elements))) ;

    }

    return nullptr ;

}

bool Parser::parseKeyValuePair(string &key, NodePtr &val) {
    key.clear() ;
    if ( !parseName(key) && !parseString(key) ) return false ;
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

bool Parser::parseKeyList(identifier_list_t &ids) {
    string name ;
    size_t count = 0 ;

    while ( true ) {
        if ( !parseName(name) ) 
            ids.push_back(string()) ;
        else {
            ids.push_back(name) ;
            count++ ;
        }
        if ( expect(',') ) continue ;
        else break ;
    }

    return count > 0 ;
}

bool Parser::parseKeyAliases(std::vector<AssignmentNode::KeyAlias> &aliases) {
    string key, alias ;
    while ( parseName(key) ) {
        if ( expect(":") ) {
            if ( parseName(alias) ) {
                aliases.emplace_back(AssignmentNode::KeyAlias(key, alias)) ;
            } else return false ;
        } else {
            aliases.emplace_back(AssignmentNode::KeyAlias(key, string())) ;
        }

        if ( expect(',') ) continue ;
        else break ;
    }

    return !aliases.empty() ;
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

  

    if ( auto n = parseNumber() ) {
        return n ;
    }

   

    string lit_s ;
    if ( parseString(lit_s) )
        return NodePtr(new LiteralNode(lit_s)) ;

    if ( auto b = parseBoolean() ) {
        return b ;
    }

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

    if ( expect("...") ) {
        NodePtr pe = parseExpression() ;
        if ( pe ) {
            val = NodePtr(new SpreadOperator(pe)) ;
        } else 
            throwException("identifier needed after spread operator") ;
    } else 
        val = parseExpression() ;

    if ( !val )
        return false ;
    else {
        arg = make_pair(std::string(), val) ;
        return true ;
    }
}

NodePtr Parser::parseVariable() {
    string name ;
    if ( parseIdentifier(name) ) {
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

    try {
        return NodePtr(new MatchesNode(lhs, rx, positive)) ;
    } catch ( std::regex_error &e ) {
        throwException("invalid regular expression: " + string(e.what())) ;
    }
    return NodePtr()  ;

}

void Parser::pushControlBlock(ContainerNodePtr node) {
    stack_.push_back(node) ;
}

void Parser::popControlBlock(const char *tag_name) {


    current_ = nullptr;
    auto it = stack_.rbegin() ;

    while ( it != stack_.rend() ) {

        if ( (*it)->tagName() == tag_name ) {
            stack_.pop_back() ; return ;
        } else if ( !(*it)->shouldClose() ) {
            ++it ; stack_.pop_back() ;
        }
        else throwException("unmatched tag")  ;
    }
}

extern void rtrim(std::string &) ;

void Parser::trimWhiteBefore()
{
    if ( !current_ ) return ;

    if ( RawTextNode *p = dynamic_cast<RawTextNode *>(current_.get()) ) {
        rtrim(p->text_) ;
        if ( p->text_.empty() ) { // erase child
                stack_.back()->children_.pop_back() ;
        }
    }
}

void Parser::trimWhiteAfter() {
    trim_next_raw_block_ = true ;
}

void Parser::addNode(ContentNodePtr node) {
    current_ = node ;

    stack_.back()->addChild(node) ;
}



}}
