#include "ast.hpp"

#include <twig/functions.hpp>
#include <twig/exceptions.hpp>
#include <twig/renderer.hpp>

#include <cmath>

using namespace std ;

namespace twig {

extern Variant escape(const Variant &src, const string &escape_mode) ;

namespace detail {




Variant BooleanOperator::eval(Context &ctx) {
    switch ( op_ ) {
    case And:
        return ( lhs_->eval(ctx).toBoolean() && rhs_->eval(ctx).toBoolean() ) ;
    case Or:
        return ( lhs_->eval(ctx).toBoolean() || rhs_->eval(ctx).toBoolean() ) ;
    }
}

Variant BooleanNegationOperator::eval(Context &ctx) {
    return !( node_->eval(ctx).toBoolean() ) ;
}

static bool compare_numbers(int64_t lhs, int64_t rhs, ComparisonPredicate::Type op) {
    switch ( op ) {
    case ComparisonPredicate::Equal:
        return lhs == rhs ;
    case ComparisonPredicate::NotEqual:
        return lhs != rhs ;
    case ComparisonPredicate::Less:
        return lhs < rhs ;
    case ComparisonPredicate::LessOrEqual:
        return lhs <= rhs ;
    case ComparisonPredicate::GreaterOrEqual:
        return lhs >= rhs ;
    case ComparisonPredicate::Greater:
        return lhs > rhs ;
    }
}

static bool compare_numbers(const Variant &lhs, const Variant &rhs, ComparisonPredicate::Type op) {

    if ( ( lhs.type() == Variant::Type::Integer || lhs.type() == Variant::Type::Boolean) && rhs.type() == Variant::Type::Float ) {
        return compare_numbers(lhs.toFloat(), rhs.toFloat(), op) ;
    } else if ( lhs.type() == Variant::Type::Float && ( rhs.type() == Variant::Type::Integer || rhs.type() == Variant::Type::Boolean ) ) {
        return compare_numbers(lhs.toFloat(), rhs.toFloat(), op) ;
    } else
        return compare_numbers(lhs.toInteger(), rhs.toInteger(), op) ;
}

static bool variant_compare(const Variant &lhs, const Variant &rhs, ComparisonPredicate::Type op) {

    if ( op == ComparisonPredicate::In || op == ComparisonPredicate::NotIn ) {
        if ( rhs.isArray() ) {
            for ( const auto &item: rhs ) {
                if ( variant_compare(lhs, item, ComparisonPredicate::Equal) )
                    return ( op == ComparisonPredicate::In ) ? true : false ;
            }
            return ( op == ComparisonPredicate::In ) ? false : true ;
        } else if ( rhs.isString() && lhs.isString() ) {
            string ls = lhs.toString(), rs = rhs.toString() ;
            return ( rs.find(ls) != string::npos ) ? ( op == ComparisonPredicate::In ) ? true : false : ( op == ComparisonPredicate::In ) ? false : true ;
        } else
            throw TemplateRuntimeException("Right operand of 'in' operator must be an array or a string") ;
    } else if ( op == ComparisonPredicate::StartsWith || op == ComparisonPredicate::EndsWith ) {
        string ls = lhs.toString(), rs = rhs.toString() ;
        if ( op == ComparisonPredicate::StartsWith )
            return ( ls.rfind(rs, 0) == 0 ) ;
        else
            return ( ls.size() >= rs.size() && ls.compare(ls.size() - rs.size(), rs.size(), rs) == 0 ) ;
    } else if ( op == ComparisonPredicate::HasSome ) {
        if ( rhs.type() != Variant::Type::Function ) 
            throw TemplateRuntimeException("need an arrow function for 'has some' test") ;
        for( auto a: lhs ) {
            Variant::Array args{a} ;
            bool res = rhs.invoke(args).toBoolean();
            if ( res ) return true ;
        }
        return false ;
    } else if ( op == ComparisonPredicate::HasEvery ) {
        if ( rhs.type() != Variant::Type::Function ) 
            throw TemplateRuntimeException("need an arrow function for 'has every' test") ;
        for( auto a: lhs ) {
            Variant::Array args{a} ;
            bool res = rhs.invoke(args).toBoolean();
            if ( !res ) return false ;
        }
        return true ;
    }

    if ( lhs.isString() && rhs.isString() ) {
        string ls = lhs.toString(), rs = rhs.toString() ;
        int res = ls.compare(rs) ;
        switch ( op ) {
        case ComparisonPredicate::Equal:
        case ComparisonPredicate::Identical:
            return res == 0 ;
        case ComparisonPredicate::NotEqual:
        case ComparisonPredicate::NotIdentical:
            return res != 0 ;
        case ComparisonPredicate::Less:
            return res < 0 ;
        case ComparisonPredicate::LessOrEqual:
            return res <= 0 ;
        case ComparisonPredicate::GreaterOrEqual:
            return res >= 0 ;
        case ComparisonPredicate::Greater:
            return res > 0 ;
        }
    }

    if ( lhs.isNumber() && rhs.isNumber() ) {
        return compare_numbers(lhs, rhs, op) ;
    } else if ( lhs.isNumber() && rhs.isString() ) {
        if ( op == ComparisonPredicate::Identical ) return false ;
        else if ( op == ComparisonPredicate::NotIdentical ) return true ;
        else
        return compare_numbers(lhs, rhs.toNumber(), op ) ;
    } else if ( lhs.isString() && rhs.isNumber() ) {
         if ( op == ComparisonPredicate::Identical ) return false ;
        else if ( op == ComparisonPredicate::NotIdentical ) return true ;
        else
        return compare_numbers(lhs.toNumber(), rhs, op ) ;
    } else return compare_numbers((int64_t)&lhs, (int64_t)&rhs, op) ;

}

Variant ComparisonPredicate::eval(Context &ctx)
{
    Variant lhs = lhs_->eval(ctx) ;
    Variant rhs = rhs_->eval(ctx) ;

    if ( lhs.isNull() || rhs.isNull() ) return false ;

    return variant_compare(lhs, rhs, op_);
}

Variant IdentifierNode::eval(Context &ctx)
{
  /*  if ( FunctionFactory::instance().hasFunction(name_) ) {
        return Variant(Variant::Function([=](const Variant &args) {
            return FunctionFactory::instance().invoke(name_, args) ;
        })) ;
    }
    else*/ {
        return ctx.get(name_) ;
        /*
        auto it = ctx.data().find(name_) ;
        if ( it == ctx.data().end() ) return Variant::undefined() ;
        else return it->second ;
        */
    }
}

static int64_t arithmetic(int64_t lhs, int64_t rhs, const std::string &op) {
    if  ( op == "+"  )
        return lhs + rhs ;
    else if ( op == "-" )
        return lhs - rhs ;
    else if ( op == "*" )
        return lhs * rhs ;
    else if ( op == "/" )
        return ( rhs ) ? (lhs / rhs) : 0 ;
    else if ( op == "%" )
        return ( rhs ) ? (lhs % rhs) : 0 ;
    else if ( op == "**" )
            return (int64_t)pow(lhs, rhs) ;
    else
        throw TemplateRuntimeException("Unknown operator: " + op) ;
}

static double arithmetic(double lhs, double rhs, const std::string &op) {
   if ( op == "+" )
        return lhs + rhs ;
    else if ( op == "-" )
        return lhs - rhs ;
    else if ( op == "*" )
        return lhs * rhs ;
    else if ( op == "/" )
        return ( rhs != 0.0 ) ? (lhs / rhs) : 0.0 ;
    else if ( op == "**" )
        return pow(lhs, rhs) ;
    else if ( op == "//" )
        return ( rhs != 0.0 ) ? (int64_t)floor(lhs / rhs) : 0 ;
    else if ( op == "%" )
        return ( rhs != 0.0 ) ? (int64_t)fmod(lhs, rhs) : 0 ;
    else
        throw TemplateRuntimeException("Unknown operator: " + op) ;
}


static Variant arithmetic(const Variant &lhs, const Variant &rhs, const std::string &op) {
    if ( ( lhs.type() == Variant::Type::Integer || lhs.type() == Variant::Type::Boolean) && rhs.type() == Variant::Type::Float ) {
        return arithmetic(lhs.toFloat(), rhs.toFloat(), op) ;
    } else if ( lhs.type() == Variant::Type::Float && ( rhs.type() == Variant::Type::Integer || rhs.type() == Variant::Type::Boolean ) ) {
        return arithmetic(lhs.toFloat(), rhs.toFloat(), op) ;
    } else
        return arithmetic(lhs.toInteger(), rhs.toInteger(), op) ;
}

Variant BinaryOperator::eval(Context &ctx)
{
    Variant op1 = lhs_->eval(ctx) ;
    Variant op2 = rhs_->eval(ctx) ;

  /*  if ( op1.isUndefined() || op1.isNull() )
        throw TemplateRuntimeException(str(boost::format("Undefined or null value on the left of %c operator") % (char)op_)) ;

    if ( op2.isUndefined() || op2.isNull() )
        throw TemplateRuntimeException(str(boost::format("Undefined or null value on the right of %c operator") % (char)op_)) ;
*/
    if ( op_ == "??" ) {
        if (op1.isUndefined() || op1.isNull()) return op2 ;
        else return op1 ;
    } else if ( op_ == "+" || op_ == "-" || op_ == "*" || op_ == "/" || op_ == "**" || op_ == "//"  || op_ == "%" )
        return arithmetic(op1.toFloat(), op2.toFloat(), op_) ;
    else if ( op_ == "~" )
        return op1.toString() + op2.toString() ;
    else
        throw TemplateRuntimeException("Unknown binary operator: " + op_) ;
}



Variant UnaryOperator::eval(Context &ctx)
{
    Variant val = rhs_->eval(ctx) ;

    if ( op_ == '-' ) {
        return arithmetic(0, val, "-") ;
    } else if ( op_ == '!' ) {
        return !val.toBoolean() ;
    }
    else return val ;
}

Variant SpreadOperator::eval(Context &ctx) {
    Variant v = rhs_->eval(ctx) ;
    return v ;
}

Variant ArrayNode::eval(Context &ctx)
{
    Variant::Array a ;

    for ( NodePtr e: elements_ ) {
        if ( SpreadOperator *so = dynamic_cast<SpreadOperator *>(e.get()) ) {
            Variant s = so->eval(ctx) ;
           
            if ( s.isArray() ) {
                for ( auto &se: s ) {
                    a.emplace_back(se) ;
                }
            } else if ( !s.isUndefined() && !s.isNull() ) {
                throw TemplateRuntimeException("spread operator needs array variable");
            }
        } else {
            a.push_back(e->eval(ctx)) ;
        }
    }

    return a ;
}

Variant DictionaryNode::eval(Context &ctx)
{
    Variant::Object o ;

    for ( auto &kv: elements_ ) {
        o.insert({kv.first, kv.second->eval(ctx)}) ;
    }

    return o ;
}

Variant SubscriptIndexingNode::eval(Context &ctx)
{
    Variant index = index_->eval(ctx) ;

    if ( index.isUndefined() || index.isNull() )
        throw TemplateRuntimeException("Undefined or null index in subscript indexing") ;
    
    Variant a = array_->eval(ctx) ;

    if ( a.isUndefined() || a.isNull() || ( !a.isArray() && !a.isObject() ) ) {
        return Variant::undefined() ;
    }

    if ( a.isArray() )
        return a.at(index.toInteger());
    else
        return a.at(index.toString()) ;
}

Variant AttributeIndexingNode::eval(Context &ctx) {
    Variant o = dict_->eval(ctx) ;
    if ( !o.isObject()) {
        if ( except_on_null_ )
            throw TemplateRuntimeException("Subscript operand applied to non-object") ;
        else
            return Variant::undefined() ;
    } 

    return o.at(key_) ;
}


static void evalArgs(const arg_list_t &input_args, Variant &packed_args, Context &ctx)
{
    Variant::Array positional ;
    Variant::Object kw ;

    for ( auto &&e: input_args ) {
        if ( e.name_.empty() ) {
            if ( SpreadOperator *so = dynamic_cast<SpreadOperator *>(e.value_.get()) ) {
                Variant s = so->eval(ctx) ;

                if ( s.isArray() ) {
                    for( auto &se: s ) {
                        positional.push_back(se) ;
                    }
                }
                else if ( !s.isUndefined() && !s.isNull() ) {
                    throw TemplateRuntimeException("spread operator needs array variable");
                }  
            }
            else {
                auto v = e.value_->eval(ctx) ;
                positional.push_back(v) ;
            }
        } else {
            kw.emplace(e.name_, e.value_->eval(ctx));
        }
    }

    Variant::Object packed ;

    packed.emplace("args", positional);
    packed.emplace("kw", kw) ;

    packed_args = Variant(packed) ;
}

static Variant evalFilter(const string &name, const arg_list_t &args, const Variant &target, Context &ctx) {
    Variant evargs ;
    evalArgs(args, evargs, ctx) ;
    return FunctionFactory::instance().invokeFilter(name, target, evargs, ctx) ;
}

static Variant evalTest(const string &name, const arg_list_t &args, const Variant &target, Context &ctx) {

    Variant evargs ;
    evalArgs(args, evargs, ctx) ;
    return FunctionFactory::instance().invokeTest(name, target, evargs, ctx) ;
}

static Variant applyFilter(const Variant &target, const std::vector<FilterNodePtr> &filters, Context &ctx) {
    Variant res = target ;
    for( FilterNodePtr filter: filters ) {
        res = evalFilter(filter->name_, filter->args_, res, ctx ) ;
    }
    return res ;
}

Variant InvokeFilterNode::eval(Context &ctx)
{
    Variant target = target_->eval(ctx) ;
    return applyFilter(target, filters_, ctx) ;
}

Variant TestExpressionNode::eval(Context &ctx)
{
    Variant target = lhs_->eval(ctx) ;
    arg_list_t args ;
    if ( args_ ) {
        args.emplace_back(std::string(), args_) ;
    } 
    return evalTest(name_, args, target, ctx) ;
}

Variant RangeOperatorNode::eval(Context &ctx)
{
    Variant lhs = lhs_->eval(ctx) ;
    Variant rhs = rhs_->eval(ctx) ;

    if ( lhs.isString() && rhs.isString() ) {
        string start= lhs.toString() ;
        string end = rhs.toString() ;

        if ( start.size() >= 1 && end.size() >= 1 ) {
            int s = (int)start[0], e = (int)end[0] ;
            Variant::Array res ;
            for ( int i = s ; i <= e ; i++ ) {
                res.push_back(string(1, (char)i)) ;
            }
            return res ;
        }
    } else if ( lhs.isNumber() || rhs.isNumber() ) {
        int start = lhs.toInteger(), end = rhs.toInteger() ;
        Variant::Array res ;
        for ( int i = start ; i <= end ; i++ ) {
            res.push_back(i) ;
        }
        return res ;
    } else 
            
    throw TemplateRuntimeException("Invalid operands for range operator") ;
    
}

Variant InvokeTestNode::eval(Context &ctx) {
    Variant target = target_->eval(ctx) ;

    Variant v = evalFilter(name_, args_, target, ctx) ;
/*
    if ( v.type() != Variant::Type::Boolean )
        throw TemplateRuntimeException("test function returning non-boolean value") ;
*/
    bool res = v.toBoolean() ;

    return ( positive_ ) ? res : !res ;
}

void ForLoopBlockNode::eval(Context &ctx, string &res) const
{
    uint counter = 0 ;
    Variant target = target_->eval(ctx) ;
    int asize = target.length() ;

    if ( asize > 0 ) {

        int child_count = ( else_child_start_ < 0 ) ? children_.size() : else_child_start_ ;
        for ( auto it = target.begin() ; it != target.end() ; ++it, counter++  ) {

            Context tctx(ctx) ;

            Variant::Object loop{ {"index0", counter},
                                  {"index", counter+1},
                                  {"revindex0", asize - counter - 1},
                                  {"revindex", asize - counter},
                                  {"first", counter == 0},
                                  {"last", counter == asize-1},
                                  {"length", asize}
                                } ;
            tctx.data()["loop"] = loop ;

            uint i = 0 ;
            for( auto &&c: children_ ) {
                if ( ++i > child_count ) break ;
                if ( ids_.size() == 1 ) {
                    Context cctx(tctx) ;
                    cctx.data()[ids_[0]] = *it ;
                    if ( condition_ && !condition_->eval(cctx).toBoolean() ) continue ;

                    c->eval(cctx, res) ;
                } else if ( ids_.size() == 2 ) {
                    Context cctx(tctx) ;
                    cctx.data()[ids_[0]] =  it.key() ;
                    cctx.data()[ids_[1]] =  it.value() ;
                    if ( condition_ && !condition_->eval(cctx).toBoolean() ) continue ;
                    c->eval(cctx, res) ;
                }
            }
        }
    } else if ( else_child_start_ >= 0 ) {

        for( uint count = else_child_start_ ; count < children_.size() ; count ++ ) {
            children_[count]->eval(ctx, res) ;
        }
    }
}

void IfBlockNode::eval(Context &ctx, string &res) const
{
    for( const Block &b: blocks_ ) {
        int c_start = b.cstart_ ;
        int c_stop = ( b.cstop_ == -1 ) ? children_.size() : b.cstop_ ;
        if (  !b.condition_ || b.condition_->eval(ctx).toBoolean() ) {
            for( int c = c_start ; c < c_stop ; c++ ) {
                children_[c]->eval(ctx, res) ;
            }
            break ;
        }
    }
}

/*
Variant TernaryExpressionNode::eval(Context &ctx)
{
    if ( condition_->eval(ctx).toBoolean() )
        return positive_->eval(ctx) ;
    else return negative_ ? negative_->eval(ctx) : Variant::null() ;
}
*/
void AssignmentBlockNode::eval(Context &ctx, string &res) const {
    if ( names_.size() == 1 && values_.empty() ) { // block assignment
        string subres ;
        for( auto &&c: children_ ) {
            c->eval(ctx, subres) ;  
        }
        ctx.data_.insert_or_assign(names_[0], subres) ;
    } else {
        Context cctx(ctx) ;
        for( size_t i = 0 ; i< names_.size() ; i++ ) {
            const auto &key = names_[i] ;
            Variant val = values_[i]->eval(ctx) ;
            cctx.data_.insert_or_assign(key, val) ;
        }
        for( auto &&c: children_ ) {
            c->eval(cctx, res) ;  
        }
    }
}


void ApplyBlockNode::eval(Context &ctx, string &res) const {
// render block content
    string block_res ;
    for( auto &&c: children_ )
        c->eval(ctx, block_res) ;

    Variant v = applyFilter(block_res, filters_, ctx) ;
    res.append(v.toString());
}

void FilterBlockNode::eval(Context &ctx, string &res) const
{
    string block_res ;
    for( auto &&c: children_ )
        c->eval(ctx, block_res) ;

    string result = evalFilter(name_, args_, block_res, ctx).toString() ;

    res.append(result)  ;
}


Variant InvokeFunctionNode::eval(Context &ctx)
{
    Variant args ;
    evalArgs(args_, args, ctx) ;


    if (IdentifierNode *node = dynamic_cast<IdentifierNode *>(callable_.get()) ) {
        string fn_name = node->name() ;

        FunctionFactory &ff = FunctionFactory::instance() ;
        if ( ff.hasFunction(fn_name) ) {
            return ff.invokeFunction(fn_name, args, ctx) ; 
        }
        
    }
    Variant callable = callable_->eval(ctx) ;

    if ( callable.type() == Variant::Type::Function )
        return callable.invoke(args) ;
    else
        throw TemplateRuntimeException("function invocation of non-callable variable") ;
/*
    Variant f = ctx.get(callable_) ;
    
    if ( f.isUndefined() || f.isNull() ) {
        FunctionFactory &ff = FunctionFactory::instance() ;
        if ( ff.hasFunction(callable_) ) {
            return ff.invokeFunction(callable_, args, ctx) ; 
        }
            
    } else if ( f.type() == Variant::Type::Function )
        return f.invoke(args) ;
    else
        throw TemplateRuntimeException("function invocation of non-callable variable") ;*/
}

const NamedBlockNode *ContainerNode::findBlock(const std::string &name) const {
    const NamedBlockNode *n ;
    if ( ( n = dynamic_cast<const NamedBlockNode *>(this) ) != nullptr )
        return n ;
    for( auto c: children_ ) {
        ContainerNode *cn = dynamic_cast<ContainerNode *>(c.get()) ;
        if ( cn ) {
            n = cn->findBlock(name) ;
            if ( n ) return n ;
        }
    }

    return nullptr ;
}

void NamedBlockNode::eval(Context &ctx, string &res) const {
    auto it = ctx.blocks_.find(name_) ;
    if ( it != ctx.blocks_.end() ) {
        Context cctx(ctx) ;
        cctx.data()["parent"] = Variant([&](const Variant &) -> Variant {
            string pp ;
            for( auto &&c: children_ ) {
                c->eval(cctx, pp) ;
            }
            return Variant(pp) ;
        }) ;

        for( auto &&c: it->second->children_ ) {
            c->eval(cctx, res) ;
        }
    }
    else {
        for( auto &&c: children_ ) {
            c->eval(ctx, res) ;
        }
    }

}

void RefBlockNode::eval(Context &ctx, string &res) const {

    const DocumentNode *r = root() ;

    const NamedBlockNode *n = r->findBlock(name_) ;

    if ( n ) n->eval(ctx, res) ;
}


void ExtensionBlockNode::eval(Context &ctx, string &res) const
{
    string resource = parent_resource_->eval(ctx).toString() ;

    TemplateRenderer &rdr = ctx.rdr_ ;

    auto parent = rdr.compile(resource) ;

    Context pctx(ctx) ;

    // fill context with block definitions

    for( auto &&c: children_ ) {
        NamedBlockNodePtr block = std::dynamic_pointer_cast<NamedBlockNode>(c) ;
        if ( block )
            pctx.addBlock(block) ;
    }

    parent->eval(pctx, res) ;
}



void MacroBlockNode::eval(Context &ctx, string &str) const
{

}

void MacroBlockNode::mapArguments(const Variant &args, Context &ctx)
{
    Variant pos_args = args["args"];
    Variant kw_args = args["kw"];

    uint n_positional_args = pos_args.length() ;

    for ( uint pos = 0 ; pos < args_.size() ; pos ++ )  {
        const string &arg_name = args_[pos].first ;
        NodePtr dval = args_[pos].second ;
        Variant kwv = kw_args.at(arg_name);
        Variant v ;
        if ( pos < n_positional_args ) 
             v = pos_args.at(pos) ;
        else if ( !kwv.isUndefined() ) {
            v = kwv ;
        }
        else if ( dval ) {
            v = dval->eval(ctx);
        } else {
            throw TemplateRuntimeException("missing require parameter:" + arg_name);
        }
        ctx.data()[arg_name] = std::move(v) ;
    }
    /*
    uint n_args = args_.size() ;

    const Variant &pos_args = args["args"] ;

    for ( uint pos = 0 ; pos < n_args && pos < pos_args.length() ; pos ++ )  {
        const string &arg_name = args_[pos].first ;

        Variant v = pos_args.at(pos) ;
        ctx.data()[arg_name] = std::move(v) ;
    }

    const Variant &kw_args = args["kw"] ;

    for ( auto it = kw_args.begin() ; it != kw_args.end() ; ++it ) {
        string key = it.key() ;
        const Variant &val = it.value() ;

        for( uint k=0 ; k<n_args ; k++ ) {
            const string &arg_name = args_[k].first ;

            if ( key == arg_name )
                ctx.data()[arg_name] = val ;
        }
    }

    // store arguments in context
    ctx.data()["_args_"] = pos_args ;
    ctx.data()["_kw_"] = kw_args ;*/
}

Variant MacroBlockNode::call(Context &ctx, const Variant &args) {

    Context mctx(ctx.rdr_, {}) ;

    mapArguments(args, mctx) ;

    string out ;

    for( auto &&c: children_ ) {
        c->eval(mctx, out) ;
    }

    return Variant(out, true) ; // macros should return safe strings
}

void ImportBlockNode::eval(Context &ctx, string &res) const
{
    DocumentNodePtr doc ;

    // if _self was provided then source should be null

    if ( source_ ) {
        string resource = source_->eval(ctx).toString() ;

        TemplateRenderer &rdr = ctx.rdr_ ;

        doc = rdr.compile(resource) ;
    }

    Context pctx(ctx) ;

    // create closures corresponding to each macro and add them to the namespace variable
    // of the current context

    Variant::Object closures ;

    for( auto &&m: (doc) ? doc->macro_blocks_ : root()->macro_blocks_ ) {

        std::shared_ptr<MacroBlockNode> p_macro = std::dynamic_pointer_cast<MacroBlockNode>(m.second) ;
        if ( p_macro ) {

            string mapped_name ;
            if ( !mapMacro(*p_macro, mapped_name) ) continue ; // if not imported

            auto macro_fn = [&ctx, p_macro](const Variant &args) -> Variant {
                return p_macro->call(ctx, args) ;
            };

            closures.insert({mapped_name, Variant::Function(macro_fn)}) ;
        }
    }

    if ( !ns_.empty() ) pctx.data()[ns_] = Variant(closures) ;
    else {
        for( auto &e: closures ) {
            pctx.data()[e.first] = e.second ;
        }
    }

    for( auto &&c: children_ ) {
        c->eval(pctx, res) ;
    }

}

bool ImportBlockNode::mapMacro(MacroBlockNode &n, string &name) const {

    // only namespace given import all macros using their name
    if ( mapping_.empty() ) {
        name = n.name_ ;
        return true ;
    }

    // check mapping list if the macro has been imported
    auto it = std::find_if(mapping_.begin(), mapping_.end(), [&n](const key_alias_t &e) { return e.first == n.name_ ; } ) ;
    if ( it == mapping_.end() ) return false ; // not found ignore
    name = (*it).second.empty() ? n.name_ : (*it).second ;
    return true ;
}

void IncludeBlockNode::eval(Context &ctx, string &res) const
{
    // get the list of templates that we are going to check

    vector<string> templates ;

    Variant source = source_->eval(ctx) ;

    if ( source.isArray() ) {
        for( auto &&e: source )
            templates.emplace_back(e.toString()) ;
    }
    else
        templates.emplace_back(source.toString()) ;

    // try to load one of the templates

    DocumentNodePtr doc ;

    for( auto &&tmpl: templates ) {
        try {
            doc = ctx.rdr_.compile(tmpl) ;
            break ;
        }
        catch ( TemplateLoadException & ) {

        }
        catch ( TemplateCompileException &e ) {
            throw e ;
        }

    }

    // check whether not found

    if ( !doc ) {
        if ( !ignore_missing_ ) // non found
            throw TemplateRuntimeException("Failed to load included template: " + templates[0]) ;
        else
            return ;
    }

    // template is compiled so render it with the appropriate context

    Variant::Object ctx_extension ;

    // if there is a with statment we collect the variables

    if ( with_ ) {
        Variant with = with_->eval(ctx) ;
        if ( with.isObject() ) {
            for ( auto it = with.begin() ; it != with.end() ; ++it ) {
                ctx_extension[it.key()] = it.value() ;
            }
        }
    }

    // create new context either inheriting parent one or empty and extend with key/values if any

    if ( only_flag_ ) {
        Context cctx(ctx.rdr_, {}) ;
        cctx.data().insert(ctx_extension.begin(), ctx_extension.end()) ;
        doc->eval(cctx, res) ;
    } else {
        Context cctx(ctx) ;
        for( auto &&e: ctx_extension )
            cctx.data()[e.first] = e.second ;
        doc->eval(cctx, res) ;
    }
}


void WithBlockNode::eval(Context &ctx, string &res) const
{
    Variant::Object ctx_extension ;

    // if there is a with statment we collect the variables

    if ( with_ ) {
        Variant with = with_->eval(ctx) ;
        if ( with.isObject() ) {
            for ( auto it = with.begin() ; it != with.end() ; ++it ) {
                ctx_extension[it.key()] = it.value() ;
            }
        }
    }

    // create new context either inheriting parent one or empty and extend with key/values if any

    if ( only_flag_ ) {
        Context cctx(ctx.rdr_, {}) ;
        cctx.data().insert(ctx_extension.begin(), ctx_extension.end()) ;
        for( auto &&c: children_ )
            c->eval(cctx, res) ;
    } else {
        Context cctx(ctx) ;
        for( auto &&e: ctx_extension )
            cctx.data()[e.first] = e.second ;
        for( auto &&c: children_ )
            c->eval(cctx, res) ;
    }
}

static bool compare_numbers(double lhs, double rhs, ComparisonPredicate::Type op) {
    switch ( op ) {
    case ComparisonPredicate::Equal:
        return lhs == rhs ;
    case ComparisonPredicate::NotEqual:
        return lhs != rhs ;
    case ComparisonPredicate::Less:
        return lhs < rhs ;
    case ComparisonPredicate::LessOrEqual:
        return lhs <= rhs ;
    case ComparisonPredicate::GreaterOrEqual:
        return lhs >= rhs ;
    case ComparisonPredicate::Greater:
        return lhs > rhs ;
    }
}


Variant ContainmentNode::eval(Context &ctx)
{
    Variant lhs = lhs_->eval(ctx) ;
    Variant rhs = rhs_->eval(ctx) ;

    if ( !lhs.isPrimitive() || !rhs.isArray() )
        throw TemplateRuntimeException("wrong type of values on containment operaror") ;

    for( auto &&e: rhs ) {
        if ( variant_compare(lhs, e, ComparisonPredicate::Equal) ) return true ;
    }

    return false ;
}

Variant LambdaNode::eval(Context &ctx) {
    return Variant([this, &ctx](const Variant &args) -> Variant {
        Variant pos_args = args ;
        if ( pos_args.length() != args_.size() )
            throw TemplateRuntimeException("wrong number of arguments passed to lambda") ;
        
        Context cctx(ctx) ;
        for( uint i = 0 ; i < args_.size() ; i++ ) {
            cctx.data()[args_[i]] = pos_args.at(i) ;
        }
        Variant res = body_->eval(cctx);
        return res ;
    });
}

Variant AssignmentNode::eval(Context &ctx) {
    Variant val = rhs_->eval(ctx) ;
    if ( args_.size() == 1 ) {
        ctx.data()[args_.front()] = val ;
        return val ;
    } else if ( type_ == ArrayDestructring && val.isArray()  ) {
        Variant::Array arr ; 
        for( uint i = 0 ; i < args_.size() ; i++ ) {
            if ( args_[i].empty() ) continue ;
            Variant v = val.at(i) ;
            ctx.data()[args_[i]] = v;
            arr.push_back(v) ;
        }
        return arr ;
    } else if ( type_ == DictionaryDestructuring && val.isObject() ) {
        Variant::Object obj ; 
        for( const auto &kv: dict_args_ ) {
            if ( kv.key_.empty() ) continue ;

            Variant res = val.at(kv.key_) ;
    
            if ( !kv.alias_.empty() ) {
                ctx.data()[kv.alias_] = res;
                 obj[kv.key_] = res ;
            } else {
                ctx.data()[kv.key_] = res;
                obj[kv.key_] = res ;
            }
        }
        return obj ;
    }
    throw TemplateRuntimeException("invalid number of arguments for assignment operator") ;
}

Variant TernaryOperatorNode::eval(Context &ctx) {
    if ( condition_->eval(ctx).toBoolean() )
        return true_expr_->eval(ctx) ;
    else return false_expr_ ? false_expr_->eval(ctx) : Variant::null() ;
}

#if 1
MatchesNode::MatchesNode(NodePtr lhs, const string &rx, bool positive): lhs_(lhs), positive_(positive) {

    if ( rx.size() < 2 ) throw TemplateCompileException("empty regex string") ;

    char delimiter = rx[0];

    size_t end_delim_pos = rx.rfind(delimiter);
    if (end_delim_pos == 0 || end_delim_pos == std::string::npos) {
        throw TemplateCompileException("unmatched delimiters in regex") ;
    }

    // 3. Extract the inner pattern and any trailing flags
    std::string pattern = rx.substr(1, end_delim_pos - 1);
    std::string flagsStr = rx.substr(end_delim_pos + 1);

    std::regex_constants::syntax_option_type regex_flags = std::regex_constants::ECMAScript;
    for (char flag : flagsStr) {
        if (flag == 'i') {
            regex_flags |= std::regex_constants::icase; // Case-insensitive
        }
        // Note: PCRE flags like 'm', 's', 'x' lack direct, identical ECMAScript flag mappings.
    }

    rx_.assign(pattern, regex_flags) ;


}
#endif
Variant MatchesNode::eval(Context &ctx)
{
    string val = lhs_->eval(ctx).toString() ;

    bool res = std::regex_search(val, rx_)  ;

    return (bool)res ;
}

void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
        return !std::isspace(ch);
    }));
}

