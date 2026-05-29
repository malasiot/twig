#include <gtest/gtest.h>
#include <twig/variant.hpp>
#include <twig/renderer.hpp>

using namespace twig;
using namespace std ;

class ExpressiongTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Set up common test data
    }

    void TearDown() override {
        // Clean up after tests
    }
};
#if 1
// Test Variant construction and type detection
TEST_F(ExpressiongTest, TernaryOperator) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    { R"({{ true ?: 'greater' }})", "1" },
    { R"({{ false ?: 'greater' }})", "greater" },
    { R"({{ 0 ?: 'greater' }})", "greater" },
    { R"({{ 1 ?: 'greater' }})", "1" },
   { R"({{ '' ?: 'greater' }})", "greater" },
    { R"({{ 'non-empty' ?: 'greater' }})", "non-empty" },
    { R"({{ null ?: 'greater' }})", "greater" },
   { R"({{ [] ?: 'greater' }})", "greater" },
    { R"({{ {} ?: 'greater' }})", "greater" },
    { R"({{ 0.0 ?: 'greater' }})", "greater" },
    { R"({{ 0.1 ?: 'greater' }})", "0.1" },
    { R"({{ false ? 'less' : 'greater' }})", "greater" },
    { R"({{ true ? 'less' : 'greater' }})", "less" },
      { R"({{ data ?? 'less' }})", "less" }

};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object()) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }

   
}

TEST_F(ExpressiongTest, Arithmetic) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    { R"({{  2 + (3 - 4)/3}})", "1.66666" },
   { R"({{ 2 * (3 - 4)/3 }})", "-0.66666" },
    { R"({{ 2 / (3 - 4)/3}})", "-0.66666" },
    { R"({{ 2 % (3 - 4)/3}})", "0" },
    { R"({{ 2 ** (3 - 4)/3}})", "0.16666" },
    { R"({{ 5 // 2}})", "2" },
    { R"({{ 5 // 0}})", "0" }

};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object()) ;
            ASSERT_NEAR(stod(output), stod(expr.second), 1e-5) ;
        }
      
    } catch ( TemplateCompileException &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }

   
}


TEST_F(ExpressiongTest, Concat) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    { R"({{ 2 ~ 'hello' ~ ( ' ' ~ 'world' ) }})", "2hello world" },
    { R"({{ 'foo' ~ 'bar' }})", "foobar" },
    { R"({{ 'foo' ~ 123 }})", "foo123" },
   { R"({{ 123 ~ 'foo' }})", "123foo" },
    { R"({{ 'foo' ~ null }})", "foo" },
    { R"({{ null ~ 'foo' }})", "foo" },
    { R"({{ 'foo' ~ true }})", "foo1" },
    { R"({{ true ~ 'foo' }})", "1foo" },
    { R"({{ 'foo' ~ false }})", "foo0" },
    { R"({{ test ~ 'foo' }})", "foo" },

};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object()) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
      
    } catch ( std::exception &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}


TEST_F(ExpressiongTest, Conditional) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    { R"({{ true and false ? 'true' : 'false' }})", "false" },
    { R"({{ true or false ? 'true' : 'false' }})", "true" },
    { R"({{ false and not ( false or true ) ? 'true' : 'false' }})", "false" },
    { R"({{ false or false ? 'true' : 'false' }})", "false" },
    { R"({{ true and not true ? 'true' : 'false' }})", "false" },
    { R"({{ true or true ? 'true' : 'false' }})", "true" },
    { R"({{ 3 in [1, 2, 4]  ? 'true' : 'false' }})", "false" },
    { R"({{ 3 in [1, 2, 3]  ? 'true' : 'false' }})", "true" },
    { R"({{ 3+1 not in [1, 2, 4]  ? 'true' : 'false' }})", "false" },
    { R"({{ "3" in [1, 2, 3]  ? 'true' : 'false' }})", "true" },
    { R"({{ "hello" starts with "he" and "hello" ends with "lo" ? 'true' : 'false' }})", "true" },
    { R"({{ "hello" matches "/^h.*o$/" ? 'true' : 'false' }})", "true" },
    { R"({{ "hello" matches "/^h.*a$/" ? 'true' : 'false' }})", "false" },
    { R"({{ not "hello"  matches "/^a.*o$/" ? 'true' : 'false' }})", "true" },
    { R"({{ "hello" matches "/^a.*a$/" ? 'true' : 'false' }})", "false" },
    { R"({{ "hello" matches "/^h.*a$/" ? 'true' : 'false' }})", "false" },
  //  { R"({{ ( 3 is divisible by 2 ) ? 'true' : 'false' }})", "false" }, // we need the parentheses otherwise the test will be parsed as 3 is (divisible by 2 ? 'true' : 'false') which is not what we want
    { R"({{ 4 is even  ? 'true' : 'false' }})", "true" },

};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object()) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
      
    } catch ( std::exception &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}

