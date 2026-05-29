#include <gtest/gtest.h>
#include <twig/variant.hpp>
#include <twig/renderer.hpp>

using namespace twig;
using namespace std ;

class TagTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up common test data
    }

    void TearDown() override {
        // Clean up after tests
    }
};

// Test Variant construction and type detection
TEST_F(TagTest, ForBlockArray) {
  TemplateRenderer rdr(nullptr) ;

  const string loop1  =  R"({% for user in users %}{{loop.index}}. {{ user.name }} {% else %}No users have been found.{% endfor %})";
 
  Variant::Object ctx1{
        {"users", Variant::Array{
            Variant::Object{{"name", "Alice"}},
            Variant::Object{{"name", "Bob"}},
            Variant::Object{{"name", "Charlie"}}
        }}
    } ;

    try {
        string output =  rdr.renderString(loop1, ctx1) ;
        EXPECT_STREQ(output.c_str(), "1. Alice 2. Bob 3. Charlie ") ;
      
        output =  rdr.renderString(loop1, {}) ;
        EXPECT_STREQ(output.c_str(), "No users have been found.") ;
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
   
}

TEST_F(TagTest, Filter) {
  TemplateRenderer rdr(nullptr) ;

  const string code  =  R"({% filter upper %}text{% endfilter %})";
 
    try {
        string output =  rdr.renderString(code, {}) ;
        EXPECT_STREQ(output.c_str(), "TEXT") ;
      
//        output =  rdr.renderString(loop1, {}) ;
 //       EXPECT_STREQ(output.c_str(), "No users have been found.") ;
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
   
}

TEST_F(TagTest, Trim) {
  TemplateRenderer rdr(nullptr) ;

       vector<pair<string, string>> exprs{ 
    { R"(XX   {{-  name  }} XX)", "XXFabien XX" },
     { R"(XX   {{  name  -}} XX)", "XX   FabienXX" },
     {R"(XX   {%- block  name  -%}{{name}}{%endblock%} XX)", "XXFabienXX"}
       };
   

     Variant::Object ctx{{"name", "Fabien"}};
    
    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, ctx) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
    
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
   
}

TEST_F(TagTest, ForBlocDictionary) {
    TemplateRenderer rdr(nullptr) ;

    const string loop2 = R"(<ul>{% for key, user in users %}<li>{{ key }} {{ user|e }}</li>{% endfor %}</ul>)";
    
     Variant::Object ctx2{
        {"users", Variant::Object{
            {"Alice", "Cooper"},
            {"Bob", "Smith"},
            {"Charlie", "Brown"}    
        }}
    } ;

    try {
        string output =  rdr.renderString(loop2, ctx2) ;
        EXPECT_STREQ(output.c_str(), "<ul><li>Alice Cooper</li><li>Bob Smith</li><li>Charlie Brown</li></ul>") ;
    
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
};

TEST_F(TagTest, ForBlockSequence) {
    TemplateRenderer rdr(nullptr) ;

    const string loop1 = R"({% for i in 'a'|upper..'d'|upper %}* {{ i }} {% endfor %})";
    const string loop2 = R"({% for i in range(0, 3, 2) %}* {{ i }} {% endfor %})";
    try {
        string output =  rdr.renderString(loop1, {}) ;
        EXPECT_STREQ(output.c_str(), "* A * B * C * D ") ;
       output =  rdr.renderString(loop2, {}) ;
       EXPECT_STREQ(output.c_str(), "* 0 * 2 ") ;
    
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
};

TEST_F(TagTest, SetBlock) {
    TemplateRenderer rdr(nullptr) ;

      vector<pair<string, string>> exprs{ 
    { R"({% set name = 'Fabien' %}{{  name  }}{% endset %})", "Fabien" },
    { R"({% set name = 'Fabien' %}{% set name = 'John' %}{{ name }})", "John" },
    { R"({% set name = 'Fabien' %}{% set name = name|upper %}{{ name }})", "FABIEN" },
    { R"({% set map = {'city': 'Paris'} %}{% set map = map|merge({country: 'France'}) %}{{ map.city }} {{ map.country }})", "Paris France" },
    { R"({% for item in items %}{% set value = item %}{% endfor %}{{value}})", "" },
    { R"({% set a, b = 1, 3+4 %}{{ a+b }})", "8" },
    { R"({% set a%}Hello World {{items[1]}}{% endset %}{{a}})", "Hello World 2" }
    };

    Variant::Object ctx{{"items", Variant::Array{1, 2, 3}}};
    
    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, ctx) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
    
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
};

TEST_F(TagTest, ApplyBlock) {
    TemplateRenderer rdr(nullptr) ;

    vector<pair<string, string>> exprs{ 
        { R"({% apply upper %}hello{% endapply %})", "HELLO" },
        { R"({% apply lower|escape('html') %}<strong>SOME TEXT</strong>{% endapply %})", "&lt;strong&gt;some text&lt;/strong&gt;" },
    };
    
    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, {}) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
    
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
};

TEST_F(TagTest, VerbatimBlock) {
    TemplateRenderer rdr(nullptr) ;

      vector<pair<string, string>> exprs{ 
    { R"({% verbatim %}<ul>{% for item in seq %}<li>{{ item }}</li>{% endfor %}</ul>{% endverbatim %})", "<ul>{% for item in seq %}<li>{{ item }}</li>{% endfor %}</ul>" },
    };
    
    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, {}) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
    
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
};

