// twig parser
#include <string>
#include <fstream>

#include "ast.hpp"

class TwigParseException ;


namespace twig {
namespace detail {
class DocumentNode ;
using DocumentNodePtr = std::shared_ptr<DocumentNode> ;

class Parser {
public:
    Parser(const std::string &src): src_(src), pos_(src) {}

    bool parse(DocumentNodePtr node, const std::string &resourceId) ;

private:

    struct Position {
        Position(const std::string &src): cursor_(src.begin()), end_(src.end()) {}

        operator bool () const { return cursor_ != end_ ; }
        char operator * () const { return *cursor_ ; }
        Position& operator++() { advance(); return *this ; }
        Position operator++(int) {
            Position p(*this) ;
            advance() ;
            return p ;
        }

        void advance() {
            // skip new line characters
            column_++ ;

            if ( cursor_ != end_ && *cursor_ == '\n' ) {
                column_ = 1 ; line_ ++ ;
            }
            cursor_ ++ ;
        }

        std::string::const_iterator cursor_, end_ ;
        size_t column_ = 1;
        size_t line_ = 1;
    } ;


    void addMacroBlock(const std::string &name, ContentNodePtr node) {
           root_->macro_blocks_.insert({name, node}) ;
    }

    struct Token {
        enum Type { LiteralString, LiteralDouble, LiteralInteger, LiteralBoolean, Name,
                    Raw, StartSubTag, EndSubTag, StartBlockTag, EndBlockTag } ;
        Position begin_, end_ ;

        Variant val_{} ;
        Type type_ ;

    };

    const std::string &src_ ;
    Position pos_ ;
    std::deque<ContainerNodePtr> stack_ ;
    ContentNodePtr current_ ;
    DocumentNodePtr root_ ;
    bool trim_prev_raw_block_ = false ;
    bool trim_next_raw_block_ = false ;


private:


    void skipSpace() ;
    bool expect(char c) ;
    bool expect(const char *str, bool backtrack=false) ;
    void eatComment() ;

    bool parseString(std::string &val) ;
    bool parseNumber(std::string &val) ;
    bool parseInteger(int64_t &val) ;
    bool parseDouble(double &val) ;
    bool parseName(std::string &name) ;
    bool parseIdentifier(std::string &name) ;
    void parseControlTag() ;
    void parseControlTagDeclaration() ;
    void parseSubstitutionTag() ;
    ContentNodePtr parseRaw(bool) ;
    NodePtr parseExpression() ;
    NodePtr parseTerm() ;
    NodePtr parseFactor() ;
    NodePtr parsePrimary() ;
    NodePtr parseVariable() ;
    NodePtr parseArray() ;
    NodePtr parseObject() ;
    NodePtr parseBoolean() ;
    NodePtr parseNull() ;
    NodePtr parseFilterExpression();
    NodePtr parseFilterExpressionReminder(NodePtr parent);
    bool parseKeyValuePair(std::string &key, NodePtr &val);
    bool parseFunctionArg(key_val_t &arg);
    NodePtr parseConditional();
    NodePtr parseBooleanTerm();
    NodePtr parseBooleanFactor();
    NodePtr parseBooleanPrimary();
    NodePtr parseBooleanPredicate();
    NodePtr parseComparisonPredicate(NodePtr lhs);
    NodePtr parseMatchesPredicate(NodePtr lhs);
    NodePtr parseContainmentPredicate(NodePtr lhs);
    NodePtr parseTestPredicate(NodePtr lhs);
    bool parseNameList(identifier_list_t &ids);
    bool parseImportList(key_alias_list_t &ids);

    void pushControlBlock(ContainerNodePtr node) ;
    void popControlBlock(const char *tag_name);
    void addNode(ContentNodePtr node) ;
    void trimWhiteBefore();
    void trimWhiteAfter();

    bool decodeUnicode(uint &cp) ;
    static std::string unicodeToUTF8(uint cp) ;

    [[noreturn]] void  throwException(const std::string msg) ;

};

/*
 * Conditional = BooleanTerm OR  Conditional
 * BooleanTerm = BooleanFactor AND BooleanTerm
 * BooleanFactor = BooleanPrimary | NOT BooleanPrimary
 * BooleanPrimary = Predicate | RoutineInvocation | '(' Conditional ')'
 * Predicate = Expression | ComparisonPredicate | NullPredicate | LikePredicate
 * ComparisonPredicate = Expression ( '<' | '>' | '==' | '!=' ) Expression
 * LikePredicate = Expression ( 'NOT' )? 'LIKE' Expression
 * NullPredicate Expression 'IS' ('NOT')? 'NULL'
 *  Expression = Term ('+'|'-') Expression ;
 * Term = Factor ('*'|'/') Term
 * Factor = ('+'|'-')? Primary
 * Primary = Literal | Variable | FunctionCall | '(' Expression ')'
 * FunctionCall = Name '(' (Expression)* ')'
 * Variable = Identifier (IndexExpression)*
 * IndexExpr = DotIndexExpr | BracketIndexExpr
 * DotIndexExpr = '.' Identifier
 * BracketIndexExpr = '[' expression ']'
*/


class ParseException {
public:
    ParseException(const std::string &msg, size_t line, size_t col): msg_(msg), line_(line), col_(col) {}

    size_t line_, col_ ;
    std::string msg_ ;
};

}}
