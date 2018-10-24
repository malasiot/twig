#include <twig/renderer.hpp>
#include <twig/exceptions.hpp>
#include <iostream>

using namespace std ;
using namespace twig ;

const string msg = R"(
hello***    {% if a.x[2] > 3 %}   {{- 'if' -}}  {% else %} else {%- endif -%}  ***
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

    TemplateRenderer rdr(make_shared<StringTemplateLoader>(msg)) ;

    try {
        cout << rdr.render("--", Variant::Object{{ "a" , Variant::Object{{ "x", Variant::Array{{ 2, 3, 4, 5}} }} },
                                                 {"f", Variant::Function([&](const Variant &v)->Variant {
                                                      return 3;})}}) << endl;
    }
    catch ( TemplateCompileException  &e ) {
        cout << e.what() << endl ;
    }
}
