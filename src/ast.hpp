#ifndef TWIG_AST_HPP
#define TWIG_AST_HPP

#include <variant/variant.hpp>
#include <twig/context.hpp>

#include <memory>
#include <deque>
#include <regex>

class Context ;

namespace twig {
namespace detail {

class Node {
public:
    Node() = default ;
    virtual ~Node() = default ;

    virtual Variant eval(Context &ctx) = 0 ;
};

using NodePtr = std::shared_ptr<Node> ;

class LiteralNode: public Node {
public:

    LiteralNode(const Variant &v): val_(v) {}

    virtual Variant eval(Context &ctx) { return val_ ; }

    Variant val_;
};

struct FuncArg {
    FuncArg() = default ;
    FuncArg(const std::string &name, NodePtr value):
        name_(name), value_(value) {}
    std::string name_ ;
    NodePtr value_ ;
};

using key_val_t = std::pair<std::string, NodePtr> ;
using key_val_list_t = std::vector<key_val_t> ;
using arg_list_t = std::vector<FuncArg> ;
using identifier_list_t = std::deque<std::string> ;
using key_alias_t = std::pair<std::string, std::string> ;
using key_alias_list_t = std::deque<key_alias_t> ;

class ValueNode: public Node {
public:

    ValueNode(NodePtr &l): val_(l) {}

    Variant eval(Context &ctx) { return val_->eval(ctx) ; }

    NodePtr val_ ;
};

class IdentifierNode: public Node {
public:
    IdentifierNode(const std::string name): name_(name) {}

    Variant eval(Context &ctx) ;

    const std::string name() const { return name_ ; }

private:
    std::string name_ ;
};

class SpreadOperator: public Node {
public:
    SpreadOperator(NodePtr rhs): rhs_(rhs) {}

    Variant eval(Context &ctx) ;
public:
   NodePtr rhs_ ;
};

class ArrayNode: public Node {
public:
    ArrayNode() = default ;
    ArrayNode(const std::vector<NodePtr> &&elements): elements_(elements) {}

    Variant eval(Context &ctx) ;

private:

    std::vector<NodePtr> elements_ ;
};

class ContainmentNode: public Node {
public:
    ContainmentNode(NodePtr lhs, NodePtr rhs, bool positive):
        lhs_(lhs), rhs_(rhs), positive_(positive) {}

    Variant eval(Context &ctx) ;

private:
    NodePtr lhs_, rhs_ ;
    bool positive_ ;
};


class MatchesNode: public Node {
public:
    MatchesNode(NodePtr lhs, const std::string &rx, bool positive) ;

    Variant eval(Context &ctx) ;

private:
    NodePtr lhs_ ;
    std::regex rx_ ;
    bool positive_ ;
};

class DictionaryNode: public Node {
public:

    DictionaryNode() = default ;
    DictionaryNode(const std::map<std::string, NodePtr> && elements): elements_(elements) {}

    Variant eval(Context &ctx) ;

private:
    std::map<std::string, NodePtr> elements_ ;
};


class SubscriptIndexingNode: public Node {
public:
    SubscriptIndexingNode(NodePtr array, NodePtr index): array_(array), index_(index) {}

    Variant eval(Context &ctx) ;

private:
    NodePtr array_ ;
    NodePtr index_ ;
};

class AttributeIndexingNode: public Node {
public:
    AttributeIndexingNode(const NodePtr &dict, const std::string &key, bool except_on_null): 
    dict_(dict), key_(key), except_on_null_(except_on_null) {}
     AttributeIndexingNode(const NodePtr &dict, NodePtr key_node): dict_(dict), key_node_(key_node) {}

    Variant eval(Context &ctx) ;

private:
    NodePtr dict_ ;
    std::string  key_ ;
    NodePtr key_node_ ;
    bool except_on_null_ ;
};

class BinaryOperator: public Node {
public:
    BinaryOperator(const std::string &op, NodePtr lhs, NodePtr rhs): op_(op), lhs_(lhs), rhs_(rhs) {}

    Variant eval(Context &ctx) ;

private:
    std::string op_ ;
    NodePtr lhs_, rhs_ ;
};


class BooleanOperator: public Node {
public:
    enum Type { And, Or } ;

    BooleanOperator(Type op, NodePtr lhs, NodePtr rhs): op_(op), lhs_(lhs), rhs_(rhs) {}

    Variant eval(Context &ctx) ;
private:
    Type op_ ;
    NodePtr lhs_, rhs_ ;
};

class RangeOperatorNode: public Node {
public:
    RangeOperatorNode(NodePtr lhs, NodePtr rhs): lhs_(lhs), rhs_(rhs) {}

