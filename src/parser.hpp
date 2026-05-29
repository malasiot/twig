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
    Parser(const std::string &src, TemplateRenderer *rdr): src_(src), pos_(src), rdr_(rdr) {}

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
    TemplateRenderer *rdr_ ;
    bool trim_prev_raw_block_ = false ;
    bool trim_next_raw_block_ = false ;
    
private:


    void skipSpace() ;
    bool expect(char c) ;
    bool expect(const char *str, bool backtrack=false) ;
    void eatComment() ;

    bool parseString(std::string &val) ;
    bool parseRegexString(std::string &val) ;
    NodePtr parseNumber() ;
    bool parseName(std::string &name) ;
    bool parseIdentifier(std::string &name) ;
    bool parseKeyAliases(std::vector<AssignmentNode::KeyAlias> &aliases);
    void parseControlTag() ;
    bool parseControlTagDeclaration() ;
    void parseSubstitutionTag() ;
    ContentNodePtr parseRaw(bool) ;
    NodePtr parseAssignment() ;
    NodePtr parseLambda() ;
    NodePtr parseUnary() ;
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
    bool parseKeyValuePair(std::string &key, NodePtr &val);
    bool parseFunctionArg(FuncArg &arg);
  
    NodePtr parseTernary() ;
    NodePtr parseNullCoalescing();

    NodePtr parseOr() ;
    NodePtr parseAnd() ;
    NodePtr parseNot() ;
    NodePtr parseComparison() ;

    NodePtr parseRange() ;
    NodePtr parseConcat() ;
    NodePtr parseAddSub() ;
    NodePtr parseMulDiv() ;
    NodePtr parseTest() ;
    NodePtr parseExponent();
    NodePtr parsePostfix() ;
    bool parseExpressionList(std::vector<NodePtr> &l) ;
    bool parseArgumentList(arg_list_t &args) ;
    void parseMacroArgList(key_val_list_t &l) ;
    bool parseFilterChain(std::vector<FilterNodePtr> &filters);
    void consume(const std::string &end_tag, std::string &res);

    bool parseNameList(identifier_list_t &ids);
    bool parseKeyList(identifier_list_t &ids);
    bool parseImportList(key_alias_list_t &ids);

    void pushControlBlock(ContainerNodePtr node) ;
    void popControlBlock(const char *tag_name);
    void addNode(ContentNodePtr node) ;
    void trimWhiteBefore();
    void trimWhiteAfter();

    bool decodeUnicode(uint &cp) ;
    static std::string unicodeToUTF8(uint cp) ;

    [[noreturn]] void throwException(const std::string msg) ;

};


class ParseException {
public:
    ParseException(const std::string &msg, size_t line, size_t col): msg_(msg), line_(line), col_(col) {}

    std::string what() const {
        return std::string("Parse error at line ") + std::to_string(line_) + ", column " + std::to_string(col_) + ": " + msg_ ;
    }

    size_t line_, col_ ;
    std::string msg_ ;
};

}}
