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

    const std::string &src_ ;
    Position pos_ ;

private:

    void skipSpace() ;
    bool expect(char c) ;
    bool expect(const char *str) ;

    bool parseString(std::string &val) ;
    bool parseNumber(std::string &val) ;
    bool parseInteger(int64_t &val) ;
    bool parseDouble(double &val) ;
    bool parseName(std::string &name) ;
    ContainerNodePtr parseControlTag() ;
    ContainerNodePtr parseControlTagDeclaration() ;
    ContentNodePtr parseSubstitutionTag() ;
    ContentNodePtr parseRaw() ;
    NodePtr parseExpression() ;
    NodePtr parseTerm() ;

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