    Variant eval(Context &ctx) ;

private:
    NodePtr lhs_, rhs_ ;
};


class BooleanNegationOperator: public Node {
public:
    BooleanNegationOperator(NodePtr node): node_(node) {}

    Variant eval(Context &ctx) ;
private:
    NodePtr node_ ;
};

class UnaryOperator: public Node {
public:

    UnaryOperator(char op, NodePtr rhs): op_(op), rhs_(rhs) {}

    Variant eval(Context &ctx) ;
private:
    char op_ ;
    NodePtr rhs_ ;
};


class AssignmentNode: public Node {
public:
    enum Type { SingleVariable, ArrayDestructring, DictionaryDestructuring } ;
    struct KeyAlias {
        KeyAlias(const std::string &k, const std::string &a): key_(k), alias_(a) {}
        std::string key_ ;
        std::string alias_ ;
    } ;

    AssignmentNode(const std::string &name, NodePtr rhs): args_({name}), rhs_(rhs), type_(SingleVariable) {}
    AssignmentNode(const identifier_list_t &args, NodePtr rhs): args_(args), rhs_(rhs), type_(ArrayDestructring) {}
    AssignmentNode(const std::vector<KeyAlias> &args, NodePtr rhs): dict_args_(args), rhs_(rhs), type_(DictionaryDestructuring) {}

    Variant eval(Context &ctx) ;
private:
    identifier_list_t args_ ;
    std::vector<KeyAlias> dict_args_ ;
    NodePtr rhs_ ;
    Type type_ ;
};  

class ComparisonPredicate: public Node {
public:
    enum Type { Equal, NotEqual, Less, Greater, LessOrEqual, GreaterOrEqual, 
        NotIdentical, Identical, In, NotIn, HasSome, HasEvery, 
        StartsWith, EndsWith, Matches } ;

    ComparisonPredicate(Type op, NodePtr lhs, NodePtr rhs): op_(op), lhs_(lhs), rhs_(rhs) {}

    Variant eval(Context &ctx) ;

private:
    Type op_ ;
    NodePtr lhs_, rhs_ ;
};

class TestExpressionNode: public Node {
public:
  
    TestExpressionNode(NodePtr lhs, const std::string &name, NodePtr args, bool positive): 
        lhs_(lhs), name_(name), args_(args),  positive_(positive) {}

    Variant eval(Context &ctx) ;

private:
    std::string name_ ;
    NodePtr lhs_, args_ ;
    bool positive_ ;
};

class TernaryNode: public Node {
public:
    TernaryNode(NodePtr cond, NodePtr t, NodePtr f):
        condition_(cond),
        positive_(t), negative_(f) {}

    Variant eval(Context &ctx) ;
private:
    NodePtr condition_, positive_, negative_ ;
};
/*
class ContextNode: public Node {
public:
    ContextNode() {}

    Variant eval(Context &ctx) {
        return ctx.data();
    }
};
*/

class FilterNode {
public:
    FilterNode(const std::string &name, arg_list_t &&args ={}): name_(name),
        args_(args) {}

    std::string name_ ;
    arg_list_t args_ ;
};

using FilterNodePtr = std::shared_ptr<FilterNode> ;


class InvokeFilterNode: public Node {
public:
    InvokeFilterNode(NodePtr target, const std::vector<FilterNodePtr> &filters): target_(target), filters_(filters) {}

    Variant eval(Context &ctx) ;

private:
    NodePtr target_ ;
    std::vector<FilterNodePtr> filters_ ;
};

class InvokeTestNode: public Node {
public:
    InvokeTestNode(NodePtr target, const std::string &name,
                   arg_list_t &&args, bool positive):
        target_(target), name_(name), args_(args), positive_(positive) {}

    Variant eval(Context &ctx) ;

private:
    NodePtr target_ ;
    std::string name_ ;
    arg_list_t args_ ;
    bool positive_ ;
};


class InvokeFunctionNode: public Node {
public:
    InvokeFunctionNode(NodePtr callable, const arg_list_t &&args = {}):
        callable_(callable), args_(args) {}

    Variant eval(Context &ctx) ;


private:
    NodePtr callable_ ;
    arg_list_t args_ ;
};

class DocumentNode ;

class ContentNode {
public:
    ContentNode() {}
    virtual ~ContentNode() {}
    // evalute a node using input context and put result in res
    virtual void eval(Context &ctx, std::string &res) = 0 ;

    DocumentNode *root() {
        ContentNode *node = this ;
        while ( node->parent_ ) {
            node = node->parent_ ;
        }

        return reinterpret_cast<DocumentNode *>(node) ;
    }

    void setLineAndColumn(int line, int col) {
        line_ = line ;
        column_ = col ;
    }

    void setTrimLeft(bool s) { trim_left_ = s ; }
    void setTrimRight(bool s) { trim_right_ = s ; }

