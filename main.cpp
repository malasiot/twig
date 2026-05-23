#include <twig/renderer.hpp>
#include <twig/exceptions.hpp>
#include <iostream>

using namespace std ;
using namespace twig ;

const string msg = R"(
hello***    {% if a.x[2] > 3 %}   {{- 'if' -}}  {% else %} else {%- endif -%}  ***
)";

const string loop  =  R"(
{% for user in users %}
    {{loop.index}}. {{ user.name }}
{% else %}
    No users have been found.
{% endfor %}
)";

class StringTemplateLoader: public TemplateLoader {
public:
    StringTemplateLoader(const string &s): str_(s) {}

    std::string load(const std::string &src) {
        return str_ ;
    }

    string str_ ;
};

int main(int argc, char *argv[]) {

    TemplateRenderer rdr(nullptr) ;

   Variant::Object ctx{
        {"users", Variant::Array{
            Variant::Object{{"name", "Alice"}},
            Variant::Object{{"name", "Bob"}},
            Variant::Object{{"name", "Charlie"}}
        }}
    } ;

    try {
        cout << rdr.renderString(R"(<title>{% block title %}xx{% endblock %}</title>

<h1>{{ block('title') }}</h1>

{% block body %}{% endblock %})", ctx) << endl;

        cout << rdr.renderString(R"({% if var ~ '/^[\\d\\.]+/' %}
    Do Stuff
{% endif %})", Variant::Object({{"var", "23xx"}})) << endl ;

  cout << rdr.renderString(R"({% set data = lipsum(40) %}{{ data }})", Variant::Object({{"var", "Hello, World!<html>"}, {
    "lipsum", Variant::Function([](const Variant::Array &args) -> Variant {
        Variant::Array vargs ;
        unpack_args(args, 1, 0, vargs) ;

        if ( !vargs.empty() ) {
            auto n = vargs[0].toInteger() ;
            string res ;
            for ( int64_t i = 0 ; i < n ; ++i ) res += "Lorem ipsum dolor sit amet, consectetur adipiscing elit. " ;
            return res ;
        }
        else return string() ;
    })}})
            ) << endl ;
    }
    catch ( TemplateCompileException  &e ) {
        cout << e.what() << endl ;
    }
}