TEST_F(TagTest, ExtendsBlock) {

    std::shared_ptr<TemplateLoader> loader(new DictTemplateLoader({
        {"base1.html.twig", R"({% block head %}<head><title>{% block title %}{% endblock %}</title></head>{% endblock %}<body>{% block content %}parent{% endblock %}</body><footer>{% block footer %}footer{% endblock %} {{block('title')}}</footer>)"},
        {"child1.html.twig", R"({% extends "base1.html.twig" %}{% block title %}title{% endblock %}{% block content %}<h1>Content-{{ parent() }}-{{ block('title') }}</h1>{% endblock %})"},
        {"base2.html.twig", R"({% for item in seq %}<li>{% block loop_item %}{{ item }}{% endblock %}</li>{% endfor %})"},
        {"child2.html.twig", R"({% extends "base2.html.twig" %}{% block loop_item %}{{loop.index}} - {{ item }}{% endblock %})"},
        {"base4.html.twig", R"({% block title %}t0{% endblock %}{%block content%}c0{%endblock%})"},
        {"base3.html.twig", R"({% extends "base4.html.twig"%} {% block content %}{% endblock %})"},
        {"child3.html.twig", R"({% extends "base3.html.twig" %}{% block title %}title-{{ parent() }}{% endblock %})"}
    })) ;
    TemplateRenderer rdr(loader) ;
   
    try {
        string output = rdr.render("child1.html.twig", {});
        EXPECT_STREQ(output.c_str(), "<head><title>title</title></head><body><h1>Content-parent-title</h1></body><footer>footer title</footer>") ;
            
        output = rdr.render("child2.html.twig", {{"seq", Variant::Array{"One", "Two", "Three"}}});
        EXPECT_STREQ(output.c_str(), "<li>1 - One</li><li>2 - Two</li><li>3 - Three</li>") ;

        output = rdr.render("child3.html.twig", {});
        EXPECT_STREQ(output.c_str(), "title-t0") ;
    
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
};


TEST_F(TagTest, MacroBlock) {

    std::shared_ptr<TemplateLoader> loader(new DictTemplateLoader({
        {"base1.html.twig", R"({% macro print_list(data, n=1) %}{%for item in data%}<li>{{loop.index0 + n}}-{{item}}</li>{%endfor%}{% endmacro %})"},
        {"child1.html.twig", R"({% import "base1.html.twig" as util %}{{ util.print_list(['A', 'B'])}})"},
        {"child2.html.twig", R"({% from "base1.html.twig" import print_list as pl %}{{ pl(['A', 'B'],n:3)}})"},
         {"base2.html.twig", R"({% macro m1(data) %}{{ data | join(',')}}{{ _self.length(data)}}{% endmacro %}{%macro length(data)%}-{{data | length}}{% endmacro %} )"},
        {"child3.html.twig", R"({% from "base2.html.twig" import m1%}{{ m1(['A', 'B'])}})"},
   
    })) ;
    TemplateRenderer rdr(loader) ;
  
    try {
            string output = rdr.render("child1.html.twig", {});
            EXPECT_STREQ(output.c_str(), "<li>1-A</li><li>2-B</li>") ;
            output = rdr.render("child2.html.twig", {});
            EXPECT_STREQ(output.c_str(), "<li>3-A</li><li>4-B</li>") ;
            output = rdr.render("child3.html.twig", {});
            EXPECT_STREQ(output.c_str(), "A,B-2") ;
          
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
};

TEST_F(TagTest, IncludeBlock) {
    std::shared_ptr<TemplateLoader> loader(new DictTemplateLoader({
        {"base.html.twig", R"({{name}})"},
        {"child1.html.twig", R"(Hello {% include 'base.html.twig' with {'name': 'Fabien'} %})"},
        {"child2.html.twig", R"(Hello {% include 'base.html.twig' only %})"},
        {"child3.html.twig", R"({%set name='Fabien'%}{{include("base.html.twig", ignore_missing:true)}})"},
    })) ;
    TemplateRenderer rdr(loader) ;

    try {
        string output = rdr.render("child1.html.twig", {});
        EXPECT_STREQ(output.c_str(), "Hello Fabien") ;

        output = rdr.render("child2.html.twig", {});
        EXPECT_STREQ(output.c_str(), "Hello ") ;

        output = rdr.render("child3.html.twig", {});
        EXPECT_STREQ(output.c_str(), "Fabien") ;

          
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
};


TEST_F(TagTest, EmbedBlock) {

    std::shared_ptr<TemplateLoader> loader(new DictTemplateLoader({
        {"main1.twig", R"(<main>{% embed "card.twig" %}{% block card_body %}<p>This is overridden body content.</p>{% endblock %}{% endembed %}</main>)"},
        {"card.twig", R"(<div class="card"><div class="card-header">{% block card_header %}Default Header{% endblock %}</div><div class="card-body">{% block card_body %}Default Body Content{% endblock %}</div></div>)"},
        {"main2.twig", R"(<div>{% embed ["custom/modal.twig", "core/modal.twig"] %}{% block modal_title %}Warning{% endblock %}{% endembed %}</div>)"},
        {"core/modal.twig", R"(<div class="modal"><h2>{% block modal_title %}Default Title{% endblock %}</h2><p>Modal Content</p></div>)"},
        {"page.twig", R"({% embed "alert.twig" %}{% block alert_content %}<strong>Error:</strong>{{ parent() }}{% endblock %}{% endembed %})"},
        {"alert.twig", R"(<div class="alert">{% block alert_content %}Something went wrong!{% endblock %}</div>)"},
        {"page2.twig", R"({% embed "advanced_input.twig" %}{% block label %}Username:{% endblock %}{% endembed %})"},
        {"advanced_input.twig", R"({% extends "base_field.twig" %}{% block input_element %}<input type="text">{% endblock %})"},
        {"base_field.twig", R"(<div class="field"><label>{% block label %}Default Label{% endblock %}</label><div class="control">{% block input_element %}{% endblock %}</div></div>)"},
    
    })) ;
    TemplateRenderer rdr(loader) ;
   
    try {
        string output = rdr.render("main1.twig", {});
        EXPECT_STREQ(output.c_str(), R"(<main><div class="card"><div class="card-header">Default Header</div><div class="card-body"><p>This is overridden body content.</p></div></div></main>)") ;           

        output = rdr.render("main2.twig", {});
        EXPECT_STREQ(output.c_str(), R"(<div><div class="modal"><h2>Warning</h2><p>Modal Content</p></div></div>)") ;           
    
        output = rdr.render("page.twig", {});
        EXPECT_STREQ(output.c_str(), R"(<div class="alert"><strong>Error:</strong>Something went wrong!</div>)") ;           
    
        output = rdr.render("page2.twig", {});
        EXPECT_STREQ(output.c_str(), R"(<div class="field"><label>Username:</label><div class="control"><input type="text"></div></div>)") ;           
    
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
};
