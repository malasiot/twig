#include <gtest/gtest.h>
#include <twig/variant.hpp>
#include <twig/renderer.hpp>
#include <ctime>

using namespace twig;
using namespace std ;

class FunctionTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up common test data
    }

    void TearDown() override {
        // Clean up after tests
    }
};

// Test Variant construction and type detection
TEST_F(FunctionTest, Cycle) {
  TemplateRenderer rdr(nullptr) ;

  const string tmpl  =  R"({% set start_year = date() | date('Y') %}
{% set end_year = start_year + 5 %}
{% for year in start_year..end_year %}
    {{ cycle(['odd', 'even'], loop.index0) }}
{% endfor %})";
 
  Variant::Object ctx1{
        {"users", Variant::Array{
            Variant::Object{{"name", "Alice"}},
            Variant::Object{{"name", "Bob"}},
            Variant::Object{{"name", "Charlie"}}
        }}
    } ;

    try {
        string output =  rdr.renderString(tmpl, ctx1) ;
        EXPECT_STREQ(output.c_str(), "odd even odd even odd even ") ;
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
   
}

TEST_F(FunctionTest, DateFilterNow) {
    TemplateRenderer rdr(nullptr) ;
    const string tmpl = R"({{ date() | date('Y') }})" ;

    time_t now = time(nullptr) ;
    std::tm tm = *std::localtime(&now) ;
    string expected = to_string(1900 + tm.tm_year) ;

    try {
        string output = rdr.renderString(tmpl, Variant::Object()) ;
        EXPECT_EQ(output, expected) ;
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}

TEST_F(FunctionTest, DateFilterStatic) {
    TemplateRenderer rdr(nullptr) ;
    const string tmpl = R"({{ date('2024-05-20 15:34:00') | date('Y-m-d H:i:s') }})" ;

    try {
        string output = rdr.renderString(tmpl, Variant::Object()) ;
        EXPECT_EQ(output, "2024-05-20 15:34:00") ;
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}
