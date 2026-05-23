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

    std::shared_ptr<TemplateLoader> loader(new ArrayTemplateLoader({
        {"base1.html.twig", R"({% block head %}<head><title>{% block title %}{% endblock %}</title></head>{% endblock %}<body>{% block content %}{% endblock %}</body><footer>{% block footer %}footer{% endblock %}</footer>)"},
        {"child1.html.twig", R"({% extends "base1.html.twig" %}{% block title %}title{% endblock %}{% block head %}{{ parent() }}-Head{% endblock %}{% block content %}<h1>Content</h1>{% endblock %})"},
        {"base2.html.twig", R"({% for item in seq %}<li>{% block loop_item %}{{ item }}{% endblock %}</li>{% endfor %})"},
        {"child2.html.twig", R"({% extends "base2.html.twig" %}{% block loop_item %}{{loop.index}} - {{ item }}{% endblock %})"}
    })) ;
    TemplateRenderer rdr(loader) ;

    try {
            string output = rdr.render("child1.html.twig", {});
            EXPECT_STREQ(output.c_str(), "<head><title>title</title></head>-Head<body><h1>Content</h1></body><footer>footer</footer>") ;
            
            output = rdr.render("child2.html.twig", {{"seq", Variant::Array{"One", "Two", "Three"}}});
            EXPECT_STREQ(output.c_str(), "<li>1 - One</li><li>2 - Two</li><li>3 - Three</li>") ;
    
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
};