TEST_F(ExpressiongTest, Filters) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    { R"({{ "  hello world  " | trim | capitalize }})", "Hello world" },
    { R"({{ "  hello world  " | trim(" ") }})", "hello world" },
    { R"({{ "hello world " ~ " test " | trim(" ")}})", "hello world test" },

};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object()) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
      
    } catch ( std::exception &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}



TEST_F(ExpressiongTest, Arrow) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    {  R"({{ [10, 2, 40, 42]|filter(v => v > 38)|join(', ') }})", "40, 42" },
    {  R"({{ {xs: 34, s:  36, m:  38, l:  40, xl: 42}|filter((k, v) => v > 38 and k != "xl")|keys | join(', ') }})", "l" }
};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object()) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
      
    } catch ( std::exception &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}

TEST_F(ExpressiongTest, Assignment) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    {  R"({{ (a = [40, 42]) | join(', ') }})", "40, 42" },
    {  R"({{ ([a, ,b] = [40, 42, 43]) | join(', ') }})", "40, 43" },
        {  R"({{ ([ ,b] = [40, 42, 43]) }}{{b}})", "42" },
    {  R"({{ ({a, b:data} = {a: 40, b: 42, c: 43}) }}{{a ~ ' ' ~ data}})", "40 42" },

};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object()) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
      
    } catch ( std::exception &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}
#endif
TEST_F(ExpressiongTest, Indexing) {
  TemplateRenderer rdr(nullptr) ;

  Variant::Object ctx ;
  ctx["a"] = Variant::Array{10, 20, Variant::Array{30, 35, 38}, 40, 42} ;
  ctx["b"] = Variant::Object{{"name", "John"}, {"surname", "Miles"}, {"telephone", Variant::Array{"1234", "456"}}};

   vector<pair<string, string>> exprs{ 
    {  R"({{ a[0+2][1] }})", "35" },
     {  R"({{ b.name ~ ' ' ~ b.surname }})", "John Miles" },
     {  R"({{ b['name'] ~ ' ' ~ b['surname'] ~ ' ' ~ b.telephone[1]}})", "John Miles 456" },
    {  R"({{ (b?.data) ?? "ok" }})", "ok" }, // precedance

};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, ctx) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
      
    } catch ( std::exception &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}

TEST_F(ExpressiongTest, HasSomeEvery) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    {  R"({{  ( [34, 36, 38, 40, 42] has every v => v > 38 ) ? "true" : "false"}})", "false" },
     {  R"({{  ( [34, 36, 38, 40, 42] has some v => v > 38 ) ? "true" : "false"}})", "true" },
      {  R"({{  ( [] has some v => v > 38 ) ? "true" : "false"}})", "false" },

};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object{}) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
      
    } catch ( std::exception &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}

TEST_F(ExpressiongTest, Spread) {
  TemplateRenderer rdr(nullptr) ;

   vector<pair<string, string>> exprs{ 
    {  R"({% set a = [1, 2] %}{{  [34, ...a, 35] | join(',') }})", "34,1,2,35" },
     {  R"({% set a = [] %}{{  [34, ...a] | join(',') }})", "34" },
     {  R"({{'Hello %s %s!'|format(...['Fabien', 'Potencier'])  }})", "Hello Fabien Potencier!" },
    
};

    try {
        for ( auto &&expr: exprs ) {
            string output =  rdr.renderString(expr.first, Variant::Object{}) ;
            EXPECT_STREQ(output.c_str(), expr.second.c_str()) ;
        }
      
    } catch ( std::exception &e ) {
        FAIL() << "Compilation failed: " << e.what() ;
    }
}