    virtual void throwException(const std::string &msg) ;

    ContentNode *parent_ = nullptr ;
    bool trim_left_ = false, trim_right_ = false ;
    int line_, column_ ;
};

typedef std::shared_ptr<ContentNode> ContentNodePtr ;

class ContainerNode: public ContentNode {
public:

    void addChild(ContentNodePtr child) {
        children_.push_back(child) ;
        child->parent_ = this ;
    }

    void getAllBlocks(std::vector<NamedBlockNode *> &blocks) ;

    NamedBlockNode *findBlock(const std::string &name) ;

    void throwException(const std::string &msg) override ;

    virtual std::string tagName() const { return {} ; }
    virtual bool shouldClose() const { return true ; }
    std::vector<ContentNodePtr> children_ ;
};

typedef std::shared_ptr<ContainerNode> ContainerNodePtr ;

class LambdaNode: public Node {
public:

    LambdaNode(const identifier_list_t &args, NodePtr body): args_(args), body_(body) {}

    Variant eval(Context &ctx) ;
private:
    identifier_list_t args_ ;
    NodePtr body_ ;
};

class TernaryOperatorNode: public Node {
public:

    TernaryOperatorNode(NodePtr condition, NodePtr true_expr, NodePtr false_expr):
        condition_(condition), true_expr_(true_expr), false_expr_(false_expr) {}

    Variant eval(Context &ctx) ;
private:
    NodePtr condition_, true_expr_, false_expr_ ;
};

typedef std::shared_ptr<LambdaNode> LambdaNodePtr ;

class ForLoopBlockNode: public ContainerNode {
public:

    ForLoopBlockNode(identifier_list_t &&ids, NodePtr target, NodePtr cond = nullptr):
        ids_(ids), target_(target), condition_(cond) {}

    void eval(Context &ctx, std::string &res) override ;

    std::string tagName() const override { return "for" ; }

    void startElseBlock() {
        else_child_start_ = children_.size() ;
    }

    int else_child_start_ = -1 ;

    identifier_list_t ids_ ;
    NodePtr target_, condition_ ;
};


class NamedBlockNode: public ContainerNode {
public:

    NamedBlockNode(const std::string &name): name_(name) {}

    void eval(Context &ctx, std::string &res) override ;

    std::string tagName() const override { return "block" ; }

    std::string name_ ;
};

typedef std::shared_ptr<NamedBlockNode> NamedBlockNodePtr ;

class RefBlockNode: public ContentNode {
public:

    RefBlockNode(const std::string &name): name_(name) {}

    void eval(Context &ctx, std::string &res) override ;

    bool shouldClose() const { return false ; }

    std::string name_ ;
};

class ExtensionBlockNode: public ContainerNode {
public:

    ExtensionBlockNode(NodePtr src): parent_resource_(src) {}

    void eval(Context &ctx, std::string &res) override ;

    std::string tagName() const override { return "extends" ; }
    bool shouldClose() const override { return false ; }

    NodePtr parent_resource_ ;
};

class IncludeBlockNode: public ContentNode {
public:

    IncludeBlockNode(NodePtr source, bool ignore_missing, NodePtr with_expr, bool only):
        source_(source), ignore_missing_(ignore_missing), with_(with_expr), only_flag_(only) {}

    void eval(Context &ctx, std::string &res) override ;

    NodePtr source_, with_ ;
    bool ignore_missing_, only_flag_ ;
};


class EmbedBlockNode: public ContainerNode {
public:

    EmbedBlockNode(NodePtr source, bool ignore_missing, NodePtr with_expr, bool only):
        source_(source), ignore_missing_(ignore_missing), with_(with_expr), only_flag_(only) {}

    void eval(Context &ctx, std::string &res) override ;
    std::string tagName() const override { return "embed" ; }

    NodePtr source_, with_ ;
    bool ignore_missing_, only_flag_ ;
};

class WithBlockNode: public ContainerNode {
public:

    WithBlockNode(NodePtr with_expr, bool only):
        with_(with_expr), only_flag_(only) {}

    void eval(Context &ctx, std::string &res) override ;

    std::string tagName() const override { return "with" ; }

    NodePtr with_ ;
    bool only_flag_ ;
};

class AutoEscapeBlockNode: public ContainerNode {
public:

    AutoEscapeBlockNode(const std::string &mode): mode_(mode) {}

    void eval(Context &ctx, std::string &res) override ;

    std::string tagName() const override { return "autoescape" ; }

    std::string mode_ ;
};

class VerbatimBlockNode: public ContainerNode {
public:

    VerbatimBlockNode(const std::string &content): content_(content) {}

    void eval(Context &ctx, std::string &res) override { res.append(content_) ; };