void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

void SubstitutionBlockNode::eval(Context &ctx, string &res) const {

    string content = escape(expr_->eval(ctx), ctx.escape_mode_).toString() ;

    res.append(content) ;
}

void AutoEscapeBlockNode::eval(Context &ctx, string &res) const
{
    Context cctx(ctx) ;
    cctx.escape_mode_ = mode_ ;
    for( auto &&c: children_ )
        c->eval(cctx, res) ;
}

void EmbedBlockNode::eval(Context &ctx, string &res) const
{
    vector<string> templates ;
    Variant source = source_->eval(ctx) ;

    if ( source.isArray() ) {
        for( auto &&e: source )
            templates.emplace_back(e.toString()) ;
    }
    else
        templates.emplace_back(source.toString()) ;

    // try to load one of the templates

    DocumentNodePtr doc ;

    for( auto &&tmpl: templates ) {
        try {
            doc = ctx.rdr_.compile(tmpl) ;
            break ;
        }
        catch ( TemplateLoadException & ) {

        }
        catch ( TemplateCompileException &e ) {
            throw e ;
        }

    }

    // check whether not found

    if ( !doc ) {
        if ( !ignore_missing_ ) // non found
            throw TemplateRuntimeException("Failed to load included template: " + templates[0]) ;
        else
            return ;
    }

    // template is compiled so render it with the appropriate context

    Variant::Object ctx_extension ;

    // if there is a with statment we collect the variables

    if ( with_ ) {
        Variant with = with_->eval(ctx) ;
        if ( with.isObject() ) {
            for ( auto it = with.begin() ; it != with.end() ; ++it ) {
                ctx_extension[it.key()] = it.value() ;
            }
        }
    }

    // create new context either inheriting parent one or empty and extend with key/values if any

    if ( only_flag_ ) {
        Context cctx(ctx.rdr_, {}) ;
        cctx.data().insert(ctx_extension.begin(), ctx_extension.end()) ;

        for( auto &&c: children_ ) {
            NamedBlockNodePtr block = std::dynamic_pointer_cast<NamedBlockNode>(c) ;
            if ( block )
                cctx.addBlock(block) ;
        }

        doc->eval(cctx, res) ;
    } else {
        Context cctx(ctx) ;


        for( auto &&e: ctx_extension )
            cctx.data()[e.first] = e.second ;


        for( auto &&c: children_ ) {
            NamedBlockNodePtr block = std::dynamic_pointer_cast<NamedBlockNode>(c) ;
            if ( block )
                cctx.addBlock(block) ;
        }

        doc->eval(cctx, res) ;
    }

}


} // detail


void Context::addBlock(detail::NamedBlockNodePtr node) {
    blocks_.emplace(node->name_, node) ;
}

}
