#include <gtest/gtest.h>
#include <twig/variant.hpp>
#include <twig/renderer.hpp>

using namespace twig;
using namespace std ;

class FormsTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up common test data
    }

    void TearDown() override {
        // Clean up after tests
    }
};

inline std::string trim(std::string& str)
{
    str.erase(str.find_last_not_of(" \n\t") + 1);  // Suffixing spaces
    str.erase(0, str.find_first_not_of(" \n\t"));  // Prefixing spaces
    return str;
}

// Test Variant construction and type detection
TEST_F(FormsTest, Themes) {

    std::shared_ptr<DictTemplateLoader> loader( new DictTemplateLoader(
        {
            {"theme.html.twig", R"({% block form_start %}
    <form name="{{ _full_name }}" method="POST" action="{{_action}}">
{% endblock %}

{% block form_end %}
    {# form_end lambda will output remaining fields before appending this #}
    </form>
{% endblock %}

{% block form_row %}
    <div class="form-row-container" style="margin-bottom: 15px;">
        {{ form_label }}
        {{ form_errors }}
        {{ form_widget }}
    </div>
{% endblock %}

{# ======================================================================= #}
{# Base Component Fallbacks                                               #}
{# ======================================================================= #}

{% block form_label %}
    <label for="{{ _id }}" style="font-weight: bold; display: block;">{{ label }}</label>
{% endblock %}

{% block form_errors %}
    {% if errors %}
        <span class="error-msg" style="color: red; font-size: 12px;">{{ errors }}</span>
    {% endif %}
{% endblock %}


{# ======================================================================= #}
{# Input Field Widgets (Testing Reverse Prefix Hierarchies)              #}
{# ======================================================================= #}

{# Generic default field widget #}
{% block form_widget %}
    <input type="text" id="{{ _id }}" name="{{ _full_name }}" value="{{ value }}" class="form-control" />
{% endblock %}

{# Specific text input widget (Matches ["form", "text"]) #}
{% block text_widget %}
    <input type="text" id="{{ _id }}" name="{{ _full_name }}" value="{{ value }}" placeholder="Enter {{ label }}..." style="padding: 5px; width: 100%;" />
{% endblock %}

{# Specific email input widget (Matches ["form", "text", "email"]) #}
{% block email_widget %}
    <input type="email" id="{{ _id }}" name="{{ _full_name }}" value="{{ value }}" placeholder="name@example.com" style="padding: 5px; border: 1px solid blue;" />
{% endblock %}

{# Specific textarea widget (Matches ["form", "textarea"]) #}
{% block textarea_widget %}
    <textarea id="{{ _id }}" name="{{ _full_name }}" style="padding: 5px; width: 100%; height: 80px;">{{ value }}</textarea>
{% endblock %}
)"},
            {"base.html.twig", R"( {% form_theme login_form 'theme.html.twig' %}
{{ form_start(login_form) }}
    {{ form_row(login_form.username) }} 
{{ form_end(login_form) }})"}
        }));
  TemplateRenderer rdr(loader) ;
 
  Variant::Object ctx{
        {"login_form", Variant::Object{
            {"_full_name", "login_form"},
            {"action", "/data.php"},
            {"method", "POST"},
            {"username", Variant::Object{
                {"_id", "user_name"},
                {"_full_name", "login_form[username]" },
                {"_prefixes", Variant::Array{"text"}},
                {"label", "Username"},
                {"value", "guest"}
            }}
        }
    }
    } ;

  
    try {
        string output =  rdr.render("base.html.twig", ctx) ;
        EXPECT_STREQ(trim(output).c_str(), "<form name=\"Alice\" method=\"POST\" action=\"/data.php\">") ;
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
   
}