    std::string tagName() const override { return "autoescape" ; }

    std::string content_ ;
};

class IfBlockNode: public ContainerNode {
public:

    IfBlockNode(NodePtr target) { addBlock(target) ; }

    void eval(Context &ctx, std::string &res) override ;

    std::string tagName() const override { return "if" ; }

    void addBlock(NodePtr ptr) {
        if ( !blocks_.empty() ) {
            blocks_.back().cstop_ = children_.size() ;
        }
        int cstart = blocks_.empty() ? 0 : blocks_.back().cstop_ ;
        blocks_.push_back({cstart, -1, ptr}) ;
    }

    struct Block {
        int cstart_, cstop_ ;
        NodePtr condition_ ;
    } ;

    std::vector<Block> blocks_ ;

};

class AssignmentBlockNode: public ContainerNode {
public:

    AssignmentBlockNode(const identifier_list_t &names, const std::vector<NodePtr> &values): 
    names_(names), values_(values) { }

    void eval(Context &ctx, std::string &res) override ;

    std::string tagName() const override { return "set" ; }
    bool shouldClose() const override { return false ; }

    std::vector<NodePtr> values_ ;
    identifier_list_t names_ ;
};

class ApplyBlockNode: public ContainerNode {
public:

    ApplyBlockNode(const std::vector<FilterNodePtr> &filters): filters_(filters) {} 
   
    void eval(Context &ctx, std::string &res) override ;

    std::string tagName() const override { return "apply" ; }
    bool shouldClose() const override { return true ; }

    std::vector<FilterNodePtr> filters_ ;
};

class FilterBlockNode: public ContainerNode {
public:

    FilterBlockNode(const std::string &name, arg_list_t &&args = {}): name_(name), args_(args) { }

    void eval(Context &ctx, std::string &res) override ;

    std::string tagName() const override { return "filter" ; }

    std::string name_ ;
    arg_list_t args_ ;
};

class MacroBlockNode: public ContainerNode {
public:

    MacroBlockNode(const std::string &name, key_val_list_t &&args): name_(name), args_(args) { }

    void eval(Context &ctx, std::string &res) override ;

    Variant call(Context &ctx, const Variant &args) ;

    void mapArguments(const Variant &args, Context &ctx) ;

    std::string tagName() const override { return "macro" ; }

    std::string name_ ;
    key_val_list_t args_ ;
};

class ImportBlockNode: public ContainerNode {
public:

    ImportBlockNode(NodePtr source, const std::string &ns): source_(source), ns_(ns) { }
    ImportBlockNode(NodePtr source, const key_alias_list_t &&mapping): source_(source),
        mapping_(mapping) { }

    void eval(Context &ctx, std::string &res) override ;

    std::string tagName() const override { return "import" ; }
    bool shouldClose() const override { return false ; }

    bool mapMacro(MacroBlockNode &n, std::string &name) const ;


    std::string ns_ ;
    NodePtr source_ ;
    key_alias_list_t mapping_ ;
};

class RawTextNode: public ContentNode {
public:
    RawTextNode(const std::string &text): text_(text) {}

    void eval(Context &, std::string &res) override {
        res.append(text_) ;
    }

    std::string text_ ;
};

class SubstitutionBlockNode: public ContentNode {
public:
    using Ptr = std::shared_ptr<SubstitutionBlockNode> ;

    SubstitutionBlockNode(NodePtr expr):
        expr_(expr) {}

    void eval(Context &ctx, std::string &res) override;

   

    NodePtr expr_ ;

};


class DocumentNode: public ContainerNode, std::enable_shared_from_this<DocumentNode> {
public:

    DocumentNode() = default ;
    DocumentNode(const std::string &resource): resource_(resource) {} 

    void eval(Context &ctx, std::string &res) override ;

     ExtensionBlockNode* findExtensionNode() const {
        for (const auto& node : children_) {
            if (auto extends_node = dynamic_cast<ExtensionBlockNode *>(node.get())) {
                return extends_node;
            }
        }
        return nullptr; // This template doesn't inherit from anything
    }

    NamedBlockNode *findBlock(const std::string &name) ;

    void populateBlocks() ;

    bool isChild() const { return parent_ != nullptr ; }
    void setParentTemplate(DocumentNodePtr parent) {
        parent_ = parent ;
    }
   
    std::map<std::string, ContentNodePtr> macro_blocks_ ;
    std::string resource_ ;
    DocumentNodePtr parent_ ;
    std::vector<DocumentNode *> child_docs_ ;
    std::map<std::string, NamedBlockNode *> blocks_ ;
};

typedef std::shared_ptr<DocumentNode> DocumentNodePtr ;


} // namespace detail
} // namespace twig
#endif
