# Twig template engine in c++

### Supported tags:
apply, autoescape, block, embed, extends, for, from, if, import, include, macro, set, verbatim, with
(use {% ref block_name %} instead of block function for repeating a block)

### Supported functions:
cycle, date, include, max, min, parent, random, range

### Supported filters:
join, lower, upper, default, e, escape, defined, length, first, last, raw, safe, batch, merge, date,
abs, capitalize, filter, trim, keys, format, json_encode, find, map, reduce, round, slice

### Supported tests:
defined, divisible by, empty, even, iterable, mapping, null, odd, sequence

All data is represented by the Variant type that is a polymorphic JSON type object that can store primitive types, arrays, dictionaries and functions.

Create a loader:

>  std::shared_ptr<TemplateLoader> loader(new DictTemplateLoader({
>        {"base1.html.twig", R"({% block head %}<head><title>{% block title %}{% endblock %}</title></head>{% endblock %}<body>{% block content %}{% endblock %}</body><footer>{% block footer %}footer{% endblock %}</footer>)"},
>    })) ;

Create a renderer:

> TemplateRenderer rdr(loader) ;

Define the context data:

>  Variant::Object ctx{
>       {"users", Variant::Array{
>            Variant::Object{{"name", "Alice"}},
>            Variant::Object{{"name", "Bob"}},
>            Variant::Object{{"name", "Charlie"}}
>        }}
>    } ;

Render a template 

> string res = rdr.render('base1.html.twig', ctx);

You may declare custom functions, filters, test by means of the global FunctionFactory object.

> FunctionFactory::instance().registerFunction("lipsum",
> [](const Variant &args) -> Variant {
>       Variant::Array vargs ;
>       unpack_args(args, {"data"}, vargs) ;
>
>       if ( !vargs.empty() ) {
>           auto n = vargs[0].toInteger() ;
>            string res ;
>            for ( int64_t i = 0 ; i < n ; ++i ) res += "Lorem ipsum dolor sit amet, consectetur > adipiscing elit. " ;
>            return res ;
>        }
>        else return string() ;
>    });

Function arguments are packed into a Variant::Object. args["args"] holds and array of positional variables, and args["kw"] a dictionary of named arguments. Use unpack_args to convert to an array of posibly undefined values.
