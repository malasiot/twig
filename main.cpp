#include <twig/renderer.hpp>
#include <twig/exceptions.hpp>
#include <iostream>

using namespace std ;
using namespace twig ;

const string msg = R"(
hello {{ 1 + 2 * 3 }} and {% block xx %} and {% endblock %}
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
        rdr.render("--", Variant::Object{{ "data" , "ok"}}) ;
    }
    catch ( TemplateCompileException  &e ) {
        cout << e.what() << endl ;
    }
}
